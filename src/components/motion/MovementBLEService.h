#pragma once

#include "MovementAnalyzer.h"
#include <array>
#include <cstdint>
#include <functional>

// 🔧 FIX: Minimal NimBLE headers - moved to .cpp only
// This prevents std::min conflicts in header chain

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
// Minimal BLE Service - Stub Implementation
// ============================================================================

class MovementBLEService {
public:
  using DataCallback = std::function<void(const MovementData&)>;

  MovementBLEService() = default;
  ~MovementBLEService() = default;
  
  int init() { return 0; }  // Stub - implement if needed
  
  bool sendMovementData(const MovementData& data) {
    (void)data;  // Avoid unused warning
    return true;  // Stub implementation
  }
  
  void onDataReceived(DataCallback cb) {
    (void)cb;  // Avoid unused warning
  }
  
  void onConnect(uint16_t conn_handle) {
    (void)conn_handle;
  }
  
  void onDisconnect(uint16_t conn_handle) {
    (void)conn_handle;
  }
  
  bool isConnected() const { return false; }
  int getConnectedClientCount() const { return 0; }
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