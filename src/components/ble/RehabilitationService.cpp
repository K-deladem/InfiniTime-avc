#include "components/ble/RehabilitationService.h"
#include "components/ble/NimbleController.h"
#include <nrf_log.h>

using namespace Pinetime::Controllers;

namespace {
  int RehabServiceCallback(uint16_t /*conn_handle*/, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    auto* rehabService = static_cast<RehabilitationService*>(arg);
    return rehabService->OnRehabDataRequested(attr_handle, ctxt);
  }
}

RehabilitationService::RehabilitationService(NimbleController& nimble)
  : nimble {nimble},
    characteristicDefinition {
      // Score characteristic
      {.uuid = &scoreCharUuid.u,
       .access_cb = RehabServiceCallback,
       .arg = this,
       .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
       .val_handle = &scoreHandle},
      
      // Movement characteristic
      {.uuid = &movementCharUuid.u,
       .access_cb = RehabServiceCallback,
       .arg = this,
       .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
       .val_handle = &movementHandle},
      
      // Symmetry characteristic
      {.uuid = &symmetryCharUuid.u,
       .access_cb = RehabServiceCallback,
       .arg = this,
       .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
       .val_handle = &symmetryHandle},
      
      {0}  // Terminator
    },
    serviceDefinition {
      {.type = BLE_GATT_SVC_TYPE_PRIMARY,
       .uuid = &rehabServiceUuid.u,
       .characteristics = characteristicDefinition},
      {0}  // Terminator
    } {
}

void RehabilitationService::Init() {
  int res = 0;
  res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);

  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);
  
  NRF_LOG_INFO("RehabilitationService initialized");
}

int RehabilitationService::OnRehabDataRequested(uint16_t attributeHandle, ble_gatt_access_ctxt* context) {
  // Callback quand un client demande une lecture
  
  if (attributeHandle == scoreHandle) {
    NRF_LOG_INFO("Rehab Score read: %u", cachedScore);
    int res = os_mbuf_append(context->om, &cachedScore, sizeof(cachedScore));
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  } 
  else if (attributeHandle == movementHandle) {
    NRF_LOG_INFO("Rehab Movement read: %u", cachedMovement);
    // Attention : little-endian sur BLE
    uint16_t movement_le = cachedMovement;  // Assuming little-endian platform
    int res = os_mbuf_append(context->om, &movement_le, sizeof(movement_le));
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  } 
  else if (attributeHandle == symmetryHandle) {
    NRF_LOG_INFO("Rehab Symmetry read: %u", cachedSymmetry);
    int res = os_mbuf_append(context->om, &cachedSymmetry, sizeof(cachedSymmetry));
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  
  return 0;
}

void RehabilitationService::OnNewRehabData(uint8_t score, uint16_t movement, uint8_t symmetry) {
  // Mettre à jour les valeurs cachées
  cachedScore = score;
  cachedMovement = movement;
  cachedSymmetry = symmetry;
  
  // Si les notifications ne sont pas activées, ne pas envoyer
  if (!rehabNotificationEnabled) {
    return;
  }

  uint16_t connectionHandle = nimble.connHandle();

  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return;
  }

  // Envoyer le score
  {
    auto* om = ble_hs_mbuf_from_flat(&score, sizeof(score));
    ble_gattc_notify_custom(connectionHandle, scoreHandle, om);
  }

  // Envoyer le mouvement
  {
    uint16_t movement_le = movement;
    auto* om = ble_hs_mbuf_from_flat(&movement_le, sizeof(movement_le));
    ble_gattc_notify_custom(connectionHandle, movementHandle, om);
  }

  // Envoyer la symétrie
  {
    auto* om = ble_hs_mbuf_from_flat(&symmetry, sizeof(symmetry));
    ble_gattc_notify_custom(connectionHandle, symmetryHandle, om);
  }
  
  NRF_LOG_INFO("Rehab Data sent - Score: %u, Movement: %u, Symmetry: %u", score, movement, symmetry);
}

void RehabilitationService::SubscribeNotification(uint16_t /*attributeHandle*/) {
  rehabNotificationEnabled = true;
  NRF_LOG_INFO("Rehab notifications subscribed");
}

void RehabilitationService::UnsubscribeNotification(uint16_t /*attributeHandle*/) {
  rehabNotificationEnabled = false;
  NRF_LOG_INFO("Rehab notifications unsubscribed");
}

bool RehabilitationService::IsRehabNotificationSubscribed() const {
  return rehabNotificationEnabled;
}