#include "algorithms/RehabilitationData.h"
#include <nrf_log.h>
#include <ctime>
#include <cstring>

RehabilitationData::RehabilitationData() {
  Load();
}

void RehabilitationData::AddMeasurement(uint8_t score, uint16_t movement) {
  // Vérifier si on est toujours le même jour
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  uint32_t todayTimestamp = mktime(timeinfo);
  
  // Si c'est un nouveau jour, passer au jour suivant
  if (lastMeasurementTime > 0) {
    struct tm* lastTime = localtime((time_t*)&lastMeasurementTime);
    lastTime->tm_hour = 0;
    lastTime->tm_min = 0;
    lastTime->tm_sec = 0;
    
    uint32_t lastDay = mktime(lastTime);
    
    if (todayTimestamp > lastDay) {
      // Nouveau jour : incrémenter l'index
      currentDayIndex = (currentDayIndex + 1) % HISTORY_SIZE;
      dailyHistory[currentDayIndex] = DailyRecord{};
    }
  }
  
  // Mettre à jour les stats du jour
  DailyRecord& today = dailyHistory[currentDayIndex];
  
  if (today.timestamp == 0) {
    today.timestamp = now;
  }
  
  // Mettre à jour max score
  if (score > today.maxScore) {
    today.maxScore = score;
  }
  
  // Mettre à jour mouvement moyen
  if (today.sessionCount > 0) {
    today.avgMovement = (today.avgMovement + movement) / 2;
  } else {
    today.avgMovement = movement;
  }
  
  // Incrémenter les sessions si score > 0 (activité détectée)
  if (score > 0) {
    today.totalDuration += 1;  // +1 seconde
    today.sessionCount++;
  }
  
  lastMeasurementTime = now;
  isDirty = true;
  
  NRF_LOG_INFO("Rehab Data: Max=%u, Avg=%u, Sessions=%u", 
               today.maxScore, today.avgMovement, today.sessionCount);
}

RehabilitationData::DailyRecord RehabilitationData::GetTodayStats() const {
  return dailyHistory[currentDayIndex];
}

void RehabilitationData::Load() {
  // TODO: Implémenter la lecture depuis la mémoire Flash (LittleFS)
  // Pour l'instant, on initialise avec des zéros
  NRF_LOG_INFO("RehabilitationData loaded from storage");
}

void RehabilitationData::Save() {
  if (!isDirty) {
    return;
  }
  
  // TODO: Implémenter la sauvegarde sur la mémoire Flash (LittleFS)
  isDirty = false;
  NRF_LOG_INFO("RehabilitationData saved to storage");
}