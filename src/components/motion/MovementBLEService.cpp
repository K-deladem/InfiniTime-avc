#include "MovementBLEService.h"
#include <algorithm>

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