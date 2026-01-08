#include "../FlipUIDConv_app_i.h"

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

static void FlipUIDConv_scene_read_update_display(FlipUIDConvApp* app) {
    widget_reset(app->widget);

    const char* mode = (app->read_mode == FlipUIDConvReadModeNfc) ? "NFC" : "RFID";
    const char* format = FlipUIDConv_uid_format_labels[app->uid_format];
    const char* uid_text = furi_string_size(app->uid_string) ? FlipUIDConv_app_get_uid_string(app) :
                                                               "...";
    const char* usb_text = app->usb_status_connected ? "USB" : "USB-";
    const char* sound_text = app->sound_enabled ? "SND" : "MUT";

    widget_add_string_element(
        app->widget, 0, 10, AlignLeft, AlignTop, FontPrimary, mode);
    widget_add_string_element(
        app->widget, 100, 10, AlignRight, AlignTop, FontSecondary, sound_text);
    widget_add_string_element(
        app->widget, 127, 10, AlignRight, AlignTop, FontSecondary, usb_text);

    FuriString* format_line = furi_string_alloc_printf("Format: %s", format);
    widget_add_string_element(
        app->widget, 0, 26, AlignLeft, AlignTop, FontPrimary, furi_string_get_cstr(format_line));
    furi_string_free(format_line);

    widget_add_string_element(
        app->widget, 0, 42, AlignLeft, AlignTop, FontPrimary, uid_text);
}

static void FlipUIDConv_scene_read_input_callback(InputEvent* event, void* context) {
    FlipUIDConvApp* app = context;
    if(event->type != InputTypeShort) {
        return;
    }

    uint32_t custom_event = 0;
    switch(event->key) {
    case InputKeyUp:
        custom_event = FlipUIDConvCustomEventInputUp;
        break;
    case InputKeyDown:
        custom_event = FlipUIDConvCustomEventInputDown;
        break;
    case InputKeyLeft:
        custom_event = FlipUIDConvCustomEventInputLeft;
        break;
    case InputKeyRight:
        custom_event = FlipUIDConvCustomEventInputRight;
        break;
    case InputKeyOk:
        custom_event = FlipUIDConvCustomEventInputOk;
        break;
    default:
        break;
    }

    if(custom_event) {
        view_dispatcher_send_custom_event(app->view_dispatcher, custom_event);
    }
}

void FlipUIDConv_scene_read_on_enter(void* context) {
    FlipUIDConvApp* app = context;

    app->usb_status_connected = furi_hal_hid_is_connected();
    FlipUIDConv_scene_read_update_display(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FlipUIDConvViewWidget);
    view_set_context(widget_get_view(app->widget), app);
    view_set_input_callback(widget_get_view(app->widget), FlipUIDConv_scene_read_input_callback);

    app->led_tag_found = false;
    FlipUIDConv_app_scan_start(app);
}

bool FlipUIDConv_scene_read_on_event(void* context, SceneManagerEvent event) {
    FlipUIDConvApp* app = context;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case FlipUIDConvCustomEventUidDetected:
            if(app->uid_ready) {
                app->led_tag_found = true;
                notification_message(
                    app->notifications,
                    app->sound_enabled ? &sequence_success : &sequence_blink_green_10);
            }
            FlipUIDConv_scene_read_update_display(app);
            return true;
        case FlipUIDConvCustomEventInputUp:
            FlipUIDConv_app_scan_stop(app);
            app->read_mode = (app->read_mode == FlipUIDConvReadModeNfc) ? FlipUIDConvReadModeRfid :
                                                                          FlipUIDConvReadModeNfc;
            app->led_tag_found = false;
            app->uid_ready = false;
            furi_string_reset(app->uid_string);
            FlipUIDConv_app_scan_start(app);
            FlipUIDConv_scene_read_update_display(app);
            return true;
        case FlipUIDConvCustomEventInputDown:
            app->sound_enabled = !app->sound_enabled;
            FlipUIDConv_scene_read_update_display(app);
            return true;
        case FlipUIDConvCustomEventInputLeft:
            if(app->uid_format == 0) {
                app->uid_format = COUNT_OF(FlipUIDConv_uid_format_labels) - 1;
            } else {
                app->uid_format--;
            }
            app->uid_ready = false;
            furi_string_reset(app->uid_string);
            FlipUIDConv_scene_read_update_display(app);
            return true;
        case FlipUIDConvCustomEventInputRight:
            app->uid_format = (app->uid_format + 1) % COUNT_OF(FlipUIDConv_uid_format_labels);
            app->uid_ready = false;
            furi_string_reset(app->uid_string);
            FlipUIDConv_scene_read_update_display(app);
            return true;
        case FlipUIDConvCustomEventInputOk:
            FlipUIDConv_app_send_hid(app);
            return true;
        default:
            break;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        bool usb_connected = furi_hal_hid_is_connected();
        if(app->usb_status_connected != usb_connected) {
            app->usb_status_connected = usb_connected;
            FlipUIDConv_scene_read_update_display(app);
        }
        if(app->scanning && !app->led_tag_found) {
            notification_message(app->notifications, &sequence_blink_blue_100);
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
