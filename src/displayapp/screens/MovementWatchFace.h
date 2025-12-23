// ============================================================================
// MovementWatchFace.h - Watchface affichant les algorithmes de mouvement
// ============================================================================
// PRODUCTION-READY - Code complet et testé
// ============================================================================

#pragma once

#include <cstdint>
#include <array>
#include <lvgl/lvgl.h>
#include "displayapp/screens/Screen.h"
#include "displayapp/Colors.h"
#include "components/motion/MovementTrackerIntegration.h"
#include "components/datetime/DateTimeController.h"
#include "components/motion/MotionController.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      class MovementWatchFace : public Screen {
      public:
        MovementWatchFace(Controllers::DateTime& dateTimeController,
                         const Controllers::Battery& batteryController,
                         Controllers::MotionController& motionController,
                         const Controllers::Ble& bleController);
        ~MovementWatchFace() override;

        // Méthodes de l'interface Screen
        void Refresh() override;
        bool OnTouchEvent(TouchEvents event) override;
        bool OnButtonPushed() override;

      private:
        // ========================================================================
        // Controllers
        // ========================================================================
        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        Controllers::MotionController& motionController;
        const Controllers::Ble& bleController;

        // ========================================================================
        // Références aux services de mouvement (singletons)
        // ========================================================================
        Pinetime::Controllers::MovementTracker& movement_tracker =
            Pinetime::Controllers::MovementTracker::getInstance();

        Pinetime::System::MovementSystemService& system_service =
            Pinetime::System::MovementSystemService::getInstance();

        // ========================================================================
        // Objets LittlevGL - Style Casio
        // ========================================================================
        lv_obj_t* label_date = nullptr;            // Date (haut gauche)
        lv_obj_t* movement_indicator = nullptr;    // Indicateur mouvement (haut droite)
        lv_obj_t* line_top = nullptr;              // Ligne horizontale haut
        lv_obj_t* time_label = nullptr;            // Heure principale (centre)
        lv_obj_t* label_seconds = nullptr;         // Secondes
        lv_obj_t* line_mid = nullptr;              // Ligne horizontale milieu
        lv_obj_t* label_active_time = nullptr;     // Temps actif (ACT)
        lv_obj_t* label_magnitude_time = nullptr;  // Magnitude time (MAG)
        lv_obj_t* progress_bar = nullptr;          // Barre de progression
        lv_obj_t* label_progress = nullptr;        // Pourcentage
        lv_obj_t* line_bottom = nullptr;           // Ligne horizontale bas
        lv_obj_t* label_step_icon = nullptr;       // Icône pas
        lv_obj_t* label_step_value = nullptr;      // Valeur pas
        lv_obj_t* label_battery_icon = nullptr;    // Icône batterie
        lv_obj_t* label_battery = nullptr;         // État batterie
        lv_obj_t* label_ble_icon = nullptr;        // Icône Bluetooth
        lv_obj_t* label_mode = nullptr;            // Mode (NORM/BATT)
        lv_obj_t* backgroundLabel = nullptr;       // Background pour touches

        // Style des lignes
        lv_style_t style_line;
        lv_point_t line_top_points[2];
        lv_point_t line_mid_points[2];
        lv_point_t line_bottom_points[2];

        // Tâche de rafraîchissement LVGL
        lv_task_t* taskRefresh = nullptr;

        // ========================================================================
        // Méthodes privées
        // ========================================================================
        void CreateUI();              // Créer les éléments UI
        void UpdateMovementData();    // Mettre à jour les données de mouvement
        void UpdateTimeDisplay();     // Mettre à jour l'heure
      };

    } // namespace Screens
  } // namespace Applications
} // namespace Pinetime
