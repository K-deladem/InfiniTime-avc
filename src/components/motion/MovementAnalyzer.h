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
  // Seuil basé sur la déviation par rapport à 1g
  // 0.08g = mouvement léger détectable (marche, gestes)
  static constexpr float MOVEMENT_THRESHOLD = 0.08f; // g deviation from 1g
  static constexpr uint32_t DEBOUNCE_THRESHOLD = 200; // milliseconds

  // Filtre plus rapide pour réponse plus immédiate (2Hz @ 10Hz)
  ButterworthFilter filter{2.0f, 10.0f};

  bool in_movement = false;
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
  // Seuil sur la dérivée (variation entre échantillons)
  // 0.03g de variation = mouvement détectable
  static constexpr float MOVEMENT_THRESHOLD = 0.03f; // g/sample
  static constexpr uint32_t DEBOUNCE_THRESHOLD = 300;    // milliseconds
  static constexpr uint32_t MAX_DEBOUNCE_TIME = DEBOUNCE_THRESHOLD + 10000;

  // Valeurs précédentes pour calculer la dérivée
  std::array<float, 3> prev_values = {0.0f, 0.0f, 0.0f};
  bool initialized = false;

  // Filtres envelope pour lisser la dérivée (2Hz @ 10Hz)
  std::array<ButterworthFilter, 3> filters {{
    ButterworthFilter{2.0f, 10.0f},
    ButterworthFilter{2.0f, 10.0f},
    ButterworthFilter{2.0f, 10.0f}
  }};

  bool in_movement = false;
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
  // Seuils sur la dérivée (variation entre échantillons)
  static constexpr float ACTIVE_THRESHOLD = 0.05f;      // g/sample - seuil pour déclencher
  static constexpr float INACTIVE_THRESHOLD = 0.01f;    // g/sample - seuil pour réarmer
  static constexpr uint32_t REFRACTORY_PERIOD = 150;    // milliseconds
  static constexpr uint32_t MAX_REFRACTORY_TICK = REFRACTORY_PERIOD + 10000;

  // Valeurs précédentes pour calculer la dérivée
  std::array<float, 3> prev_values = {0.0f, 0.0f, 0.0f};
  bool initialized = false;

  // Filtres envelope (2Hz @ 10Hz)
  std::array<ButterworthFilter, 3> filters {{
    ButterworthFilter{2.0f, 10.0f},
    ButterworthFilter{2.0f, 10.0f},
    ButterworthFilter{2.0f, 10.0f}
  }};

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