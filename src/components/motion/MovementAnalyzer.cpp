#include "MovementAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <numbers>  // C++20



// ============================================================================
// ButterworthFilter Implementation
// ============================================================================

ButterworthFilter::ButterworthFilter(float cutoff_hz, float sample_rate) {
  // Butterworth 2nd order low-pass filter design
  float omega = 2.0f * static_cast<float>(std::numbers::pi) * cutoff_hz / sample_rate;
  float sin_omega = std::sin(omega);
  float cos_omega = std::cos(omega);
  
  // Formule correcte pour Butterworth
  // Pour un filtre Butterworth, Q = 1/sqrt(2) ≈ 0.707
  // Mais la formule d'alpha doit être: alpha = sin_omega / 2.0f
  float alpha = sin_omega / 2.0f;

  // IIR filter coefficients (forme directe II)
  b1 = 2.0f * cos_omega;
  b2 = -(1.0f - alpha);
  
  float norm = 1.0f + alpha;
  a0 = (1.0f - cos_omega) / (2.0f * norm);
  a1 = (1.0f - cos_omega) / norm;
  a2 = (1.0f - cos_omega) / (2.0f * norm);
  
  b1 /= norm;
  b2 /= norm;
}

float ButterworthFilter::apply(float input) {
  // FIX: Vérifier les entrées NaN/Inf pour éviter la propagation d'erreurs
  if (!std::isfinite(input)) {
    // Maintenir la dernière valeur valide
    return y1;
  }
  
  float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
  
  // FIX: Vérifier les sorties NaN/Inf
  if (!std::isfinite(output)) {
    // Clamp à 0.0f au lieu de propager NaN
    output = 0.0f;
  }
  
  x2 = x1;
  x1 = input;
  y2 = y1;
  y1 = output;
  
  return output;
}

void ButterworthFilter::reset() {
  x1 = x2 = y1 = y2 = 0.0f;
}

// ============================================================================
// MagnitudeActiveTimeCounter Implementation
// ============================================================================

MagnitudeActiveTimeCounter::MagnitudeActiveTimeCounter() = default;

uint32_t MagnitudeActiveTimeCounter::update(const std::array<float, 3>& gravity_corrected_xl,
                                             uint32_t delta_time_ms) {
  // Validate input
  if (delta_time_ms == 0) {
    // FIX: Retourner la valeur convertie correctement
    return static_cast<uint32_t>(active_time_ms);
  }

  // PSEUDOCODE: SET xl_magnitude to norm( gravity_corrected_xl )
  float magnitude = std::sqrt(gravity_corrected_xl[0] * gravity_corrected_xl[0] +
                              gravity_corrected_xl[1] * gravity_corrected_xl[1] +
                              gravity_corrected_xl[2] * gravity_corrected_xl[2]);
  
  // PSEUDOCODE: SET xl_magnitude_clean to low_pass_filter( xl_magnitude )
  float magnitude_clean = filter.apply(magnitude);

  // PSEUDOCODE: IF member_variables.in_movement
  if (in_movement) {
    // PSEUDOCODE: INCREMENT member_variables.active_time by delta_time
    // FIX: Accumulation sûre dans uint64_t
    active_time_ms += delta_time_ms;
    
    // PSEUDOCODE: IF xl_magnitude_clean is less than movement_threshold
    if (magnitude_clean < MOVEMENT_THRESHOLD) {
      // PSEUDOCODE: INCREMENT member_variables.debounce_time by delta_time
      debounce_time += delta_time_ms;
      
      // PSEUDOCODE: IF member_variables.debounce_time is greater than debounce_thresh
      if (debounce_time > DEBOUNCE_THRESHOLD) {
        // PSEUDOCODE: SET member_variables.in_movement to False
        in_movement = false;
        // PSEUDOCODE: SET member_variables.debounce_time to 0
        debounce_time = 0;
      }
    } else {
      // PSEUDOCODE: ELSE SET member_variables.debounce_time to 0
      debounce_time = 0;
    }
  } else {
    // PSEUDOCODE: ELSE (not in_movement)
    // PSEUDOCODE: IF xl_magnitude_clean is greater than movement_threshold
    if (magnitude_clean > MOVEMENT_THRESHOLD) {
      // PSEUDOCODE: INCREMENT member_variables.debounce_time by delta_time
      debounce_time += delta_time_ms;
      
      // PSEUDOCODE: IF member_variables.debounce_time is greater than debounce_thresh
      if (debounce_time > DEBOUNCE_THRESHOLD) {
        // PSEUDOCODE: SET member_variables.in_movement to True
        in_movement = true;
        // PSEUDOCODE: SET member_variables.debounce_time to 0
        debounce_time = 0;
      }
    } else {
      // PSEUDOCODE: ELSE SET new_debounce_timer to 0
      debounce_time = 0;
    }
  }

  // PSEUDOCODE: RETURN member_variables.active_time
  // FIX: Retourner une conversion sûre
  return static_cast<uint32_t>(active_time_ms);
}

