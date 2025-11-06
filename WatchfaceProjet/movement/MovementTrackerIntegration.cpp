#include "MovementTrackerIntegration.h"
#include "controllers/MotionController.h"
#include "drivers/Bma421.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ============================================================================
// MovementTracker Implementation (Singleton)
// ============================================================================

namespace Pinetime::Controllers {

MovementTracker& MovementTracker::getInstance() {
  static MovementTracker instance;
  return instance;
}

void MovementTracker::init() {
  movement_service.init();
  last_update_ms = 0;
  initialized = true;
}

void MovementTracker::handleMotionDataRaw(int16_t x, int16_t y, int16_t z, uint32_t delta_time_ms) {
  // Validation basique
  if (!initialized || delta_time_ms == 0) {
    return;
  }

  // Les données du BMA421 sont en "binary milli-g units"
  // où 1g = 1024 unités (voir BMA421 scale factors dans Bma421.cpp)
  // On divise par 1024.0f pour convertir en g (unités gravitationnelles)
  constexpr float ACCEL_SCALE = 1.0f / 1024.0f;  // Conversion de binary milli-g à g

  // Récupérer les valeurs d'accélération et convertir en float
  float accel_x = static_cast<float>(x) * ACCEL_SCALE;
  float accel_y = static_cast<float>(y) * ACCEL_SCALE;
  float accel_z = static_cast<float>(z) * ACCEL_SCALE;

  // Créer le tableau d'accélération (gravité corrigée)
  std::array<float, 3> gravity_corrected_xl = {accel_x, accel_y, accel_z};

  // Mettre à jour le service de mouvement avec les nouvelles données
  movement_service.updateAccelerometer(gravity_corrected_xl, delta_time_ms);
}

void MovementTracker::handleMotionData(const Pinetime::Controllers::MotionController& motion_controller,
                                       uint32_t delta_time_ms) {
  // Validation
  if (!initialized || delta_time_ms == 0) {
    return;
  }

  // Extraire les données d'accélération depuis le MotionController
  // Le MotionController stocke les données dans sa structure interne
  // Cette approche permet de récupérer les données directement depuis la source
  
  // Note: Cette fonction dépend de l'interface publique de MotionController
  // Si MotionController n'a pas de méthode publique pour accéder aux données,
  // utilisez plutôt handleMotionDataRaw() appelée directement de la motion task

  // Exemple (à adapter selon l'API réelle de MotionController):
  // int16_t x, y, z;
  // motion_controller.getAccelerometerData(x, y, z);
  // handleMotionDataRaw(x, y, z, delta_time_ms);
}

MovementService::CurrentStatus MovementTracker::getStatus() const {
  return movement_service.getCurrentStatus();
}

void MovementTracker::reset() {
  movement_service.resetStatistics();
  last_update_ms = 0;
}

} // namespace Pinetime::Controllers

// ============================================================================
// MovementWatchFaceHelper Implementation (Singleton)
// ============================================================================

namespace Pinetime::Drivers {

MovementWatchFaceHelper& MovementWatchFaceHelper::getInstance() {
  static MovementWatchFaceHelper instance;
  return instance;
}

MovementDisplayData MovementWatchFaceHelper::getDisplayData() const {
  auto status = Pinetime::Controllers::MovementTracker::getInstance().getStatus();

  MovementDisplayData display_data;
  display_data.active_time_seconds = status.axis_active_time / 1000;
  display_data.progress_percent = static_cast<uint8_t>(status.progress_percent);
  display_data.is_moving = status.any_movement;

  // Format active time
  formatActiveTime(status.axis_active_time, display_data.time_str, sizeof(display_data.time_str));

  // Format progress string
  uint32_t minutes = display_data.active_time_seconds / 60;
  uint32_t seconds = display_data.active_time_seconds % 60;
  std::snprintf(display_data.progress_str, sizeof(display_data.progress_str),
                "%lu:%02lu min", minutes, seconds);

  return display_data;
}

uint8_t MovementWatchFaceHelper::getProgressBarLevel() const {
  auto status = Pinetime::Controllers::MovementTracker::getInstance().getStatus();
  // Convert 0-100 percent to 0-255 range
  return static_cast<uint8_t>((status.progress_percent / 100.0f) * 255.0f);
}

bool MovementWatchFaceHelper::shouldShowActivityIndicator() const {
  auto status = Pinetime::Controllers::MovementTracker::getInstance().getStatus();
  return status.any_movement;
}

void MovementWatchFaceHelper::formatActiveTime(uint32_t ms, char* buffer, size_t size) {
  if (!buffer || size < 16) {
    return;
  }

  uint32_t total_seconds = ms / 1000;
  uint32_t hours = total_seconds / 3600;
  uint32_t minutes = (total_seconds % 3600) / 60;
  uint32_t seconds = total_seconds % 60;

  std::snprintf(buffer, size, "%02lu:%02lu:%02lu", hours, minutes, seconds);
}

} // namespace Pinetime::Drivers

// ============================================================================
// MovementSystemService Implementation (Singleton)
// ============================================================================

namespace Pinetime::System {

MovementSystemService& MovementSystemService::getInstance() {
  static MovementSystemService instance;
  return instance;
}

void MovementSystemService::init() {
  Pinetime::Controllers::MovementTracker::getInstance().init();
}

void MovementSystemService::update() {
  // Cette fonction est appelée périodiquement depuis la motion task
  // ou le système de timers. Elle peut être utilisée pour des tâches de maintenance
}

void MovementSystemService::onBleConnect(uint16_t conn_handle) {
  // BLE connecté - le service peut maintenant envoyer des données
  auto& ble_service = Pinetime::Controllers::MovementTracker::getInstance().getStatus();
  // Les notifications BLE seront automatiquement envoyées depuis MovementService
}

void MovementSystemService::onBleDisconnect(uint16_t conn_handle) {
  // BLE déconnecté
}

void MovementSystemService::setBatterySavingMode(bool enabled) {
  battery_saving_mode = enabled;

  if (enabled) {
    update_interval_ms = 500;  // Reduce update frequency to 500ms
  } else {
    update_interval_ms = 100;  // Normal: 100ms
  }
}

} // namespace Pinetime::System