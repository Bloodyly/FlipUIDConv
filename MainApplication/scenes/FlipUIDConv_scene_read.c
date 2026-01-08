#include "../FlipUIDConv_app_i.h"

static void FlipUIDConv_scene_read_show_prompt(FlipUIDConvApp* app) {
    widget_reset(app->widget);

    FuriString* text = furi_string_alloc();
    const char* mode = (app->read_mode == FlipUIDConvReadModeNfc) ? "NFC" : "RFID";
    const char* format =
        (app->uid_format == FlipUIDConvUidFormatSpaced) ? "Spaced HEX" : "Compact HEX";

    furi_string_printf(text, "Scan %s tag\nUID format: %s\n\nHold tag close.", mode, format);
    widget_add_text_box_element(
        app->widget, 0, 0, 128, 52, AlignLeft, AlignTop, furi_string_get_cstr(text), true);
    furi_string_free(text);
}

static void FlipUIDConv_scene_read_show_uid(FlipUIDConvApp* app) {
    widget_reset(app->widget);

    FuriString* text = furi_string_alloc();
    furi_string_printf(
        text, "UID (HEX):\n%s", FlipUIDConv_app_get_uid_string(app));
    widget_add_text_box_element(
        app->widget, 0, 0, 128, 52, AlignLeft, AlignTop, furi_string_get_cstr(text), true);
    furi_string_free(text);
}

void FlipUIDConv_scene_read_on_enter(void* context) {
    FlipUIDConvApp* app = context;

    FlipUIDConv_scene_read_show_prompt(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipUIDConvViewWidget);

    FlipUIDConv_app_scan_start(app);
}

bool FlipUIDConv_scene_read_on_event(void* context, SceneManagerEvent event) {
    FlipUIDConvApp* app = context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == FlipUIDConvCustomEventUidDetected) {
            notification_message(app->notifications, &sequence_success);
            FlipUIDConv_scene_read_show_uid(app);
            return true;
        }
    }

    return false;
}

void FlipUIDConv_scene_read_on_exit(void* context) {
    FlipUIDConvApp* app = context;
    FlipUIDConv_app_scan_stop(app);
    widget_reset(app->widget);
}
