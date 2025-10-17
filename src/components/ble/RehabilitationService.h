#pragma once
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#undef max
#undef min
#include <atomic>
#include <cstdint>

namespace Pinetime {
  namespace Controllers {
    class NimbleController;

    class RehabilitationService {
    public:
      RehabilitationService(NimbleController& nimble);
      void Init();
      
      // Callbacks pour les données
      int OnRehabDataRequested(uint16_t attributeHandle, ble_gatt_access_ctxt* context);
      
      // Envoyer les notifications
      void OnNewRehabData(uint8_t score, uint16_t movement, uint8_t symmetry);

      // Gestion des subscriptions
      void SubscribeNotification(uint16_t attributeHandle);
      void UnsubscribeNotification(uint16_t attributeHandle);
      bool IsRehabNotificationSubscribed() const;

    private:
      NimbleController& nimble;

      // UUID custom (service 0006) :
      // Base : xxxxxxxx-78fc-48fe-8e23-433b3a1942d0
      // Service : 00060000-78fc-48fe-8e23-433b3a1942d0
      // Score :  00060001-78fc-48fe-8e23-433b3a1942d0
      // Movement : 00060002-78fc-48fe-8e23-433b3a1942d0
      // Symmetry : 00060003-78fc-48fe-8e23-433b3a1942d0

      // UUIDs définis comme constexpr (pas d'appel de fonction)
      static constexpr ble_uuid128_t rehabServiceUuid {.u = {.type = BLE_UUID_TYPE_128},
                                                        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 
                                                                 0xfe, 0x48, 0xfc, 0x78, 0x00, 0x00, 0x06, 0x00}};
      
      static constexpr ble_uuid128_t scoreCharUuid {.u = {.type = BLE_UUID_TYPE_128},
                                                     .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 
                                                              0xfe, 0x48, 0xfc, 0x78, 0x01, 0x00, 0x06, 0x00}};
      
      static constexpr ble_uuid128_t movementCharUuid {.u = {.type = BLE_UUID_TYPE_128},
                                                        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 
                                                                 0xfe, 0x48, 0xfc, 0x78, 0x02, 0x00, 0x06, 0x00}};
      
      static constexpr ble_uuid128_t symmetryCharUuid {.u = {.type = BLE_UUID_TYPE_128},
                                                        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 
                                                                 0xfe, 0x48, 0xfc, 0x78, 0x03, 0x00, 0x06, 0x00}};

      struct ble_gatt_chr_def characteristicDefinition[4];
      struct ble_gatt_svc_def serviceDefinition[2];

      uint16_t scoreHandle = 0;
      uint16_t movementHandle = 0;
      uint16_t symmetryHandle = 0;
      
      // Cached values pour les lectures
      uint8_t cachedScore = 0;
      uint16_t cachedMovement = 0;
      uint8_t cachedSymmetry = 0;
      
      std::atomic_bool rehabNotificationEnabled {false};
    };
  }
}