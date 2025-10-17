#include "displayapp/screens/WatchFaceRehabilitationCasio.h"
#include "displayapp/Colors.h"
#include "displayapp/DisplayApp.h"
#include "systemtask/SystemTask.h"  // ← AJOUTER
#include "components/ble/NimbleController.h"  // ← AJOUTER
#include <cmath>
#include <cstdio>
#include <nrf_log.h>  // ← AJOUTER

using namespace Pinetime::Applications::Screens;

WatchFaceRehabilitationCasio::WatchFaceRehabilitationCasio(
    Controllers::DateTime& dateTimeController,
    const Controllers::Battery& batteryController,
    const Controllers::Ble& bleController,
    Controllers::NotificationManager& notificationManager,
    Controllers::Settings& settingsController,
    Controllers::MotionController& motionController,
    Pinetime::System::SystemTask& systemTask)  // ← AJOUTER
  : dateTimeController{dateTimeController},
    batteryController{batteryController},
    bleController{bleController},
    notificationManager{notificationManager},
    settingsController{settingsController},
    motionController{motionController},
    systemTask{systemTask},  // ← AJOUTER
    algorithm{} {
  
  // ===== UI SETUP (reste identique) =====
  timeDisplay = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(timeDisplay, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &open_sans_light);
  lv_obj_set_style_local_text_color(timeDisplay, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_align(timeDisplay, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 20);

  scoreDisplay = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(scoreDisplay, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &open_sans_light);
  lv_obj_set_style_local_text_color(scoreDisplay, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_obj_align(scoreDisplay, lv_scr_act(), LV_ALIGN_CENTER, 0, -30);

  progressBar = lv_bar_create(lv_scr_act(), nullptr);
  lv_obj_set_width(progressBar, 200);
  lv_obj_set_height(progressBar, 20);
  lv_bar_set_range(progressBar, 0, 100);
  lv_bar_set_value(progressBar, 0, LV_ANIM_OFF);
  lv_obj_set_style_local_bg_color(progressBar, LV_BAR_PART_BG, LV_STATE_DEFAULT, LV_COLOR_GRAY);
  lv_obj_set_style_local_bg_color(progressBar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_obj_align(progressBar, lv_scr_act(), LV_ALIGN_CENTER, 0, 10);

  symmetryLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(symmetryLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &open_sans_light);
  lv_obj_set_style_local_text_color(symmetryLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_align(symmetryLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, 40);

  statusIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(statusIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &open_sans_light);
  lv_obj_set_style_local_text_color(statusIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_align(statusIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_MID, 0, -20);
}

WatchFaceRehabilitationCasio::~WatchFaceRehabilitationCasio() {
  lv_obj_clean(lv_scr_act());
}

void WatchFaceRehabilitationCasio::Refresh() {
  UpdateTime();
  UpdateMotionData();
  UpdateUI();
  SendDataToMobile();
}

void WatchFaceRehabilitationCasio::UpdateTime() {
  auto now = dateTimeController.CurrentDateTime();
  
  auto hour = std::chrono::time_point_cast<std::chrono::hours>(now);
  auto day = std::chrono::time_point_cast<std::chrono::days>(now);
  
  auto hours_remaining = now - day;
  auto minutes_remaining = hours_remaining - std::chrono::duration_cast<std::chrono::hours>(hours_remaining);
  
  uint8_t h = hour.time_since_epoch().count() % 24;
  uint8_t m = std::chrono::duration_cast<std::chrono::minutes>(minutes_remaining).count();
  
  char timeBuffer[10];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02u:%02u", h, m);
  
  lv_label_set_text(timeDisplay, timeBuffer);
}

void WatchFaceRehabilitationCasio::UpdateMotionData() {
  int16_t x = motionController.X();
  int16_t y = motionController.Y();
  int16_t z = motionController.Z();
  
  RehabilitationAlgorithm::SensorData sensorData;
  sensorData.accelX = static_cast<float>(x);
  sensorData.accelY = static_cast<float>(y);
  sensorData.accelZ = static_cast<float>(z);
  sensorData.gyroX = 0.0f;
  sensorData.gyroY = 0.0f;
  sensorData.gyroZ = 0.0f;
  sensorData.heartRate = 0;
  sensorData.timestamp = xTaskGetTickCount();
  
  RehabilitationAlgorithm::Result result = algorithm.Calculate(sensorData);
  
  rehabilitationScore = result.score;
  movementQuality = result.movementQuality;
  movementSymmetry = result.symmetry;
  isActiveMovement = result.isActiveMovement;
}

void WatchFaceRehabilitationCasio::UpdateUI() {
  char scoreBuffer[30];
  snprintf(scoreBuffer, sizeof(scoreBuffer), "Rehab: %u%%", rehabilitationScore);
  lv_label_set_text(scoreDisplay, scoreBuffer);
  
  lv_bar_set_value(progressBar, rehabilitationScore, LV_ANIM_ON);
  
  char symmetryBuffer[30];
  snprintf(symmetryBuffer, sizeof(symmetryBuffer), "Sym: %u%%", movementSymmetry);
  lv_label_set_text(symmetryLabel, symmetryBuffer);
  
  const char* icon = bleController.IsConnected() ? "✓ BLE" : "✗ BLE";
  lv_label_set_text(statusIcon, icon);
}

// VRAIE IMPLÉMENTATION BLE
void WatchFaceRehabilitationCasio::SendDataToMobile() {
  // Vérifier que BLE est connecté
  if (!bleController.IsConnected()) {
    return;
  }

  // Accéder au NimbleController via SystemTask
  auto& nimbleController = systemTask.nimble();
  
  // Envoyer les données via le service BLE
  nimbleController.rehab().OnNewRehabData(
    rehabilitationScore,
    movementQuality,
    movementSymmetry
  );
  
  NRF_LOG_INFO("Rehab BLE sent - Score: %u%%, Movement: %u, Symmetry: %u%%",
               rehabilitationScore, movementQuality, movementSymmetry);
}

bool WatchFaceRehabilitationCasio::OnButtonPushed() {
  return true;
}