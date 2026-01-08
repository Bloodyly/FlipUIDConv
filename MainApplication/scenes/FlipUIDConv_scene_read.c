#include "../FlipUIDConv_app_i.h"

static void FlipUIDConv_scene_read_show_prompt(FlipUIDConvApp* app) {
    widget_reset(app->widget);

    FuriString* text = furi_string_alloc();
    const char* mode = (app->read_mode == FlipUIDConvReadModeNfc) ? "NFC" : "RFID";
    static const char* FlipUIDConv_uid_format_labels[] = {
        "Compact",
        "Spaced",
        "Hex10",
        "Hex8/COMLock",
        "IK3/IS",
        "IK2",
        "ZK/Codier",
        "IS",
    };
    const char* format = FlipUIDConv_uid_format_labels[app->uid_format];

    furi_string_printf(text, "Scan %s tag\nUID format: %s\n\nHold tag close.", mode, format);
    widget_add_text_box_element(
        app->widget, 0, 0, 128, 52, AlignLeft, AlignTop, furi_string_get_cstr(text), true);
    furi_string_free(text);
}

static void FlipUIDConv_scene_read_show_uid(FlipUIDConvApp* app) {
    widget_reset(app->widget);

    widget_add_string_element(
        app->widget, 0, 0, AlignLeft, AlignTop, FontPrimary, "UID (HEX):");
    widget_add_string_element(
        app->widget,
        0,
        14,
        AlignLeft,
        AlignTop,
        FontPrimary,
        FlipUIDConv_app_get_uid_string(app));
    widget_add_string_element(
        app->widget, 0, 30, AlignLeft, AlignTop, FontPrimary, "Type:");
    widget_add_string_element(
        app->widget,
        0,
        44,
        AlignLeft,
        AlignTop,
        FontPrimary,
        furi_string_get_cstr(app->tag_type_string));
}

void FlipUIDConv_scene_read_on_enter(void* context) {
    FlipUIDConvApp* app = context;

    FlipUIDConv_scene_read_show_prompt(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipUIDConvViewWidget);

    app->led_tag_found = false;
    FlipUIDConv_app_scan_start(app);
}

bool FlipUIDConv_scene_read_on_event(void* context, SceneManagerEvent event) {
    FlipUIDConvApp* app = context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == FlipUIDConvCustomEventUidDetected) {
            if(app->uid_ready) {
                app->led_tag_found = true;
                notification_message(app->notifications, &sequence_success);
            }
            FlipUIDConv_scene_read_show_uid(app);
            return true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        if(app->scanning && !app->led_tag_found) {
            notification_message(app->notifications, &sequence_blink_blue_10);
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        scene_manager_previous_scene(app->scene_manager);
        return true;
    }

    return false;
}

void FlipUIDConv_scene_read_on_exit(void* context) {
    FlipUIDConvApp* app = context;
    FlipUIDConv_app_scan_stop(app);
    app->led_tag_found = false;
    notification_message(app->notifications, &sequence_reset_rgb);
    widget_reset(app->widget);
}
