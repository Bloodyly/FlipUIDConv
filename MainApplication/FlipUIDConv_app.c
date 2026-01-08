#include "FlipUIDConv_app_i.h"

#include <furi.h>
#include <furi_hal.h>

static bool FlipUIDConv_app_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    FlipUIDConvApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool FlipUIDConv_app_back_event_callback(void* context) {
    furi_assert(context);
    FlipUIDConvApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void FlipUIDConv_app_tick_event_callback(void* context) {
    furi_assert(context);
    FlipUIDConvApp* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

FlipUIDConvApp* FlipUIDConv_app_alloc() {
    FlipUIDConvApp* app = malloc(sizeof(FlipUIDConvApp));

    // GUI
    app->gui = furi_record_open(RECORD_GUI);

    // View Dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&FlipUIDConv_scene_handlers, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, FlipUIDConv_app_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, FlipUIDConv_app_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, FlipUIDConv_app_tick_event_callback, 100);

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Open Notification record
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    app->usb_prev_config = furi_hal_usb_get_config();
    furi_hal_usb_unlock();
    furi_check(furi_hal_usb_set_config(&usb_hid, NULL) == true);

    // SubMenu
    app->variable_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        FlipUIDConvViewVariableItemList,
        variable_item_list_get_view(app->variable_item_list));

    // Widget
    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, FlipUIDConvViewWidget, widget_get_view(app->widget));

    app->uid_string = furi_string_alloc();
    app->tag_type_string = furi_string_alloc();
    app->read_mode = FlipUIDConvReadModeNfc;
    app->uid_format = FlipUIDConvUidFormatSpaced;
    app->scanning = false;
    app->uid_ready = false;
    app->scan_thread = NULL;
    app->rfid_protocol_id = PROTOCOL_NO;
    app->usb_status_item = NULL;
    app->usb_status_connected = false;
    app->led_tag_found = false;
    app->sound_enabled = true;
    app->uid_bytes_len = 0;
    app->scan_led_active = false;

    scene_manager_next_scene(app->scene_manager, FlipUIDConvSceneSettings);

    return app;
}

void FlipUIDConv_app_free(FlipUIDConvApp* app) {
    furi_assert(app);

    // Variable item list
    view_dispatcher_remove_view(app->view_dispatcher, FlipUIDConvViewVariableItemList);
    variable_item_list_free(app->variable_item_list);

    //  Widget
    view_dispatcher_remove_view(app->view_dispatcher, FlipUIDConvViewWidget);
    widget_free(app->widget);

    FlipUIDConv_app_scan_stop(app);
    furi_string_free(app->uid_string);
    furi_string_free(app->tag_type_string);

    // View dispatcher
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    if(app->usb_prev_config) {
        furi_hal_usb_set_config(app->usb_prev_config, NULL);
        app->usb_prev_config = NULL;
    }

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);
    app->notifications = NULL;

    // Close records
    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t FlipUIDConv_app(void* p) {
    UNUSED(p);
    FlipUIDConvApp* FlipUIDConv_app = FlipUIDConv_app_alloc();

    view_dispatcher_run(FlipUIDConv_app->view_dispatcher);

    FlipUIDConv_app_free(FlipUIDConv_app);

    return 0;
}
