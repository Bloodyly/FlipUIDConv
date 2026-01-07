#include "nfc_rfid_detector_app_i.h"

#include <furi.h>
#include <furi_hal.h>

static bool nfc_rfid_detector_app_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    NfcRfidDetectorApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool nfc_rfid_detector_app_back_event_callback(void* context) {
    furi_assert(context);
    NfcRfidDetectorApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void nfc_rfid_detector_app_tick_event_callback(void* context) {
    furi_assert(context);
    NfcRfidDetectorApp* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

NfcRfidDetectorApp* nfc_rfid_detector_app_alloc() {
    NfcRfidDetectorApp* app = malloc(sizeof(NfcRfidDetectorApp));

    // GUI
    app->gui = furi_record_open(RECORD_GUI);

    // View Dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&nfc_rfid_detector_scene_handlers, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, nfc_rfid_detector_app_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, nfc_rfid_detector_app_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, nfc_rfid_detector_app_tick_event_callback, 100);

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Open Notification record
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    // SubMenu
    app->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        NfcRfidDetectorViewVariableItemList,
        variable_item_list_get_view(app->variable_item_list));

    // Widget
    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, NfcRfidDetectorViewWidget, widget_get_view(app->widget));

    app->uid_string = furi_string_alloc();
    app->read_mode = NfcRfidDetectorReadModeNfc;
    app->uid_format = NfcRfidDetectorUidFormatSpaced;
    app->scanning = false;
    app->uid_ready = false;
    app->scan_thread = NULL;
    app->rfid_protocol_id = PROTOCOL_NO;

    scene_manager_next_scene(app->scene_manager, NfcRfidDetectorSceneSettings);

    return app;
}

void nfc_rfid_detector_app_free(NfcRfidDetectorApp* app) {
    furi_assert(app);

    // Variable item list
    view_dispatcher_remove_view(app->view_dispatcher, NfcRfidDetectorViewVariableItemList);
    variable_item_list_free(app->variable_item_list);

    //  Widget
    view_dispatcher_remove_view(app->view_dispatcher, NfcRfidDetectorViewWidget);
    widget_free(app->widget);

    nfc_rfid_detector_app_scan_stop(app);
    furi_string_free(app->uid_string);

    // View dispatcher
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);
    app->notifications = NULL;

    // Close records
    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t nfc_rfid_detector_app(void* p) {
    UNUSED(p);
    NfcRfidDetectorApp* nfc_rfid_detector_app = nfc_rfid_detector_app_alloc();

    view_dispatcher_run(nfc_rfid_detector_app->view_dispatcher);

    nfc_rfid_detector_app_free(nfc_rfid_detector_app);

    return 0;
}