void MagnitudeActiveTimeCounter::reset() {
  in_movement = false;
  active_time_ms = 0;
  debounce_time = 0;
  filter.reset();
}

// ============================================================================
// ActiveTimeCounter Implementation
// ============================================================================

ActiveTimeCounter::ActiveTimeCounter() = default;

uint32_t ActiveTimeCounter::update(const std::array<float, 3>& gravity_corrected_xl,
                                   uint32_t delta_time_ms) {
  // Validate input
  if (delta_time_ms == 0) {
    return static_cast<uint32_t>(active_time_ms);
  }

  // PSEUDOCODE: INITIALIZE envelope to [0,0,0]
  std::array<float, 3> envelope;
  
  // PSEUDOCODE: FOR each axis in gravity_corrected_xl
  //             envelope[axis_index] = low_pass_filter( abs( axis ) )
  for (int i = 0; i < 3; ++i) {
    envelope[i] = filter.apply(std::fabs(gravity_corrected_xl[i]));
  }

  // PSEUDOCODE: IF member_variables.in_movement
  if (in_movement) {
    // PSEUDOCODE: INCREMENT member_variables.active_time by delta_time
    // FIX: Accumulation sûre dans uint64_t
    active_time_ms += delta_time_ms;
    
    // PSEUDOCODE: IF ALL envelope axes are below movement_threshold
    bool all_below = (envelope[0] < MOVEMENT_THRESHOLD &&
                      envelope[1] < MOVEMENT_THRESHOLD &&
                      envelope[2] < MOVEMENT_THRESHOLD);
    
    if (all_below) {
      // PSEUDOCODE: INCREMENT member_variables.debounce_time by delta_time
      // FIX: Capper debounce_time pour éviter débordement
      debounce_time = std::min(debounce_time + delta_time_ms, MAX_DEBOUNCE_TIME);
      
      // PSEUDOCODE: IF member_variables.debounce_time is greater than debounce_threshold
      if (debounce_time > DEBOUNCE_THRESHOLD) {
        // PSEUDOCODE: SET member_variable.in_movement to False
        in_movement = false;
        // PSEUDOCODE: (IMPLIED) SET member_variable.debounce_time to 0
        debounce_time = 0;
      }
    } else {
      // PSEUDOCODE: (IMPLIED) IF NOT all_below, reset debounce
      // PSEUDOCODE: ELSE SET member_variable.debounce_time to 0
      debounce_time = 0;
    }
  } else {
    // PSEUDOCODE: ELSE (not in_movement)
    // PSEUDOCODE: IF ANY envelope axis is above the movement_threshold
    bool any_above = (envelope[0] > MOVEMENT_THRESHOLD ||
                      envelope[1] > MOVEMENT_THRESHOLD ||
                      envelope[2] > MOVEMENT_THRESHOLD);
    
    if (any_above) {
      // =====================================================================
      // PSEUDOCODE TEXTUAL COMPLIANCE FIX
      // =====================================================================
      // PSEUDOCODE says (line 39-44):
      //   IF ANY envelope axis is above the movement_threshold
      //     SET member_variables.in_movement to True
      //     SET member_variable.debounce_time to 0
      //
      // This is IMMEDIATE activation - NO debounce delay before activation.
      // The pseudocode is unambiguous and clear.
      // =====================================================================
      
      // PSEUDOCODE: SET member_variables.in_movement to True
      in_movement = true;
      
      // PSEUDOCODE: SET member_variable.debounce_time to 0
      debounce_time = 0;
    } else {
      // PSEUDOCODE: (IMPLIED) IF NOT any_above
      // PSEUDOCODE: ELSE SET member_variable.debounce_time to 0
      debounce_time = 0;
    }
  }

  // PSEUDOCODE: RETURN member_variable.active_time
  // FIX: Retourner une conversion sûre
  return static_cast<uint32_t>(active_time_ms);
}

