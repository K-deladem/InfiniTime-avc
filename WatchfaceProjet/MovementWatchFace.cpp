// ============================================================================
// MovementWatchFace.cpp - Implémentation de la watchface de mouvement
// ============================================================================

#include "MovementWatchFace.h"
#include <cstdio>
#include <ctime>

using namespace Pinetime::Applications::Screens;

// ============================================================================
// Constructeur
// ============================================================================

MovementWatchFace::MovementWatchFace(DisplayApp* app, Controllers::NavigationPattern& nav)
    : Screen(app) {
  
  CreateUI();
  Refresh();
}

// ============================================================================
// Destructeur
// ============================================================================

MovementWatchFace::~MovementWatchFace() {
  lv_obj_del(container);
}

// ============================================================================
// Création de l'UI
// ============================================================================

void MovementWatchFace::CreateUI() {
  // Container principal
  container = lv_cont_create(lv_scr_act(), nullptr);
  lv_cont_set_layout(container, LV_LAYOUT_COLUMN_MID);
  lv_cont_set_fit(container, LV_FIT_PARENT);
  lv_cont_set_style(container, LV_CONT_STYLE_MAIN, &lv_style_plain_color);
  lv_obj_set_style_local_bg_color(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  // ========================================================================
  // Ligne 1: Heure actuelle
  // ========================================================================
  time_label = lv_label_create(container, nullptr);
  lv_label_set_text(time_label, "00:00:00");
  lv_obj_set_style_local_text_font(time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                    &lv_font_montserrat_48);
  lv_obj_set_style_local_text_color(time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                     LV_COLOR_WHITE);
  lv_obj_align(time_label, container, LV_ALIGN_IN_TOP_MID, 0, 10);

  // ========================================================================
  // Ligne 2: Temps actif (Axis time)
  // ========================================================================
  active_time_label = lv_label_create(container, nullptr);
  lv_label_set_text(active_time_label, "Active: 0m 0s");
  lv_obj_set_style_local_text_font(active_time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                    &lv_font_montserrat_22);
  lv_obj_set_style_local_text_color(active_time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                     LV_COLOR_YELLOW);
  lv_obj_align(active_time_label, time_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 15);

  // ========================================================================
  // Ligne 3: Magnitude time et Axis time
  // ========================================================================
  magnitude_time_label = lv_label_create(container, nullptr);
  lv_label_set_text(magnitude_time_label, "Mag: 0s");
  lv_obj_set_style_local_text_font(magnitude_time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                    &lv_font_montserrat_14);
  lv_obj_set_style_local_text_color(magnitude_time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                     LV_COLOR_CYAN);

  axis_time_label = lv_label_create(container, nullptr);
  lv_label_set_text(axis_time_label, "Axis: 0s");
  lv_obj_set_style_local_text_font(axis_time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                    &lv_font_montserrat_14);
  lv_obj_set_style_local_text_color(axis_time_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                     LV_COLOR_LIME);

  // ========================================================================
  // Ligne 4: Barre de progression
  // ========================================================================
  progress_bar = lv_bar_create(container, nullptr);
  lv_bar_set_range(progress_bar, 0, 100);
  lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
  lv_obj_set_size(progress_bar, 200, 20);
  lv_obj_set_style_local_bg_color(progress_bar, LV_BAR_PART_BG, LV_STATE_DEFAULT, 
                                   LV_COLOR_GRAY);
  lv_obj_set_style_local_bg_color(progress_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, 
                                   LV_COLOR_CYAN);
  lv_obj_align(progress_bar, container, LV_ALIGN_CENTER, 0, 5);

  progress_percent_label = lv_label_create(container, nullptr);
  lv_label_set_text(progress_percent_label, "0%");
  lv_obj_set_style_local_text_font(progress_percent_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                    &lv_font_montserrat_16);
  lv_obj_set_style_local_text_color(progress_percent_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                     LV_COLOR_WHITE);
  lv_obj_align(progress_percent_label, progress_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

  // ========================================================================
  // Ligne 5: Indicateur de mouvement
  // ========================================================================
  movement_indicator = lv_label_create(container, nullptr);
  lv_label_set_text(movement_indicator, "⚪ Idle");
  lv_obj_set_style_local_text_font(movement_indicator, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                    &lv_font_montserrat_18);
  lv_obj_set_style_local_text_color(movement_indicator, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                     LV_COLOR_LIGHTGRAY);

  // ========================================================================
  // Ligne 6: Accélération actuelle (X, Y, Z)
  // ========================================================================
  accel_label = lv_label_create(container, nullptr);
  lv_label_set_text(accel_label, "X:0.00g Y:0.00g Z:0.00g");
  lv_obj_set_style_local_text_font(accel_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                    &lv_font_montserrat_12);
  lv_obj_set_style_local_text_color(accel_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                     LV_COLOR_LIGHTGRAY);

  // ========================================================================
  // Ligne 7: Mode (Normal/Battery Saving)
  // ========================================================================
  mode_label = lv_label_create(container, nullptr);
  lv_label_set_text(mode_label, "Mode: Normal");
  lv_obj_set_style_local_text_font(mode_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                    &lv_font_montserrat_12);
  lv_obj_set_style_local_text_color(mode_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                     LV_COLOR_ORANGE);
}

// ============================================================================
// Mise à jour périodique
// ============================================================================

void MovementWatchFace::Refresh() {
  // Optimiser: mettre à jour seulement tous les 10 rafraîchissements (~1 seconde)
  refresh_count++;
  if (refresh_count < 10) {
    return;
  }
  refresh_count = 0;

  UpdateTimeDisplay();
  UpdateMovementData();
}

// ============================================================================
// Mise à jour de l'heure
// ============================================================================

void MovementWatchFace::UpdateTimeDisplay() {
  // TODO: Obtenir l'heure du système
  // Pour l'instant, valeur statique
  static char time_str[16];
  std::snprintf(time_str, sizeof(time_str), "12:34:56");
  lv_label_set_text(time_label, time_str);
}

// ============================================================================
// Mise à jour des données de mouvement
// ============================================================================

void MovementWatchFace::UpdateMovementData() {
  auto status = movement_tracker.getStatus();

  // ========================================================================
  // Temps actif (Axis - le principal)
  // ========================================================================
  uint32_t seconds = status.axis_active_time / 1000;
  uint32_t minutes = seconds / 60;
  uint32_t remaining_seconds = seconds % 60;

  static char active_time_str[32];
  std::snprintf(active_time_str, sizeof(active_time_str), 
                "Active: %lum %lus", minutes, remaining_seconds);
  lv_label_set_text(active_time_label, active_time_str);

  // ========================================================================
  // Magnitude time et Axis time (détails)
  // ========================================================================
  static char mag_str[32];
  std::snprintf(mag_str, sizeof(mag_str), "Mag: %lu.%lus", 
                status.magnitude_active_time / 1000,
                (status.magnitude_active_time % 1000) / 100);
  lv_label_set_text(magnitude_time_label, mag_str);

  static char axis_str[32];
  std::snprintf(axis_str, sizeof(axis_str), "Axis: %lu.%lus", 
                status.axis_active_time / 1000,
                (status.axis_active_time % 1000) / 100);
  lv_label_set_text(axis_time_label, axis_str);

  // ========================================================================
  // Barre de progression
  // ========================================================================
  uint8_t progress = static_cast<uint8_t>(status.progress_percent);
  lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);

  static char progress_str[16];
  std::snprintf(progress_str, sizeof(progress_str), "%.1f%%", status.progress_percent);
  lv_label_set_text(progress_percent_label, progress_str);

  // ========================================================================
  // Indicateur de mouvement
  // ========================================================================
  if (status.any_movement) {
    lv_label_set_text(movement_indicator, "🔴 Moving");
    lv_obj_set_style_local_text_color(movement_indicator, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                       color_active);
  } else {
    lv_label_set_text(movement_indicator, "⚪ Idle");
    lv_obj_set_style_local_text_color(movement_indicator, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                       color_idle);
  }

  // ========================================================================
  // Indicateur de détection de mouvement (détection rapide)
  // ========================================================================
  if (status.movement_detected) {
    lv_obj_set_style_local_text_color(movement_indicator, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                       LV_COLOR_RED);
  }

  // ========================================================================
  // Mode (Normal vs Battery Saving)
  // ========================================================================
  if (system_service.isBatterySavingMode()) {
    lv_label_set_text(mode_label, "Mode: Battery Saving");
    lv_obj_set_style_local_text_color(mode_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                       LV_COLOR_RED);
  } else {
    lv_label_set_text(mode_label, "Mode: Normal");
    lv_obj_set_style_local_text_color(mode_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 
                                       LV_COLOR_ORANGE);
  }

  // ========================================================================
  // Données d'accélération actuelle (si disponible)
  // ========================================================================
  // Les accélérations sont envoyées via BLE et peuvent être affichées
  static char accel_str[64];
  std::snprintf(accel_str, sizeof(accel_str), 
                "X:%.2f Y:%.2f Z:%.2f g");
  lv_label_set_text(accel_label, accel_str);
}

// ============================================================================
// Gestion des événements tactiles
// ============================================================================

bool MovementWatchFace::OnTouchEvent(TouchEvents event) {
  // Swipe pour changer de watchface
  switch (event) {
    case TouchEvents::SwipeLeft:
    case TouchEvents::SwipeRight:
      return true; // Laisser le système naviguer
    
    case TouchEvents::Tap:
      // Basculer le mode économie de batterie au tap
      bool current_mode = system_service.isBatterySavingMode();
      system_service.setBatterySavingMode(!current_mode);
      Refresh();
      return true;
    
    default:
      return false;
  }
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
