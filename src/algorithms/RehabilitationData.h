#pragma once
#include <cstdint>
#include <array>

class RehabilitationData {
public:
  struct DailyRecord {
    uint32_t timestamp;      // Date/heure
    uint8_t maxScore;        // Meilleur score du jour
    uint16_t avgMovement;    // Mouvement moyen
    uint32_t totalDuration;  // Durée totale d'activité (secondes)
    uint16_t sessionCount;   // Nombre de sessions
  };

  static constexpr uint16_t HISTORY_SIZE = 30;  // 30 jours

  RehabilitationData();
  
  // Ajouter une mesure
  void AddMeasurement(uint8_t score, uint16_t movement);
  
  // Obtenir les stats du jour
  DailyRecord GetTodayStats() const;
  
  // Obtenir l'historique
  const std::array<DailyRecord, HISTORY_SIZE>& GetHistory() const {
    return dailyHistory;
  }
  
  // Charger/sauvegarder
  void Load();
  void Save();

private:
  std::array<DailyRecord, HISTORY_SIZE> dailyHistory = {};
  uint16_t currentDayIndex = 0;
  uint32_t lastMeasurementTime = 0;
  bool isDirty = false;  // Flag pour savoir si on doit sauvegarder
};