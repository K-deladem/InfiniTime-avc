#include "MovementBLEService.h"
#include <algorithm>
#include <cstring>

// ============================================================================
// NimBLE UUID Definitions
// ============================================================================

// Movement Service UUID: 12345678-1234-5678-1234-56789abcdef0
const ble_uuid128_t MovementBLEService::service_uuid = {
    .u = {.type = BLE_UUID_TYPE_128},
    .value = {0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
              0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x78, 0x56}
};

// Movement Data Characteristic UUID: 87654321-4321-8765-4321-0fedcba98765
const ble_uuid128_t MovementBLEService::movement_data_uuid = {
    .u = {.type = BLE_UUID_TYPE_128},
    .value = {0x65, 0x87, 0xa9, 0x0f, 0x21, 0x43, 0x65, 0x87,
              0x21, 0x43, 0x21, 0x43, 0x87, 0x65, 0x21, 0x43}
};

// Command Characteristic UUID: 11111111-1111-1111-1111-111111111111
const ble_uuid128_t MovementBLEService::command_uuid = {
    .u = {.type = BLE_UUID_TYPE_128},
    .value = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
              0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11}
};

// Singleton instance
MovementBLEService* MovementBLEService::instance = nullptr;

// ============================================================================
// GATT Service Definition
// ============================================================================

// FIX: GATT Service array - structure for NimBLE
static const struct ble_gatt_svc_def movement_gatt_svcs[] = {
    {
        // Primary service: Movement Analysis
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (ble_uuid_t*)&MovementBLEService::service_uuid,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                // Characteristic: Movement Data (notify)
                .uuid = (ble_uuid_t*)&MovementBLEService::movement_data_uuid,
                .access_cb = MovementBLEService::gattServerCallback,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .descriptors = nullptr,
            },
            {
                // Characteristic: Command (write)
                .uuid = (ble_uuid_t*)&MovementBLEService::command_uuid,
                .access_cb = MovementBLEService::gattServerCallback,
                .flags = BLE_GATT_CHR_F_WRITE,
                .descriptors = nullptr,
            },
            {
                // Terminator
                0,
            }
        },
    },
    {
        // Terminator
        0,
    }
};

// ============================================================================
// MovementBLEService Implementation
// ============================================================================

MovementBLEService::MovementBLEService() 
    : movement_data_char_handle(0), command_char_handle(0) {
  instance = this;
}

MovementBLEService::~MovementBLEService() {
  if (instance == this) {
    instance = nullptr;
  }
}

int MovementBLEService::init() {
  int rc;

  // Register GATT services
  rc = ble_gatts_add_svcs(movement_gatt_svcs);
  if (rc != 0) {
    return rc;
  }

  // Find the characteristic handles
  // We need to get the handles after registration
  // This would typically be done after ble_gatts_add_svcs()
  
  return 0;
}

bool MovementBLEService::sendMovementData(const MovementData& data) {
  if (connected_clients.empty()) {
    return false; // No clients to notify
  }

  auto buffer = data.serialize();
  
  // Send to all connected clients
  bool any_sent = false;
  for (uint16_t conn_handle : connected_clients) {
    int rc = sendNotificationToClient(conn_handle, buffer);
    if (rc == 0) {
      any_sent = true;
    }
  }

  return any_sent;
}

int MovementBLEService::sendNotificationToClient(uint16_t conn_handle, 
                                                  const std::array<uint8_t, 22>& data) {
  // Create an mbuf chain for the notification
  struct os_mbuf* om = ble_hs_mbuf_from_flat(data.data(), data.size());
  if (om == nullptr) {
    return BLE_HS_ENOMEM;
  }

  // Send the notification using the movement data characteristic handle
  // In a real scenario, we need to find the correct handle
  // For now, we use a placeholder. In real code, store this from GATT registration
  const uint16_t MOVEMENT_DATA_HANDLE = 2; // Example handle
  
  int rc = ble_gattc_notify_custom(conn_handle, MOVEMENT_DATA_HANDLE, om);
  
  if (rc != 0) {
    // Error sending, free the buffer
    os_mbuf_free_chain(om);
  }

  return rc;
}

void MovementBLEService::onDataReceived(DataCallback cb) {
  data_callback = cb;
}

void MovementBLEService::onConnect(uint16_t conn_handle) {
  connected_clients.push_back(conn_handle);
}

void MovementBLEService::onDisconnect(uint16_t conn_handle) {
  auto it = std::find(connected_clients.begin(), connected_clients.end(), conn_handle);
  if (it != connected_clients.end()) {
    connected_clients.erase(it);
  }
}

// ============================================================================
// GATT Server Callback Implementation
// ============================================================================

int MovementBLEService::gattServerCallback(uint16_t conn_handle, uint16_t attr_handle,
                                            struct ble_gatt_access_ctxt* ctxt, void* arg) {
  if (instance == nullptr) {
    return BLE_ATT_ERR_UNLIKELY;
  }

  // Determine which characteristic is being accessed
  // This would typically be determined by comparing attr_handle
  
  if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    // Read request - return movement data
    // For now, we return a stub response
    uint8_t stub_data[22] = {0};
    os_mbuf_append(ctxt->om, stub_data, sizeof(stub_data));
    return 0;
    
  } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    // Write request - receive command from Flutter app
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    
    if (len > 0 && len <= 22) {
      uint8_t command_data[22];
      int rc = ble_hs_mbuf_to_flat(ctxt->om, command_data, len, nullptr);
      
      if (rc == 0 && instance->data_callback) {
        // Parse and execute command
        // Example: first byte could be command ID
        // 0x01 = reset, 0x02 = get status, etc.
        
        // You can implement command handling here
      }
    }
    
    return 0;
  }

  return BLE_ATT_ERR_UNSUPPORTED_OP;
}

// ============================================================================
// GAP Event Callback Implementation
// ============================================================================

int MovementBLEService::gapEventCallback(struct ble_gap_event* event, void* arg) {
  if (instance == nullptr) {
    return 0;
  }

  switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
      // Client connected
      if (event->connect.status == 0) {
        instance->onConnect(event->connect.conn_handle);
      }
      break;

    case BLE_GAP_EVENT_DISCONNECT:
      // Client disconnected
      instance->onDisconnect(event->disconnect.conn.conn_handle);
      break;

    case BLE_GAP_EVENT_NOTIFY_TX:
      // Notification transmitted
      // Can be used for debugging or statistics
      break;

    case BLE_GAP_EVENT_SUBSCRIBE:
      // Client subscribed to notifications
      // Can track CCCD (Client Characteristic Configuration Descriptor)
      break;

    default:
      break;
  }

  return 0;
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
  int rc = ble_service.init();
  if (rc != 0) {
    // Error initializing BLE service
    return;
  }
  
  analyzer.reset();
  ble_service.onDataReceived([this](const MovementData& data) {
    // Handle received data from Flutter if needed
  });
}

void MovementService::updateAccelerometer(const std::array<float, 3>& gravity_corrected_xl,
                                          uint32_t delta_time_ms) {
  // FIX: Accumulate time in uint64_t to avoid overflow
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