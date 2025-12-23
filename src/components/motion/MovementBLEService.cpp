#include "MovementBLEService.h"
#include "components/ble/NimbleController.h"
#include <host/ble_hs.h>
#include <host/ble_gatt.h>
#include <nrf_log.h>
#include <cstring>

using namespace Pinetime::Controllers;

// ============================================================================
// UUID Definitions
// ============================================================================

// Service UUID: 00060000-78fc-48fe-8e23-433b3a1942d0
// Characteristic UUID: 00060001-78fc-48fe-8e23-433b3a1942d0

// Note: Using static storage (not constexpr) to ensure stable addresses for NimBLE
static const ble_uuid128_t movementServiceUuid = {
  .u = {.type = BLE_UUID_TYPE_128},
  .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, 0x00, 0x00, 0x06, 0x00}
};

static const ble_uuid128_t movementCharUuid = {
  .u = {.type = BLE_UUID_TYPE_128},
  .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, 0x01, 0x00, 0x06, 0x00}
};

static int MovementServiceCallback(uint16_t /*conn_handle*/, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
  auto* movementService = static_cast<MovementBLEService*>(arg);
  return movementService->OnCharacteristicAccess(attr_handle, ctxt);
}

// ============================================================================
// MovementBLEService Implementation
// ============================================================================

MovementBLEService::MovementBLEService(NimbleController& nimble)
  : nimble {nimble},
    characteristicDefinition {{.uuid = &movementCharUuid.u,
                               .access_cb = MovementServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                               .val_handle = &movementCharacteristicHandle},
                              {0}},
    serviceDefinition {
      {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &movementServiceUuid.u, .characteristics = characteristicDefinition},
      {0},
    } {
}

void MovementBLEService::Init() {
  NRF_LOG_INFO("Movement BLE Service: Starting initialization...");

  int res = ble_gatts_count_cfg(serviceDefinition);
  if (res != 0) {
    NRF_LOG_ERROR("Movement BLE Service: ble_gatts_count_cfg FAILED with error %d", res);
    return;
  }
  NRF_LOG_INFO("Movement BLE Service: count_cfg OK");

  res = ble_gatts_add_svcs(serviceDefinition);
  if (res != 0) {
    NRF_LOG_ERROR("Movement BLE Service: ble_gatts_add_svcs FAILED with error %d", res);
    return;
  }
  NRF_LOG_INFO("Movement BLE Service: add_svcs OK");

  NRF_LOG_INFO("Movement BLE Service: Initialized with char handle=%d", movementCharacteristicHandle);
}

int MovementBLEService::OnCharacteristicAccess(uint16_t attributeHandle, ble_gatt_access_ctxt* context) {
  if (attributeHandle == movementCharacteristicHandle) {
    NRF_LOG_DEBUG("Movement Service: READ characteristic (handle=%d)", attributeHandle);

    auto buffer = lastData.serialize();
    int res = os_mbuf_append(context->om, buffer.data(), buffer.size());
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  return 0;
}

bool MovementBLEService::sendMovementData(const MovementData& data) {
  // Store for read requests
  lastData = data;

  if (!notificationEnabled) {
    return false;  // Notifications not enabled by client
  }

  uint16_t connectionHandle = nimble.connHandle();

  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return false;  // No connection
  }

  // Serialize the data
  auto buffer = data.serialize();

  // Create an mbuf for the notification
  auto* om = ble_hs_mbuf_from_flat(buffer.data(), buffer.size());
  if (!om) {
    NRF_LOG_ERROR("Movement Service: Failed to allocate mbuf");
    return false;
  }

  // Send notification
  int rc = ble_gattc_notify_custom(connectionHandle, movementCharacteristicHandle, om);

  if (rc != 0 && rc != BLE_HS_ENOTCONN) {
    NRF_LOG_ERROR("Movement Service: Failed to send notification, rc=%d", rc);
    return false;
  }

  return true;
}

void MovementBLEService::SubscribeNotification(uint16_t attributeHandle) {
  if (attributeHandle == movementCharacteristicHandle) {
    notificationEnabled = true;
    NRF_LOG_INFO("Movement Service: Notifications enabled");
  }
}

void MovementBLEService::UnsubscribeNotification(uint16_t attributeHandle) {
  if (attributeHandle == movementCharacteristicHandle) {
    notificationEnabled = false;
    NRF_LOG_INFO("Movement Service: Notifications disabled");
  }
}

// ============================================================================
// MovementService Implementation (Singleton)
// ============================================================================

static MovementService* g_movement_service = nullptr;

MovementService& MovementService::getInstance() {
  if (g_movement_service == nullptr) {
    static MovementService instance;
    g_movement_service = &instance;
  }
  return *g_movement_service;
}

void MovementService::init(MovementBLEService& bleService) {
  // Store reference to the BLE service (owned by NimbleController)
  ble_service = &bleService;

  analyzer.reset();

  NRF_LOG_INFO("MovementService: analyzer initialized, BLE service at %p", ble_service);
}

void MovementService::updateAccelerometer(const std::array<float, 3>& gravity_corrected_xl,
                                          uint32_t delta_time_ms) {
  // Accumulate time in uint64_t to avoid overflow
  system_time_ms += delta_time_ms;

  // Run analysis algorithm
  auto result = analyzer.analyze(gravity_corrected_xl, delta_time_ms);

  // Update status
  current_status.magnitude_active_time = result.magnitude_active_time;
  current_status.axis_active_time = result.axis_active_time;
  current_status.movement_detected = result.movement_detected;
  current_status.any_movement = result.any_movement;
  current_status.accel_x = gravity_corrected_xl[0];
  current_status.accel_y = gravity_corrected_xl[1];
  current_status.accel_z = gravity_corrected_xl[2];

  // Calculate progress percentage (max 3600 seconds = 1 hour)
  constexpr uint32_t MAX_ACTIVE_TIME = 3600000; // milliseconds
  current_status.progress_percent =
      (static_cast<float>(result.axis_active_time) / MAX_ACTIVE_TIME) * 100.0f;
  if (current_status.progress_percent > 100.0f) {
    current_status.progress_percent = 100.0f;
  }

  // Send via BLE if service is available
  if (ble_service) {
    // Prepare data for BLE transmission
    MovementData ble_data;
    ble_data.timestamp_ms = static_cast<uint32_t>(system_time_ms % 0x100000000ULL);
    ble_data.magnitude_active_time = result.magnitude_active_time;
    ble_data.axis_active_time = result.axis_active_time;
    ble_data.movement_detected = result.movement_detected;
    ble_data.any_movement = result.any_movement;

    // Scale accelerometer data (multiply by 100 to preserve 2 decimal places)
    ble_data.accel_x = static_cast<int16_t>(gravity_corrected_xl[0] * 100.0f);
    ble_data.accel_y = static_cast<int16_t>(gravity_corrected_xl[1] * 100.0f);
    ble_data.accel_z = static_cast<int16_t>(gravity_corrected_xl[2] * 100.0f);

    // Send via BLE to connected client (if subscribed)
    ble_service->sendMovementData(ble_data);
  }
}

MovementService::CurrentStatus MovementService::getCurrentStatus() const {
  return current_status;
}

void MovementService::resetStatistics() {
  analyzer.reset();
  current_status = {0, 0, false, false, 0.0f};
  system_time_ms = 0;
}
