#pragma once

#include "helpers/FlipUIDConv_types.h"
#include "helpers/FlipUIDConv_event.h"

#include "scenes/FlipUIDConv_scene.h"
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

typedef struct FlipUIDConvApp FlipUIDConvApp;

struct FlipUIDConvApp {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    NotificationApp* notifications;
    VariableItemList* variable_item_list;
    Widget* widget;
    FuriThread* scan_thread;
    bool scanning;
    bool uid_ready;
    FlipUIDConvReadMode read_mode;
    FlipUIDConvUidFormat uid_format;
    FuriString* uid_string;
    ProtocolId rfid_protocol_id;
};

void FlipUIDConv_app_scan_start(FlipUIDConvApp* app);
void FlipUIDConv_app_scan_stop(FlipUIDConvApp* app);
const char* FlipUIDConv_app_get_uid_string(FlipUIDConvApp* app);
