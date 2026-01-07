#pragma once

#include "helpers/nfc_rfid_detector_types.h"
#include "helpers/nfc_rfid_detector_event.h"

#include "scenes/nfc_rfid_detector_scene.h"
#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include <notification/notification_messages.h>
#include <nfc/nfc.h>
#include <nfc/nfc_poller.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a.h>
#include <nfc/protocols/iso14443_3a/iso14443_3a_poller.h>
#include <lfrfid/lfrfid_worker.h>
#include <lfrfid/protocols/lfrfid_protocols.h>
#include <toolbox/protocols/protocol_dict.h>

#define ISO14443_3A_ASYNC_FLAG_COMPLETE (1UL << 0)

typedef struct NfcRfidDetectorApp NfcRfidDetectorApp;

struct NfcRfidDetectorApp {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    NotificationApp* notifications;
    VariableItemList* variable_item_list;
    Widget* widget;
    FuriThread* scan_thread;
    bool scanning;
    bool uid_ready;
    NfcRfidDetectorReadMode read_mode;
    NfcRfidDetectorUidFormat uid_format;
    FuriString* uid_string;
    ProtocolId rfid_protocol_id;
};

void nfc_rfid_detector_app_scan_start(NfcRfidDetectorApp* app);
void nfc_rfid_detector_app_scan_stop(NfcRfidDetectorApp* app);
const char* nfc_rfid_detector_app_get_uid_string(NfcRfidDetectorApp* app);
void nfc_rfid_detector_app_send_uid_hid(NfcRfidDetectorApp* app);
