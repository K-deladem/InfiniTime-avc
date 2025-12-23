#pragma once

#include "MovementAnalyzer.h"
#include <array>
#include <atomic>
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

// Forward declaration
namespace Pinetime::Controllers {
  class NimbleController;
}

// ============================================================================
// BLE Service - Full NimBLE Implementation (like MotionService)
// ============================================================================

class MovementBLEService {
public:
  MovementBLEService(Pinetime::Controllers::NimbleController& nimble);
  ~MovementBLEService() = default;

  // Initialize the BLE service (register with NimBLE)
  void Init();

  // Send movement data to connected client
  bool sendMovementData(const MovementData& data);

  // Notification subscription (called by NimbleController)
  void SubscribeNotification(uint16_t attributeHandle);
  void UnsubscribeNotification(uint16_t attributeHandle);

  // Get characteristic handle for subscription matching
  uint16_t GetCharacteristicHandle() const { return movementCharacteristicHandle; }

  // Callback for characteristic access
  int OnCharacteristicAccess(uint16_t attributeHandle, ble_gatt_access_ctxt* context);

private:
  Pinetime::Controllers::NimbleController& nimble;

  // GATT definitions
  ble_gatt_chr_def characteristicDefinition[2];
  ble_gatt_svc_def serviceDefinition[2];

  // Characteristic handle (for sending notifications)
  uint16_t movementCharacteristicHandle = 0;

  // Notification enabled flag (atomic for thread safety)
  std::atomic_bool notificationEnabled {false};

  // Last data for read requests
  MovementData lastData {};
};

// ============================================================================
// High-level Movement Service Integration (Singleton)
// ============================================================================

class MovementService {
public:
  // Get singleton instance
  static MovementService& getInstance();

  // Initialize with BLE service reference (called from NimbleController)
  void init(MovementBLEService& bleService);

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

  // Get BLE service for NimbleController integration
  MovementBLEService* getBLEService() { return ble_service; }

private:
  MovementService() = default;
  MovementService(const MovementService&) = delete;
  MovementService& operator=(const MovementService&) = delete;

  MovementAnalyzer analyzer;
  MovementBLEService* ble_service = nullptr;
  CurrentStatus current_status = {0, 0, false, false, 0.0f};
  uint64_t system_time_ms = 0;
};
