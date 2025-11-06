#pragma once

#include <array>
#include <cmath>
#include <cstdint>

// ============================================================================
// Butterworth Low Pass Filter 2nd Order
// ============================================================================
class ButterworthFilter {
public:
  ButterworthFilter(float cutoff_hz, float sample_rate);
  float apply(float input);
  void reset();

private:
  float a0, a1, a2, b1, b2;
  float x1 = 0.0f, x2 = 0.0f;
  float y1 = 0.0f, y2 = 0.0f;
};

// ============================================================================
// Magnitude Active Time Counter
// ============================================================================
class MagnitudeActiveTimeCounter {
public:
  MagnitudeActiveTimeCounter();
  uint32_t update(const std::array<float, 3>& gravity_corrected_xl, uint32_t delta_time_ms);
  bool isMoving() const { return in_movement; }
  void reset();
  
  // FIX: Getter sécurisé pour accéder au temps total en secondes
  // Évite le débordement en retournant une valeur sûre
  uint32_t getActiveTotalSeconds() const {
    return static_cast<uint32_t>((active_time_ms / 1000) % 0xFFFFFFFFULL);
  }

private:
  static constexpr float MOVEMENT_THRESHOLD = 0.388f;
  static constexpr uint32_t DEBOUNCE_THRESHOLD = 90; // milliseconds (harmonized name)
  
  ButterworthFilter filter{7.8f, 100.0f}; // 7.8Hz cutoff, 100Hz sample rate
  
  bool in_movement = false;
  // FIX: Changé de uint32_t à uint64_t pour éviter débordement après ~50 jours
  // Nouvelle capacité: ~584 millions d'années (au lieu de 49.7 jours)
  uint64_t active_time_ms = 0;
  uint32_t debounce_time = 0;
};

// ============================================================================
// Active Time Counter (per-axis)
// ============================================================================
class ActiveTimeCounter {
public:
  ActiveTimeCounter();
  uint32_t update(const std::array<float, 3>& gravity_corrected_xl, uint32_t delta_time_ms);
  bool isMoving() const { return in_movement; }
  void reset();
  
  // FIX: Getter sécurisé pour accéder au temps total en secondes
  uint32_t getActiveTotalSeconds() const {
    return static_cast<uint32_t>((active_time_ms / 1000) % 0xFFFFFFFFULL);
  }

private:
  static constexpr float MOVEMENT_THRESHOLD = 0.02547f; // gs
  static constexpr uint32_t DEBOUNCE_THRESHOLD = 90;    // milliseconds
  // FIX: Cap pour debounce_time afin d'éviter débordement en oscillation prolongée
  static constexpr uint32_t MAX_DEBOUNCE_TIME = DEBOUNCE_THRESHOLD + 10000; // 10 secondes de marge
  
  ButterworthFilter filter{3.83f, 100.0f}; // 3.83Hz cutoff, 100Hz sample rate
  
  bool in_movement = false;
  // FIX: Changé de uint32_t à uint64_t
  uint64_t active_time_ms = 0;
  uint32_t debounce_time = 0;
};

// ============================================================================
// Movement Detector
// ============================================================================
class MovementDetector {
public:
  MovementDetector();
  bool detect(const std::array<float, 3>& gravity_corrected_xl, uint32_t delta_time_ms);
  void reset();

private:
  static constexpr float ACTIVE_THRESHOLD = 0.189f;      // g
  static constexpr float INACTIVE_THRESHOLD = 0.01f;     // g
  static constexpr uint32_t REFRACTORY_PERIOD = 38;      // milliseconds
  // FIX: Cap pour refractory_tick afin d'éviter débordement
  static constexpr uint32_t MAX_REFRACTORY_TICK = REFRACTORY_PERIOD + 10000; // 10 secondes de marge
  
  ButterworthFilter filter{7.8f, 100.0f}; // 7.8Hz cutoff
  
  std::array<bool, 3> axis_armed = {true, true, true};
  bool in_refractory = false;
  uint32_t refractory_tick = 0;
};

// ============================================================================
// Main analyzer combining all algorithms
// ============================================================================
class MovementAnalyzer {
public:
  struct AnalysisResult {
    uint32_t magnitude_active_time;
    uint32_t axis_active_time;
    bool movement_detected;
    bool any_movement;
  };

  MovementAnalyzer();
  AnalysisResult analyze(const std::array<float, 3>& gravity_corrected_xl, uint32_t delta_time_ms);
  void reset();

private:
  MagnitudeActiveTimeCounter mag_counter;
  ActiveTimeCounter axis_counter;
  MovementDetector detector;
};