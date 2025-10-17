#include "algorithms/RehabilitationAlgorithm.h"
#include <algorithm>
#include <nrf_log.h>

RehabilitationAlgorithm::RehabilitationAlgorithm() : bufferIndex(0) {
  accelMagnitudeBuffer.fill(0.0f);
  ResetCalibration();
}

void RehabilitationAlgorithm::ResetCalibration() {
  calibration = CalibrationParams{};
  NRF_LOG_INFO("Rehab Algorithm: Calibration reset to defaults");
}

RehabilitationAlgorithm::Result RehabilitationAlgorithm::Calculate(const SensorData& data) {
  float accelMagnitude = CalculateAccelMagnitude(data.accelX, data.accelY, data.accelZ);
  float gyroMagnitude = CalculateGyroMagnitude(data.gyroX, data.gyroY, data.gyroZ);
  
  bool isActive = DetectActiveMovement(accelMagnitude, gyroMagnitude);
  uint16_t quality = CalculateMovementQuality(accelMagnitude);
  uint8_t symmetry = CalculateSymmetry(data.gyroX, data.gyroY, data.gyroZ);
  uint8_t score = CalculateFinalScore(quality, symmetry, isActive);
  
  return Result{
    .score = score,
    .movementQuality = quality,
    .symmetry = symmetry,
    .isActiveMovement = isActive
  };
}

float RehabilitationAlgorithm::CalculateAccelMagnitude(float ax, float ay, float az) const {
  return std::sqrt(ax * ax + ay * ay + az * az);
}

float RehabilitationAlgorithm::CalculateGyroMagnitude(float gx, float gy, float gz) const {
  return std::sqrt(gx * gx + gy * gy + gz * gz);
}

bool RehabilitationAlgorithm::DetectActiveMovement(float accelMag, float gyroMag) const {
  return (accelMag > calibration.accelThreshold) || (gyroMag > 10.0f);
}

uint16_t RehabilitationAlgorithm::CalculateMovementQuality(float accelMag) {
  accelMagnitudeBuffer[bufferIndex] = accelMag;
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
  
  float sum = 0.0f;
  for (const auto& val : accelMagnitudeBuffer) {
    sum += val;
  }
  float average = sum / BUFFER_SIZE;
  
  uint16_t quality = static_cast<uint16_t>(average * calibration.qualityScale);
  return std::min(quality, uint16_t(1000));
}

uint8_t RehabilitationAlgorithm::CalculateSymmetry(float gyroX, float gyroY, float /*gyroZ*/) const {
  float absX = std::abs(gyroX);
  float absY = std::abs(gyroY);
  
  if (absX + absY < 0.1f) {
    return 100;
  }
  
  float ratio = std::min(absX, absY) / std::max(absX, absY);
  uint8_t symmetry = static_cast<uint8_t>(ratio * 100.0f);
  
  return std::min(symmetry, uint8_t(100));
}

uint8_t RehabilitationAlgorithm::CalculateFinalScore(uint16_t quality, uint8_t symmetry, bool isActive) const {
  if (!isActive) {
    return 0;
  }
  
  uint16_t qualityNormalized = std::min(static_cast<uint16_t>(quality / 10), uint16_t(100));
  
  uint16_t combinedScore = (qualityNormalized * calibration.qualityWeight) / 100 + 
                           (static_cast<uint16_t>(symmetry) * calibration.symmetryWeight) / 100;
  
  return static_cast<uint8_t>(std::min(combinedScore, uint16_t(100)));
}