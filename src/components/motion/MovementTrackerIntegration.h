#pragma once

#include "MovementBLEService.h"
#include <array>
#include <cstdint>

// Forward declarations
namespace Pinetime::Controllers {
class MotionController;
}

// ============================================================================
// Motion Task Integration (runs independently)
// ============================================================================

namespace Pinetime::Controllers {

class MovementTracker {
public:
  static MovementTracker& getInstance();

  // Recevoir directement les données brutes du capteur
  // Cette signature évite le problème "type incomplet"
  // Appelée depuis la motion task avec les données d'accélération
  void handleMotionDataRaw(int16_t x, int16_t y, int16_t z, uint32_t delta_time_ms);

  // Alternative avec pointeur opaque (si vous devez passer MotionController)
  // Cette version utilise un pointeur void pour éviter l'inclusion de l'en-tête
  void handleMotionDataViaPointer(void* motion_controller_ptr);

  // Get current status for any watchface
  MovementService::CurrentStatus getStatus() const;

  // Initialize on system startup (MovementService is already initialized by NimbleController)
  void init();

  // Reset statistics (button press, app command)
  void reset();

private:
  MovementTracker() = default;
  MovementTracker(const MovementTracker&) = delete;
  MovementTracker& operator=(const MovementTracker&) = delete;

  uint32_t last_update_ms = 0;
};

} // namespace Pinetime::Controllers

// ============================================================================
// Watchface Integration (generic, works for any watchface)
// ============================================================================

namespace Pinetime::Drivers {

struct MovementDisplayData {
  uint32_t active_time_seconds;
  uint8_t progress_percent; // 0-100
  bool is_moving;
  char time_str[16];      // "HH:MM:SS"
  char progress_str[32];  // "12.5 min" or similar
};

class MovementWatchFaceHelper {
public:
  static MovementWatchFaceHelper& getInstance();

  // Format data for display
  MovementDisplayData getDisplayData() const;

  // Get progress bar fill level (0-255 for 8-bit display)
  uint8_t getProgressBarLevel() const;

  // Get activity indicator (blinking effect when moving)
  bool shouldShowActivityIndicator() const;

private:
  MovementWatchFaceHelper() = default;

  // Format time from milliseconds
  static void formatActiveTime(uint32_t ms, char* buffer, size_t size);
};

} // namespace Pinetime::Drivers

// ============================================================================
// System Service (starts on boot, manages background updates)
// ============================================================================

namespace Pinetime::System {

class MovementSystemService {
public:
  static MovementSystemService& getInstance();

  // Initialize on system startup
  void init();

  // Called periodically by system timer (keep running even in sleep)
  void update();

  // BLE connection callbacks
  void onBleConnect();
  void onBleDisconnect();

  // Battery saving mode (reduce update frequency)
  void setBatterySavingMode(bool enabled);

  bool isBatterySavingMode() const { return battery_saving_mode; }

private:
  MovementSystemService() = default;
  MovementSystemService(const MovementSystemService&) = delete;
  MovementSystemService& operator=(const MovementSystemService&) = delete;

  bool battery_saving_mode = false;
  uint32_t update_interval_ms = 100; // Normal: 100ms
};

} // namespace Pinetime::System