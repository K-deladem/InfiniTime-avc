#pragma once

#include <lvgl/lvgl.h>
#include <chrono>
#include <cstdint>
#include <memory>

#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "components/datetime/DateTimeController.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/settings/Settings.h"
#include "components/motion/MotionController.h"
#include "utility/DirtyValue.h"
#include "algorithms/RehabilitationAlgorithm.h"

namespace Pinetime {
  namespace System {
    class SystemTask;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceRehabilitationCasio : public Screen {
      public:
        WatchFaceRehabilitationCasio(
          Controllers::DateTime& dateTimeController,
          const Controllers::Battery& batteryController,
          const Controllers::Ble& bleController,
          Controllers::NotificationManager& notificationManager,
          Controllers::Settings& settingsController,
          Controllers::MotionController& motionController,
          Pinetime::System::SystemTask& systemTask
        );

        ~WatchFaceRehabilitationCasio() override;
        void Refresh() override;
        bool OnButtonPushed() override;

      private:
        // ===== RÉFÉRENCES AUX CONTRÔLEURS =====
        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::MotionController& motionController;
        Pinetime::System::SystemTask& systemTask;

        // ===== DONNÉES DE SYNCHRONISATION =====
        Utility::DirtyValue<uint8_t> batteryPercentRemaining{};
        Utility::DirtyValue<bool> isCharging{};
        Utility::DirtyValue<bool> bleState{};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime{};
        
        // ===== DONNÉES CALCULÉES (POST-AVC) =====
        uint8_t rehabilitationScore = 0;
        uint16_t movementQuality = 0;
        uint8_t movementSymmetry = 0;
        bool isActiveMovement = false;
        
        // ===== TIMERS POUR THROTTLING =====
        uint32_t lastMotionUpdate = 0;
        uint32_t lastBleUpdate = 0;
        uint8_t lastSentScore = 0;
        uint8_t lastSentSymmetry = 0;
        
        // ===== ALGORITHME =====
        RehabilitationAlgorithm algorithm;

        // ===== ÉLÉMENTS UI LVGL =====
        lv_obj_t* timeDisplay = nullptr;
        lv_obj_t* scoreDisplay = nullptr;
        lv_obj_t* progressBar = nullptr;
        lv_obj_t* statusIcon = nullptr;
        lv_obj_t* symmetryLabel = nullptr;

        // ===== STYLES LVGL =====
        lv_style_t time_style;
        lv_style_t score_style;
        lv_style_t symmetry_style;
        lv_style_t status_style;
        lv_style_t bar_bg_style;
        lv_style_t bar_indic_style;

        // ===== MÉTHODES PRIVÉES =====
        void UpdateTime();
        void UpdateMotionData();
        void UpdateUI();
        void SendDataToMobile();
        bool ValidateSensorData(int16_t x, int16_t y, int16_t z);
      };

    }
  }
}

// ===== ENREGISTREMENT DE LA WATCHFACE =====
namespace Pinetime {
  namespace Applications {
    template <>
    struct WatchFaceTraits<WatchFace::RehabilitationCasio> {
      static constexpr WatchFace watchFace = WatchFace::RehabilitationCasio;
      static constexpr const char* name = "Rehab Casio";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceRehabilitationCasio(
          controllers.dateTimeController,
          controllers.batteryController,
          controllers.bleController,
          controllers.notificationManager,
          controllers.settingsController,
          controllers.motionController,
          *controllers.systemTask
        );
      }

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}