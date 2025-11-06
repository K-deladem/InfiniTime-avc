#include "MovementBLEService.h"
#include <host/ble_hs.h>
#include <host/ble_gatt.h>
#include <nrf_log.h>
#include <cstring>

// ============================================================================
// Global instance pointer for callback wrapper
// ============================================================================

static MovementBLEService* g_movement_ble_service = nullptr;

// ============================================================================
// NimBLE callback wrapper (non-member function)
// ============================================================================

int movementCharacteristicCallback(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt* ctxt, void* arg) {
  (void)arg;  // Unused parameter
  
  if (g_movement_ble_service) {
    return g_movement_ble_service->handleCharacteristicAccess(conn_handle, attr_handle, ctxt);
  }
  return BLE_ATT_ERR_UNLIKELY;
}

// ============================================================================
// MovementBLEService Implementation
// ============================================================================

MovementBLEService::MovementBLEService() : connectedClientCount(0) {
  g_movement_ble_service = this;
  std::memset(&characteristicDefinition, 0, sizeof(characteristicDefinition));
  std::memset(&serviceDefinition, 0, sizeof(serviceDefinition));
  
  // Initialize characteristic definition
  characteristicDefinition[0] = {
    .uuid = (ble_uuid_t*)&characteristicUuid,
    .access_cb = movementCharacteristicCallback,
    .arg = this,
    .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ,
    .val_handle = &movementCharacteristicHandle,
  };
  // characteristicDefinition[1] is already zeroed (terminator)
  
  // Initialize service definition
  serviceDefinition[0] = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = (ble_uuid_t*)&serviceUuid,
    .characteristics = characteristicDefinition,
  };
  // serviceDefinition[1] is already zeroed (terminator)
}

// Initialize the BLE service with NimBLE
int MovementBLEService::init() {
  int rc = 0;
  
  // Count the service/characteristic configuration
  rc = ble_gatts_count_cfg(serviceDefinition);
  if (rc != 0) {
    NRF_LOG_ERROR("Movement Service: ble_gatts_count_cfg failed, rc=%d", rc);
    return rc;
  }
  
  // Add the service to the GATT server
  rc = ble_gatts_add_svcs(serviceDefinition);
  if (rc != 0) {
    NRF_LOG_ERROR("Movement Service: ble_gatts_add_svcs failed, rc=%d", rc);
    return rc;
  }
  
  NRF_LOG_INFO("Movement BLE Service initialized successfully");
  return 0;
}

// Send movement data via BLE notification
bool MovementBLEService::sendMovementData(const MovementData& data) {
  if (connectedClientCount == 0) {
    return false;  // No connected clients
  }
  
  // Serialize the data
  auto buffer = data.serialize();
  
  // Create an mbuf for the notification
  os_mbuf* om = ble_hs_mbuf_from_flat(buffer.data(), buffer.size());
  if (!om) {
    NRF_LOG_ERROR("Movement Service: Failed to allocate mbuf");
    return false;
  }
  
  // Send notification to all connected clients
  // Use 0xFFFF to notify all connections
  int rc = ble_gattc_notify_custom(0xFFFF, movementCharacteristicHandle, om);
  
  if (rc != 0 && rc != BLE_HS_ENOTCONN) {
    NRF_LOG_ERROR("Movement Service: Failed to send notification, rc=%d", rc);
    return false;
  }
  
  return true;
}

// Register data received callback
void MovementBLEService::onDataReceived(DataCallback cb) {
  dataReceivedCallback = cb;
}

// Handle client connection
void MovementBLEService::onConnect(uint16_t conn_handle) {
  (void)conn_handle;  // Unused in this simple implementation
  connectedClientCount++;
  NRF_LOG_INFO("Movement Service: Client connected (total: %d)", connectedClientCount);
}

// Handle client disconnection
void MovementBLEService::onDisconnect(uint16_t conn_handle) {
  (void)conn_handle;  // Unused in this simple implementation
  if (connectedClientCount > 0) {
    connectedClientCount--;
  }
  NRF_LOG_INFO("Movement Service: Client disconnected (total: %d)", connectedClientCount);
}

// Check if any client is connected
bool MovementBLEService::isConnected() const {
  return connectedClientCount > 0;
}

// Get number of connected clients
int MovementBLEService::getConnectedClientCount() const {
  return connectedClientCount;
}

// NimBLE characteristic access callback - now a member function
int MovementBLEService::handleCharacteristicAccess(uint16_t conn_handle, uint16_t attr_handle,
                                                   struct ble_gatt_access_ctxt* ctxt) {
  (void)conn_handle;  // Unused
  
  // Handle READ requests
  if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    NRF_LOG_DEBUG("Movement Service: READ characteristic (handle=%d)", attr_handle);
    // Return empty data for read - real data is sent via notifications
    return 0;
  }
  
  // Handle WRITE requests (if Flutter needs to send commands)
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    NRF_LOG_DEBUG("Movement Service: WRITE characteristic (handle=%d, len=%d)", 
                  attr_handle, OS_MBUF_PKTLEN(ctxt->om));
    
    // Extract data from mbuf
    if (ctxt->om && OS_MBUF_PKTLEN(ctxt->om) >= 22) {
      std::array<uint8_t, 22> buffer;
      int rc = ble_hs_mbuf_to_flat(ctxt->om, buffer.data(), buffer.size(), nullptr);
      
      if (rc == 0 && dataReceivedCallback) {
        MovementData data = MovementData::deserialize(buffer);
        dataReceivedCallback(data);
      }
    }
    return 0;
  }
  
  return BLE_ATT_ERR_UNLIKELY;
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

void MovementService::init() {
  // Initialize the BLE service
  int rc = ble_service.init();
  if (rc != 0) {
    return;
  }
  
  analyzer.reset();
  
  // Setup data callback (stub)
  ble_service.onDataReceived([this](const MovementData& data) {
    (void)data;  // Avoid unused warning
    // Handle received data from Flutter if needed
  });
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

  // Calculate progress percentage (example: max 3600 seconds = 1 hour)
  constexpr uint32_t MAX_ACTIVE_TIME = 3600000; // milliseconds
  current_status.progress_percent =
      (static_cast<float>(result.axis_active_time) / MAX_ACTIVE_TIME) * 100.0f;
  if (current_status.progress_percent > 100.0f) {
    current_status.progress_percent = 100.0f;
  }

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

  // Send via BLE to all connected clients (works even if display is off)
  ble_service.sendMovementData(ble_data);
}

MovementService::CurrentStatus MovementService::getCurrentStatus() const {
  return current_status;
}

void MovementService::resetStatistics() {
  analyzer.reset();
  current_status = {0, 0, false, false, 0.0f};
  system_time_ms = 0;
}

void MovementService::onBleConnect(uint16_t conn_handle) {
  ble_service.onConnect(conn_handle);
}

void MovementService::onBleDisconnect(uint16_t conn_handle) {
  ble_service.onDisconnect(conn_handle);
}