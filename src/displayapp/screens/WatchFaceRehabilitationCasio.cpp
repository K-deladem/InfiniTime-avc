#include "displayapp/screens/WatchFaceRehabilitationCasio.h"
#include "displayapp/Colors.h"
#include "displayapp/DisplayApp.h"
#include "systemtask/SystemTask.h"
#include "components/ble/NimbleController.h"
#include <cmath>
#include <cstdio>
#include <nrf_log.h>

using namespace Pinetime::Applications::Screens;

// ===== CONSTANTES =====
namespace {
  constexpr uint32_t MOTION_UPDATE_INTERVAL_MS = 100;   // 10 Hz pour les capteurs
  constexpr uint32_t BLE_UPDATE_INTERVAL_MS = 1000;     // 1 Hz pour BLE
  constexpr int16_t SENSOR_MAX_VALUE = 32000;           // Plage valide accéléromètre
  constexpr uint8_t SCORE_CHANGE_THRESHOLD = 1;         // Seuil pour éviter envois répétés
}

WatchFaceRehabilitationCasio::WatchFaceRehabilitationCasio(
    Controllers::DateTime& dateTimeController,
    const Controllers::Battery& batteryController,
    const Controllers::Ble& bleController,
    Controllers::NotificationManager& notificationManager,
    Controllers::Settings& settingsController,
    Controllers::MotionController& motionController,
    Pinetime::System::SystemTask& systemTask)
  : dateTimeController{dateTimeController},
    batteryController{batteryController},
    bleController{bleController},
    notificationManager{notificationManager},
    settingsController{settingsController},
    motionController{motionController},
    systemTask{systemTask},
    algorithm{} {
  
  // ===== INITIALISATION DES STYLES =====
  
  // Style pour l'heure
  lv_style_init(&time_style);
  lv_style_set_text_font(&time_style, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_style_set_text_color(&time_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  
  // Style pour le score
  lv_style_init(&score_style);
  lv_style_set_text_font(&score_style, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_style_set_text_color(&score_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  
  // Style pour la symétrie
  lv_style_init(&symmetry_style);
  lv_style_set_text_font(&symmetry_style, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_style_set_text_color(&symmetry_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  
  // Style pour le statut
  lv_style_init(&status_style);
  lv_style_set_text_font(&status_style, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_style_set_text_color(&status_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  
  // Style pour la barre de progression (fond)
  lv_style_init(&bar_bg_style);
  lv_style_set_bg_color(&bar_bg_style, LV_STATE_DEFAULT, LV_COLOR_GRAY);
  
  // Style pour la barre de progression (indicateur)
  lv_style_init(&bar_indic_style);
  lv_style_set_bg_color(&bar_indic_style, LV_STATE_DEFAULT, LV_COLOR_LIME);

  // ===== CRÉATION DES ÉLÉMENTS UI =====
  
  // Affichage de l'heure
  timeDisplay = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_add_style(timeDisplay, LV_LABEL_PART_MAIN, &time_style);
  lv_obj_align(timeDisplay, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 20);

  // Affichage du score
  scoreDisplay = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_add_style(scoreDisplay, LV_LABEL_PART_MAIN, &score_style);
  lv_obj_align(scoreDisplay, lv_scr_act(), LV_ALIGN_CENTER, 0, -30);

  // Barre de progression
  progressBar = lv_bar_create(lv_scr_act(), nullptr);
  lv_obj_set_width(progressBar, 200);
  lv_obj_set_height(progressBar, 20);
  lv_bar_set_range(progressBar, 0, 100);
  lv_bar_set_value(progressBar, 0, LV_ANIM_OFF);
  lv_obj_add_style(progressBar, LV_BAR_PART_BG, &bar_bg_style);
  lv_obj_add_style(progressBar, LV_BAR_PART_INDIC, &bar_indic_style);
  lv_obj_align(progressBar, lv_scr_act(), LV_ALIGN_CENTER, 0, 10);

  // Label de symétrie
  symmetryLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_add_style(symmetryLabel, LV_LABEL_PART_MAIN, &symmetry_style);
  lv_obj_align(symmetryLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, 40);

  // Icône de statut
  statusIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_add_style(statusIcon, LV_LABEL_PART_MAIN, &status_style);
  lv_obj_align(statusIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_MID, 0, -20);

  NRF_LOG_INFO("WatchFaceRehabilitationCasio initialized");
}

WatchFaceRehabilitationCasio::~WatchFaceRehabilitationCasio() {
  // ===== NETTOYAGE DES STYLES =====
  lv_style_reset(&time_style);
  lv_style_reset(&score_style);
  lv_style_reset(&symmetry_style);
  lv_style_reset(&status_style);
  lv_style_reset(&bar_bg_style);
  lv_style_reset(&bar_indic_style);
  
  // ===== NETTOYAGE DE L'ÉCRAN =====
  lv_obj_clean(lv_scr_act());
  
  NRF_LOG_INFO("WatchFaceRehabilitationCasio destroyed");
}

void WatchFaceRehabilitationCasio::Refresh() {
  // Toujours mettre à jour l'heure
  UpdateTime();
  
  // Obtenir le temps actuel pour le throttling
  uint32_t now = xTaskGetTickCount();
  
  // ===== MISE À JOUR DES CAPTEURS (100ms) =====
  if (now - lastMotionUpdate >= pdMS_TO_TICKS(MOTION_UPDATE_INTERVAL_MS)) {
    UpdateMotionData();
    lastMotionUpdate = now;
  }
  
  // Toujours mettre à jour l'UI (léger, pas de problème)
  UpdateUI();
  
  // ===== ENVOI BLE (1s) =====
  if (now - lastBleUpdate >= pdMS_TO_TICKS(BLE_UPDATE_INTERVAL_MS)) {
    SendDataToMobile();
    lastBleUpdate = now;
  }
}

void WatchFaceRehabilitationCasio::UpdateTime() {
  // Utiliser les méthodes directes du contrôleur
  uint8_t h = dateTimeController.Hours();
  uint8_t m = dateTimeController.Minutes();
  
  char timeBuffer[10];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02u:%02u", h, m);
  lv_label_set_text(timeDisplay, timeBuffer);
}

bool WatchFaceRehabilitationCasio::ValidateSensorData(int16_t x, int16_t y, int16_t z) {
  // Vérifier si tous les capteurs sont à zéro (pas de données)
  if (x == 0 && y == 0 && z == 0) {
    NRF_LOG_WARNING("No sensor data available");
    return false;
  }
  
  // Vérifier la plage des valeurs
  if (abs(x) > SENSOR_MAX_VALUE || 
      abs(y) > SENSOR_MAX_VALUE || 
      abs(z) > SENSOR_MAX_VALUE) {
    NRF_LOG_WARNING("Sensor data out of range: x=%d, y=%d, z=%d", x, y, z);
    return false;
  }
  
  return true;
}

void WatchFaceRehabilitationCasio::UpdateMotionData() {
  // Lire les données des capteurs
  int16_t x = motionController.X();
  int16_t y = motionController.Y();
  int16_t z = motionController.Z();
  
  // Valider les données
  if (!ValidateSensorData(x, y, z)) {
    return;
  }
  
  // Préparer les données pour l'algorithme
  RehabilitationAlgorithm::SensorData sensorData;
  sensorData.accelX = static_cast<float>(x);
  sensorData.accelY = static_cast<float>(y);
  sensorData.accelZ = static_cast<float>(z);
  sensorData.gyroX = 0.0f;
  sensorData.gyroY = 0.0f;
  sensorData.gyroZ = 0.0f;
  sensorData.heartRate = 0;
  sensorData.timestamp = xTaskGetTickCount();
  
  // Calculer les métriques de réadaptation
  RehabilitationAlgorithm::Result result = algorithm.Calculate(sensorData);
  
  // Mettre à jour les variables membres
  rehabilitationScore = result.score;
  movementQuality = result.movementQuality;
  movementSymmetry = result.symmetry;
  isActiveMovement = result.isActiveMovement;
  
  // Log pour debug (optionnel, à retirer en production)
  #ifdef DEBUG
  NRF_LOG_DEBUG("Motion updated - Score: %u%%, Quality: %u, Symmetry: %u%%",
                rehabilitationScore, movementQuality, movementSymmetry);
  #endif
}

void WatchFaceRehabilitationCasio::UpdateUI() {
  // Mise à jour du score
  char scoreBuffer[30];
  snprintf(scoreBuffer, sizeof(scoreBuffer), "Rehab: %u%%", rehabilitationScore);
  lv_label_set_text(scoreDisplay, scoreBuffer);
  
  // Mise à jour de la barre de progression avec animation
  lv_bar_set_value(progressBar, rehabilitationScore, LV_ANIM_ON);
  
  // Mise à jour de la symétrie
  char symmetryBuffer[30];
  snprintf(symmetryBuffer, sizeof(symmetryBuffer), "Sym: %u%%", movementSymmetry);
  lv_label_set_text(symmetryLabel, symmetryBuffer);
  
  // Mise à jour du statut BLE
  const char* icon = bleController.IsConnected() ? "✓ BLE" : "✗ BLE";
  lv_label_set_text(statusIcon, icon);
  
  // Changer la couleur du score selon le niveau
  lv_color_t scoreColor;
  if (rehabilitationScore >= 80) {
    scoreColor = LV_COLOR_LIME;  // Excellent
  } else if (rehabilitationScore >= 60) {
    scoreColor = LV_COLOR_YELLOW;  // Bon
  } else if (rehabilitationScore >= 40) {
    scoreColor = LV_COLOR_ORANGE;  // Moyen
  } else {
    scoreColor = LV_COLOR_RED;  // Faible
  }
  lv_style_set_text_color(&score_style, LV_STATE_DEFAULT, scoreColor);
}

void WatchFaceRehabilitationCasio::SendDataToMobile() {
  // Vérifier que BLE est connecté
  if (!bleController.IsConnected()) {
    return;
  }
  
  // Éviter d'envoyer si les données n'ont pas significativement changé
  if (abs(static_cast<int>(rehabilitationScore) - static_cast<int>(lastSentScore)) < SCORE_CHANGE_THRESHOLD &&
      abs(static_cast<int>(movementSymmetry) - static_cast<int>(lastSentSymmetry)) < SCORE_CHANGE_THRESHOLD) {
    return;  // Pas de changement significatif
  }
  
  // Mettre à jour les dernières valeurs envoyées
  lastSentScore = rehabilitationScore;
  lastSentSymmetry = movementSymmetry;
  
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
  // Gérer l'appui sur le bouton (pour navigation, etc.)
  return true;
}