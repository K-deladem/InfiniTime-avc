// ============================================================================
// MovementWatchFace.h - Watchface affichant les algorithmes de mouvement
// ============================================================================

#pragma once

#include <cstdint>
#include <array>
#include "displayapp/screens/Screen.h"
#include "displayapp/Colors.h"
#include "/movement/MovementTrackerIntegration.h"

// LittlevGL
#include <lvgl/lvgl.h>

namespace Pinetime::Applications::Screens {

class MovementWatchFace : public Screen {
public:
  MovementWatchFace(DisplayApp* app, Controllers::NavigationPattern& nav);
  ~MovementWatchFace() override;

  void Refresh() override;
  bool OnTouchEvent(TouchEvents event) override;
  bool OnButtonPushed() override;

private:
  // References to the movement system
  Controllers::MovementTracker& movement_tracker = Controllers::MovementTracker::getInstance();
  Controllers::MovementSystemService& system_service = Controllers::System::MovementSystemService::getInstance();

  // LittlevGL objects
  lv_obj_t* container;
  lv_obj_t* time_label;
  lv_obj_t* active_time_label;
  lv_obj_t* magnitude_time_label;
  lv_obj_t* axis_time_label;
  lv_obj_t* progress_bar;
  lv_obj_t* progress_percent_label;
  lv_obj_t* movement_indicator;
  lv_obj_t* accel_label;
  lv_obj_t* mode_label;

  // Refresh counter for optimization
  uint8_t refresh_count = 0;
  
  // Helper functions
  void CreateUI();
  void UpdateMovementData();
  void UpdateTimeDisplay();
  void DrawAccelerationIndicator();

  // Colors
  static constexpr uint32_t color_active = Colors::green;
  static constexpr uint32_t color_idle = Colors::lightgray;
  static constexpr uint32_t color_progress = Colors::cyan;
};

} // namespace Pinetime::Applications::Screens