void ActiveTimeCounter::reset() {
  in_movement = false;
  active_time_ms = 0;
  debounce_time = 0;
  filter.reset();
}

// ============================================================================
// MovementDetector Implementation
// ============================================================================

MovementDetector::MovementDetector() = default;

bool MovementDetector::detect(const std::array<float, 3>& gravity_corrected_xl,
                              uint32_t delta_time_ms) {
  // Validate input
  if (delta_time_ms == 0) {
    return false;
  }

  // PSEUDOCODE: INITIALIZE envelope to [0,0,0]
  std::array<float, 3> envelope;
  // PSEUDOCODE: FOR each axis in gravity_corrected_xl
  //             envelope[axis_index] = low_pass_filter( abs( axis ) )
  for (int i = 0; i < 3; ++i) {
    envelope[i] = filter.apply(std::fabs(gravity_corrected_xl[i]));
  }

  // PSEUDOCODE: INITIALIZE movement_detected to false
  bool movement_detected = false;

  // PSEUDOCODE: SET any_axis_became_active to false
  bool any_axis_became_active = false;
  
  // PSEUDOCODE: FOR each axis in envelope
  for (int i = 0; i < 3; ++i) {
    // PSEUDOCODE: IF member_variables.axis_armed[axis_index]
    if (axis_armed[i]) {
      // PSEUDOCODE: IF axis is greater than the active_threshold
      if (envelope[i] > ACTIVE_THRESHOLD) {
        // PSEUDOCODE: SET member_variables.axis_armed[axis_index] to false
        axis_armed[i] = false;
        // PSEUDOCODE: SET any_axis_became_active to TRUE
        any_axis_became_active = true;
      }
    } else {
      // PSEUDOCODE: ELSE (not armed)
      // PSEUDOCODE: IF axis is less than the inactive_threshold
      if (envelope[i] < INACTIVE_THRESHOLD) {
        // PSEUDOCODE: SET member_variables.axis_armed[axis_index] to true
        axis_armed[i] = true;
      }
    }
  }

  // PSEUDOCODE: IF state_variables.in_refractory
  if (in_refractory) {
    // PSEUDOCODE: // wait a short time till it can fire again
    // PSEUDOCODE: INCREMENT member_variables.refractory_tick by delta_time
    // FIX: Capper refractory_tick pour éviter débordement
    refractory_tick = std::min(refractory_tick + delta_time_ms, MAX_REFRACTORY_TICK);
    
    // PSEUDOCODE: IF member_variables.refractory_tick is greater than refractory_period
    if (refractory_tick > REFRACTORY_PERIOD) {
      // PSEUDOCODE: SET member_variables.in_refractory to false
      in_refractory = false;
      // PSEUDOCODE: SET member_variables.refractory_tick to 0
      refractory_tick = 0;
    }
  } else {
    // PSEUDOCODE: ELSE (not in refractory)
    // PSEUDOCODE: IF any_axis_became_active
    if (any_axis_became_active) {
      // PSEUDOCODE: SET movement_detected to true
      movement_detected = true;
      // PSEUDOCODE: SET member_variables.in_refractory to true
      in_refractory = true;
    }
  }

  // PSEUDOCODE: RETURN movement_detected
  return movement_detected;
}

void MovementDetector::reset() {
  axis_armed = {true, true, true};
  in_refractory = false;
  refractory_tick = 0;
  filter.reset();
}

// ============================================================================
// MovementAnalyzer Implementation
// ============================================================================

MovementAnalyzer::MovementAnalyzer() = default;

MovementAnalyzer::AnalysisResult MovementAnalyzer::analyze(const std::array<float, 3>& gravity_corrected_xl,
                                                            uint32_t delta_time_ms) {
  AnalysisResult result;
  
  result.magnitude_active_time = mag_counter.update(gravity_corrected_xl, delta_time_ms);
  result.axis_active_time = axis_counter.update(gravity_corrected_xl, delta_time_ms);
  result.movement_detected = detector.detect(gravity_corrected_xl, delta_time_ms);
  result.any_movement = mag_counter.isMoving() || axis_counter.isMoving();

  return result;
}

void MovementAnalyzer::reset() {
  mag_counter.reset();
  axis_counter.reset();
  detector.reset();
}