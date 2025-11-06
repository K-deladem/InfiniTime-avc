#pragma once

#include "MovementAnalyzer.h"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

// NimBLE includes for InfiniTime
#include <host/ble_gatt.h>
#include <host/ble_gap.h>

// ============================================================================
// BLE Data Structure sent to Flutter
// ============================================================================

struct MovementData {
  // Packet structure (22 bytes typical BLE MTU)
  uint32_t timestamp_ms;           // 4 bytes
  uint32_t magnitude_active_time;  // 4 bytes
  uint32_t axis_active_time;       // 4 bytes
  uint8_t movement_detected;       // 1 byte (bool)
  uint8_t any_movement;            // 1 byte (bool)
  int16_t accel_x;                 // 2 bytes (raw acceleration * 100)
  int16_t accel_y;                 // 2 bytes
  int16_t accel_z;                 // 2 bytes
  // Total: 22 bytes (fits in typical BLE packet)

  // Serialize to byte array for transmission
  std::array<uint8_t, 22> serialize() const {
    std::array<uint8_t, 22> buffer;
    int offset = 0;

    // Timestamp (little-endian)
    buffer[offset++] = (timestamp_ms >> 0) & 0xFF;
    buffer[offset++] = (timestamp_ms >> 8) & 0xFF;
    buffer[offset++] = (timestamp_ms >> 16) & 0xFF;
    buffer[offset++] = (timestamp_ms >> 24) & 0xFF;

    // Magnitude active time
    buffer[offset++] = (magnitude_active_time >> 0) & 0xFF;
    buffer[offset++] = (magnitude_active_time >> 8) & 0xFF;
    buffer[offset++] = (magnitude_active_time >> 16) & 0xFF;
    buffer[offset++] = (magnitude_active_time >> 24) & 0xFF;

    // Axis active time
    buffer[offset++] = (axis_active_time >> 0) & 0xFF;
    buffer[offset++] = (axis_active_time >> 8) & 0xFF;
    buffer[offset++] = (axis_active_time >> 16) & 0xFF;
    buffer[offset++] = (axis_active_time >> 24) & 0xFF;

    // Booleans
    buffer[offset++] = movement_detected ? 1 : 0;
    buffer[offset++] = any_movement ? 1 : 0;

    // Acceleration (little-endian)
    buffer[offset++] = (accel_x >> 0) & 0xFF;
    buffer[offset++] = (accel_x >> 8) & 0xFF;
    buffer[offset++] = (accel_y >> 0) & 0xFF;
    buffer[offset++] = (accel_y >> 8) & 0xFF;
    buffer[offset++] = (accel_z >> 0) & 0xFF;
    buffer[offset++] = (accel_z >> 8) & 0xFF;

    return buffer;
  }

  // Deserialize from byte array (for debugging)
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
// BLE Service Definition for InfiniTime with Real NimBLE Implementation
// ============================================================================

/*
 * Custom BLE Service for Movement Analysis
 * 
 * Service UUID: 12345678-1234-5678-1234-56789abcdef0
 * Characteristic UUIDs:
 *   - Movement Data (Notify): 87654321-4321-8765-4321-0fedcba98765
 *   - Command (Write): 11111111-1111-1111-1111-111111111111
 */

class MovementBLEService {
public:
  using DataCallback = std::function<void(const MovementData&)>;

  MovementBLEService();
  ~MovementBLEService();
  
  // Initialize BLE service (call from NimBLE initialization)
  // Returns 0 on success, non-zero on error
  int init();
  
  // Called by InfiniTime when data is ready to send (sends via notifications)
  // Returns true if data was queued/sent successfully
  bool sendMovementData(const MovementData& data);
  
  // Register callback for received commands from Flutter
  void onDataReceived(DataCallback callback);
  
  // BLE connection events
  void onConnect(uint16_t conn_handle);
  void onDisconnect(uint16_t conn_handle);
  bool isConnected() const { return !connected_clients.empty(); }
  int getConnectedClientCount() const { return connected_clients.size(); }

private:
  // NimBLE callback wrappers (static, called by C-style BLE stack)
  static int gattServerCallback(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt* ctxt, void* arg);
  
  // Static callback for gap events
  static int gapEventCallback(struct ble_gap_event* event, void* arg);
  
  // Helper to send notification to a specific client
  int sendNotificationToClient(uint16_t conn_handle, const std::array<uint8_t, 22>& data);

  // Track connected clients
  std::vector<uint16_t> connected_clients;
  
  // GATT attribute handles
  uint16_t movement_data_char_handle;
  uint16_t command_char_handle;
  
  // Callbacks
  DataCallback data_callback;
  
  // BLE service UUIDs (128-bit)
  static const ble_uuid128_t service_uuid;
  static const ble_uuid128_t movement_data_uuid;
  static const ble_uuid128_t command_uuid;
  
  // Singleton instance for callbacks
  static MovementBLEService* instance;
};

// ============================================================================
// High-level Movement Service Integration (Singleton)
// ============================================================================

class MovementService {
public:
  static MovementService& getInstance();
  
  // Initialize the service (call during InfiniTime startup)
  void init();
  
  // Update with accelerometer data (call from motion task)
  void updateAccelerometer(const std::array<float, 3>& gravity_corrected_xl, uint32_t delta_time_ms);
  
  // Get current analysis result
  struct CurrentStatus {
    uint32_t magnitude_active_time;
    uint32_t axis_active_time;
    bool movement_detected;
    bool any_movement;
    float progress_percent; // 0-100 for progress bar
  };
  
  CurrentStatus getCurrentStatus() const;
  
  // Reset statistics
  void resetStatistics();
  
  // BLE events (called from application BLE callbacks)
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