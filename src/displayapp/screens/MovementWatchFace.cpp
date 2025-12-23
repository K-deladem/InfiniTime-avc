// ============================================================================
// MovementWatchFace.cpp - Watchface Movement avec style Casio G7710
// ============================================================================

#include "MovementWatchFace.h"
#include <cstdio>
#include <ctime>
#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;

// ============================================================================
// Constructeur
// ============================================================================

MovementWatchFace::MovementWatchFace(Controllers::DateTime& dateTimeController,
                                     const Controllers::Battery& batteryController,
                                     Controllers::MotionController& motionController,
                                     const Controllers::Ble& bleController)
    : dateTimeController(dateTimeController),
      batteryController(batteryController),
      motionController(motionController),
      bleController(bleController) {
  CreateUI();
  Refresh();
}

// ============================================================================
// Destructeur
// ============================================================================

MovementWatchFace::~MovementWatchFace() {
  lv_task_del(taskRefresh);
  lv_style_reset(&style_line);
  lv_obj_clean(lv_scr_act());
}

// ============================================================================
// Création de l'UI
// ============================================================================

void MovementWatchFace::CreateUI() {
  // Couleur du texte (vert Casio)
  lv_color_t color_text = lv_color_hex(0x98B69A);
  lv_color_t color_active = lv_color_hex(0x00FF00);

  // ========================================================================
  // Ligne du haut: Date et Indicateur
  // ========================================================================
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 5, 5);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text(label_date, "01-01");

  movement_indicator = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(movement_indicator, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -5, 5);
  lv_obj_set_style_local_text_color(movement_indicator, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text(movement_indicator, "IDLE");

  // ========================================================================
  // Style des lignes (comme Casio)
  // ========================================================================
  lv_style_init(&style_line);
  lv_style_set_line_width(&style_line, LV_STATE_DEFAULT, 2);
  lv_style_set_line_color(&style_line, LV_STATE_DEFAULT, color_text);
  lv_style_set_line_rounded(&style_line, LV_STATE_DEFAULT, true);

  // Ligne horizontale sous la date
  line_top_points[0] = {0, 25};
  line_top_points[1] = {240, 25};
  line_top = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_top, line_top_points, 2);
  lv_obj_add_style(line_top, LV_LINE_PART_MAIN, &style_line);

  // ========================================================================
  // Zone centrale: HEURE PRINCIPALE (police digitale professionnelle)
  // ========================================================================
  time_label = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_label_set_text(time_label, "00:00");
  lv_obj_align(time_label, lv_scr_act(), LV_ALIGN_CENTER, 0, -30);

  // Secondes (même police, taille cohérente)
  label_seconds = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_seconds, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_obj_set_style_local_text_font(label_seconds, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_text(label_seconds, ":00");
  lv_obj_align(label_seconds, time_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

  // ========================================================================
  // Ligne avant les données
  // ========================================================================
  line_mid_points[0] = {0, 150};
  line_mid_points[1] = {240, 150};
  line_mid = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_mid, line_mid_points, 2);
  lv_obj_add_style(line_mid, LV_LINE_PART_MAIN, &style_line);

  // ========================================================================
  // Zone inférieure: DONNÉES DE MOUVEMENT
  // ========================================================================
  label_active_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_active_time, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 10, 55);
  lv_obj_set_style_local_text_color(label_active_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_active);
  lv_obj_set_style_local_text_font(label_active_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_text(label_active_time, "ACT  00:00");

  label_magnitude_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_magnitude_time, label_active_time, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
  lv_obj_set_style_local_text_color(label_magnitude_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text(label_magnitude_time, "MAG  00:00");

  // Barre de progression (à droite)
  progress_bar = lv_bar_create(lv_scr_act(), nullptr);
  lv_bar_set_range(progress_bar, 0, 100);
  lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
  lv_obj_set_size(progress_bar, 80, 45);
  lv_obj_set_style_local_bg_color(progress_bar, LV_BAR_PART_BG, LV_STATE_DEFAULT, lv_color_hex(0x333333));
  lv_obj_set_style_local_bg_color(progress_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, color_active);
  lv_obj_align(progress_bar, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, -10, 60);

  label_progress = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_progress, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_style_local_text_font(label_progress, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_text(label_progress, "0%");
  lv_obj_align(label_progress, progress_bar, LV_ALIGN_CENTER, 0, 0);

  // ========================================================================
  // Ligne du bas
  // ========================================================================
  line_bottom_points[0] = {0, 215};
  line_bottom_points[1] = {240, 215};
  line_bottom = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(line_bottom, line_bottom_points, 2);
  lv_obj_add_style(line_bottom, LV_LINE_PART_MAIN, &style_line);

  label_step_icon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_step_icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(label_step_icon, Symbols::shoe);
  lv_obj_align(label_step_icon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 5, -5);

  label_step_value = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_step_value, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text(label_step_value, "0");
  lv_obj_align(label_step_value, label_step_icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  // Batterie (centre bas) - icône et pourcentage
  label_battery_icon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_battery_icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text_static(label_battery_icon, Symbols::batteryHalf);
  lv_obj_align(label_battery_icon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_MID, -25, -5);

  label_battery = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_battery, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  lv_label_set_text(label_battery, "100%");
  lv_obj_align(label_battery, label_battery_icon, LV_ALIGN_OUT_RIGHT_MID, 3, 0);

  // Icône Bluetooth (après batterie)
  label_ble_icon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_ble_icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x555555));
  lv_label_set_text_static(label_ble_icon, Symbols::bluetooth);
  lv_obj_align(label_ble_icon, label_battery, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

  label_mode = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_mode, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFF8800));
  lv_label_set_text(label_mode, "NORM");
  lv_obj_align(label_mode, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, -5, -5);

  // Background pour les touches
  backgroundLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_click(backgroundLabel, true);
  lv_label_set_long_mode(backgroundLabel, LV_LABEL_LONG_CROP);
  lv_obj_set_size(backgroundLabel, 240, 240);
  lv_obj_set_pos(backgroundLabel, 0, 0);
  lv_label_set_text_static(backgroundLabel, "");

  // Créer la tâche de rafraîchissement automatique (toutes les ~40ms)
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

// ============================================================================
// Mise à jour périodique
// ============================================================================

void MovementWatchFace::Refresh() {
  UpdateTimeDisplay();
  UpdateMovementData();
}

// ============================================================================
// Mise à jour de l'heure
// ============================================================================

void MovementWatchFace::UpdateTimeDisplay() {
  // Obtenir l'heure depuis le DateTimeController
  uint8_t hour = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();
  uint8_t second = dateTimeController.Seconds();

  // Format heure:minutes
  static char time_str[8];
  std::snprintf(time_str, sizeof(time_str), "%02d:%02d", hour, minute);
  lv_label_set_text(time_label, time_str);

  // Format secondes
  static char sec_str[4];
  std::snprintf(sec_str, sizeof(sec_str), ":%02d", second);
  lv_label_set_text(label_seconds, sec_str);

  // Format date (MM-DD)
  uint8_t month = static_cast<uint8_t>(dateTimeController.Month());
  uint8_t day = dateTimeController.Day();
  static char date_str[8];
  std::snprintf(date_str, sizeof(date_str), "%02d-%02d", month, day);
  lv_label_set_text(label_date, date_str);
}

// ============================================================================
// Mise à jour des données de mouvement
// ============================================================================

void MovementWatchFace::UpdateMovementData() {
  auto status = movement_tracker.getStatus();

  lv_color_t color_text = lv_color_hex(0x98B69A);
  lv_color_t color_active = lv_color_hex(0x00FF00);

  // ========================================================================
  // Indicateur de mouvement (haut droite)
  // ========================================================================
  // any_movement = état continu (en train de bouger)
  // movement_detected = événement ponctuel (ignoré pour l'affichage)
  if (status.any_movement) {
    lv_label_set_text(movement_indicator, "MOVE");
    lv_obj_set_style_local_text_color(movement_indicator, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_active);
  } else {
    lv_label_set_text(movement_indicator, "IDLE");
    lv_obj_set_style_local_text_color(movement_indicator, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_text);
  }

  // ========================================================================
  // Temps actif (AXIS TIME)
  // ========================================================================
  uint32_t seconds = status.axis_active_time / 1000;
  uint32_t minutes = seconds / 60;
  uint32_t remaining_seconds = seconds % 60;

  static char active_str[16];
  std::snprintf(active_str, sizeof(active_str), "ACT  %02lu:%02lu", minutes, remaining_seconds);
  lv_label_set_text(label_active_time, active_str);

  // ========================================================================
  // Magnitude time
  // ========================================================================
  uint32_t mag_seconds = status.magnitude_active_time / 1000;
  uint32_t mag_minutes = mag_seconds / 60;
  uint32_t mag_remaining = mag_seconds % 60;

  static char mag_str[16];
  std::snprintf(mag_str, sizeof(mag_str), "MAG  %02lu:%02lu", mag_minutes, mag_remaining);
  lv_label_set_text(label_magnitude_time, mag_str);

  // ========================================================================
  // Barre de progression
  // ========================================================================
  uint8_t progress = static_cast<uint8_t>(status.progress_percent);
  if (progress > 100) progress = 100;
  lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);

  static char progress_str[8];
  std::snprintf(progress_str, sizeof(progress_str), "%d%%", progress);
  lv_label_set_text(label_progress, progress_str);

  // ========================================================================
  // Pas (depuis le MotionController)
  // ========================================================================
  uint32_t steps = motionController.NbSteps();
  static char steps_str[16];
  std::snprintf(steps_str, sizeof(steps_str), "%lu", steps);
  lv_label_set_text(label_step_value, steps_str);

  // ========================================================================
  // Batterie
  // ========================================================================
  uint8_t batteryPercent = batteryController.PercentRemaining();
  static char battery_str[8];
  std::snprintf(battery_str, sizeof(battery_str), "%d%%", batteryPercent);
  lv_label_set_text(label_battery, battery_str);

  // Changer couleur selon niveau (icône et texte)
  lv_color_t battery_color;
  if (batteryPercent <= 15) {
    battery_color = lv_color_hex(0xFF0000);
  } else if (batteryPercent <= 30) {
    battery_color = lv_color_hex(0xFF8800);
  } else {
    battery_color = color_text;
  }
  lv_obj_set_style_local_text_color(label_battery_icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, battery_color);
  lv_obj_set_style_local_text_color(label_battery, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, battery_color);

  // ========================================================================
  // Mode
  // ========================================================================
  if (system_service.isBatterySavingMode()) {
    lv_label_set_text(label_mode, "BATT");
    lv_obj_set_style_local_text_color(label_mode, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFF0000));
  } else {
    lv_label_set_text(label_mode, "NORM");
    lv_obj_set_style_local_text_color(label_mode, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFF8800));
  }

  // ========================================================================
  // Indicateur Bluetooth
  // ========================================================================
  if (bleController.IsConnected()) {
    lv_obj_set_style_local_text_color(label_ble_icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x0082FC));
  } else {
    lv_obj_set_style_local_text_color(label_ble_icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x555555));
  }
}

// ============================================================================
// Gestion des événements tactiles
// ============================================================================

bool MovementWatchFace::OnTouchEvent(TouchEvents event) {
  // Laisser le système gérer toute la navigation (swipe gauche/droite pour settings, etc.)
  // Ne gérer que le tap pour le mode batterie
  if (event == TouchEvents::DoubleTap) {
    // Double tap pour basculer le mode économie de batterie
    bool current_mode = system_service.isBatterySavingMode();
    system_service.setBatterySavingMode(!current_mode);
    Refresh();
    return true;
  }

  // Tous les autres événements (swipes) sont gérés par le système
  return false;
}

// ============================================================================
// Gestion des boutons
// ============================================================================

bool MovementWatchFace::OnButtonPushed() {
  // Appui sur le bouton: réinitialiser les statistiques
  movement_tracker.reset();
  Refresh();
  return true;
}