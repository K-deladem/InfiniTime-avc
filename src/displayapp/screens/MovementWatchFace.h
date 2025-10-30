// ============================================================================
// MovementWatchFace.h - Watchface affichant les algorithmes de mouvement
// ============================================================================
// PRODUCTION-READY - Code complet et testé
// ============================================================================

#pragma once

#include <cstdint>
#include <array>
#include "displayapp/screens/Screen.h"
#include "displayapp/Colors.h"
#include "components/motion/MovementTrackerIntegration.h"

// LittlevGL
#include <lvgl/lvgl.h>

namespace Pinetime::Applications::Screens {

class MovementWatchFace : public Screen {
public:
  // Constructeur simplifié
  MovementWatchFace(DisplayApp* app);
  ~MovementWatchFace() override;

  // Méthodes de l'interface Screen
  void Refresh() override;
  bool OnTouchEvent(TouchEvents event) override;
  bool OnButtonPushed() override;

private:
  // ========================================================================
  // Références aux services de mouvement (singletons)
  // ========================================================================
  Pinetime::Controllers::MovementTracker& movement_tracker = 
      Pinetime::Controllers::MovementTracker::getInstance();
  Pinetime::System::MovementSystemService& system_service = 
      Pinetime::System::MovementSystemService::getInstance();

  // ========================================================================
  // Objets LittlevGL
  // ========================================================================
  lv_obj_t* container;                    // Container principal
  lv_obj_t* time_label;                   // Heure (ligne 1)
  lv_obj_t* active_time_label;            // Temps actif (ligne 2)
  lv_obj_t* magnitude_time_label;         // Magnitude time (ligne 3)
  lv_obj_t* axis_time_label;              // Axis time (ligne 3)
  lv_obj_t* progress_bar;                 // Barre de progression (ligne 4)
  lv_obj_t* progress_percent_label;       // Pourcentage (ligne 4)
  lv_obj_t* movement_indicator;           // Indicateur mouvement (ligne 5)
  lv_obj_t* accel_label;                  // Accélération (ligne 6)
  lv_obj_t* mode_label;                   // Mode batterie (ligne 7)

  // ========================================================================
  // État interne
  // ========================================================================
  uint8_t refresh_count = 0;              // Compteur pour optimiser refresh
  
  // ========================================================================
  // Méthodes privées
  // ========================================================================
  void CreateUI();                        // Créer les éléments UI
  void UpdateMovementData();              // Mettre à jour données mouvement
  void UpdateTimeDisplay();               // Mettre à jour l'heure

  // ========================================================================
  // Constantes de couleur (FIXED: static const au lieu de constexpr)
  // Les initialisations sont dans le .cpp car lv_color_make() n'est pas constexpr
  // ========================================================================
  static const lv_color_t color_active;    // Green
  static const lv_color_t color_idle;      // Light gray
  static const lv_color_t color_progress;  // Cyan
};

} // namespace Pinetime::Applications::Screens