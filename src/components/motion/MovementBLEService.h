#pragma once

#include "MovementAnalyzer.h"
#include <array>
#include <cstdint>
#include <functional>
#include <host/ble_gatt.h>

// ============================================================================
// UUID Definitions (InfiniTime Convention)
// ============================================================================
// Base UUID: xxxxxxxx-78fc-48fe-8e23-433b3a1942d0
// Service UUID: 00060000-78fc-48fe-8e23-433b3a1942d0 (Movement Service)
// Characteristic UUID: 00060001-78fc-48fe-8e23-433b3a1942d0
// ============================================================================

// ============================================================================
// BLE Data Structure sent to Flutter
// ============================================================================

struct MovementData {
  uint32_t timestamp_ms;           // 4 bytes
  uint32_t magnitude_active_time;  // 4 bytes
  uint32_t axis_active_time;       // 4 bytes
  uint8_t movement_detected;       // 1 byte (bool)
  uint8_t any_movement;            // 1 byte (bool)
  int16_t accel_x;                 // 2 bytes
  int16_t accel_y;                 // 2 bytes
  int16_t accel_z;                 // 2 bytes

  std::array<uint8_t, 22> serialize() const {
    std::array<uint8_t, 22> buffer;
    int offset = 0;

    buffer[offset++] = (timestamp_ms >> 0) & 0xFF;
    buffer[offset++] = (timestamp_ms >> 8) & 0xFF;
    buffer[offset++] = (timestamp_ms >> 16) & 0xFF;
    buffer[offset++] = (timestamp_ms >> 24) & 0xFF;

    buffer[offset++] = (magnitude_active_time >> 0) & 0xFF;
    buffer[offset++] = (magnitude_active_time >> 8) & 0xFF;
    buffer[offset++] = (magnitude_active_time >> 16) & 0xFF;
    buffer[offset++] = (magnitude_active_time >> 24) & 0xFF;

    buffer[offset++] = (axis_active_time >> 0) & 0xFF;
    buffer[offset++] = (axis_active_time >> 8) & 0xFF;
    buffer[offset++] = (axis_active_time >> 16) & 0xFF;
    buffer[offset++] = (axis_active_time >> 24) & 0xFF;

    buffer[offset++] = movement_detected ? 1 : 0;
    buffer[offset++] = any_movement ? 1 : 0;

    buffer[offset++] = (accel_x >> 0) & 0xFF;
    buffer[offset++] = (accel_x >> 8) & 0xFF;
    buffer[offset++] = (accel_y >> 0) & 0xFF;
    buffer[offset++] = (accel_y >> 8) & 0xFF;
    buffer[offset++] = (accel_z >> 0) & 0xFF;
    buffer[offset++] = (accel_z >> 8) & 0xFF;

    return buffer;
  }

  static MovementData deserialize(const std::array<uint8_t, 22>& buffer) {
    MovementData data;
    int offset = 0;

    data.timestamp_ms = (buffer[offset] << 0) | (buffer[offset + 1] << 8) |
                        (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
    offset += 4;

    data.magnitude_active_time = (buffer[offset] << 0) | (buffer[offset + 1] << 8) |
                                 (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
    offset += 4;

    data.axis_active_time = (buffer[offset] << 0) | (buffer[offset + 1] << 8) |
                            (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
    offset += 4;

    data.movement_detected = buffer[offset++] != 0;
    data.any_movement = buffer[offset++] != 0;

    data.accel_x = (int16_t)((buffer[offset] << 0) | (buffer[offset + 1] << 8));
    offset += 2;
    data.accel_y = (int16_t)((buffer[offset] << 0) | (buffer[offset + 1] << 8));
    offset += 2;
    data.accel_z = (int16_t)((buffer[offset] << 0) | (buffer[offset + 1] << 8));

    return data;
  }
};

// ============================================================================
// BLE Service - Full NimBLE Implementation
// ============================================================================

class MovementBLEService {
public:
  using DataCallback = std::function<void(const MovementData&)>;

  MovementBLEService();
  ~MovementBLEService() = default;
  
  // Initialize the BLE service (register with NimBLE)
  int init();
  
  // Send movement data to all connected clients
  bool sendMovementData(const MovementData& data);
  
  // Register callback for data received from Flutter
  void onDataReceived(DataCallback cb);
  
  // Connection/disconnection callbacks
  void onConnect(uint16_t conn_handle);
  void onDisconnect(uint16_t conn_handle);
  
  // Status
  bool isConnected() const;
  int getConnectedClientCount() const;

  // Friend function for callback access
  friend int movementCharacteristicCallback(uint16_t conn_handle, uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt* ctxt, void* arg);

private:
  // UUID Definitions (128-bit format for NimBLE)
  static constexpr ble_uuid128_t serviceUuid {
    .u {.type = BLE_UUID_TYPE_128},
    .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
              0xfe, 0x48, 0xfc, 0x78, 0x00, 0x00, 0x06, 0x00}
    // 00060000-78fc-48fe-8e23-433b3a1942d0
  };
  
  static constexpr ble_uuid128_t characteristicUuid {
    .u {.type = BLE_UUID_TYPE_128},
    .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
              0xfe, 0x48, 0xfc, 0x78, 0x01, 0x00, 0x06, 0x00}
    // 00060001-78fc-48fe-8e23-433b3a1942d0
  };
  
  // GATT definitions
  ble_gatt_chr_def characteristicDefinition[2];
  ble_gatt_svc_def serviceDefinition[2];
  
  // Characteristic handle (for sending notifications)
  uint16_t movementCharacteristicHandle = 0;
  
  // Callbacks
  DataCallback dataReceivedCallback;
  
  // Connected clients tracking
  int connectedClientCount = 0;
  
  // Internal callback handler
  int handleCharacteristicAccess(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt* ctxt);
};

// ============================================================================
// High-level Movement Service Integration (Singleton)
// ============================================================================

class MovementService {
public:
  static MovementService& getInstance();
  
  void init();
  
  void updateAccelerometer(const std::array<float, 3>& gravity_corrected_xl, 
                          uint32_t delta_time_ms);
  
  struct CurrentStatus {
    uint32_t magnitude_active_time;
    uint32_t axis_active_time;
    bool movement_detected;
    bool any_movement;
    float progress_percent;
    float accel_x;
    float accel_y;
    float accel_z;
  };
  
  CurrentStatus getCurrentStatus() const;
  void resetStatistics();
  void onBleConnect(uint16_t conn_handle);
  void onBleDisconnect(uint16_t conn_handle);

private:
  MovementService() = default;
  MovementService(const MovementService&) = delete;
  MovementService& operator=(const MovementService&) = delete;

  MovementAnalyzer analyzer;
  MovementBLEService ble_service;
  CurrentStatus current_status = {0, 0, false, false, 0.0f};
  uint64_t system_time_ms = 0;
};