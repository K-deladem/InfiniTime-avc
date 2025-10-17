#pragma once

#include <cstdint>
#include <cmath>
#include <array>

class RehabilitationAlgorithm {
public:
  struct SensorData {
    float accelX, accelY, accelZ;
    float gyroX, gyroY, gyroZ;
    uint16_t heartRate;
    uint32_t timestamp;
  };

  struct Result {
    uint8_t score;
    uint16_t movementQuality;
    uint8_t symmetry;
    bool isActiveMovement;
  };

  struct CalibrationParams {
    float accelThreshold = 0.5f;      // Seuil minimal pour détecter activité
    float qualityScale = 100.0f;      // Facteur d'échelle qualité
    uint8_t qualityWeight = 60;       // Poids qualité dans score (0-100)
    uint8_t symmetryWeight = 40;      // Poids symétrie dans score (0-100)
  };

  RehabilitationAlgorithm();
  
  Result Calculate(const SensorData& data);
  
  // Méthodes de calibration
  void SetCalibrationParams(const CalibrationParams& params) {
    calibration = params;
  }
  
  CalibrationParams GetCalibrationParams() const {
    return calibration;
  }
  
  // Reset à la calibration par défaut
  void ResetCalibration();

private:
  CalibrationParams calibration;
  
  static constexpr uint16_t BUFFER_SIZE = 50;
  std::array<float, BUFFER_SIZE> accelMagnitudeBuffer;
  uint16_t bufferIndex = 0;

  float CalculateAccelMagnitude(float ax, float ay, float az) const;
  float CalculateGyroMagnitude(float gx, float gy, float gz) const;
  bool DetectActiveMovement(float accelMag, float gyroMag) const;
  uint16_t CalculateMovementQuality(float accelMag);
  uint8_t CalculateSymmetry(float gyroX, float gyroY, float /*gyroZ*/) const;
  uint8_t CalculateFinalScore(uint16_t quality, uint8_t symmetry, bool isActive) const;
};