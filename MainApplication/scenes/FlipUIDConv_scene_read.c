#include "../FlipUIDConv_app_i.h"

#include <stdio.h>

#include "../images/FlipUIDConv_icons.h"

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

static const NotificationSequence FlipUIDConv_sequence_set_magenta_255 = {
    &message_red_255,
    &message_blue_255,
    &message_do_not_reset,
    NULL,
};

static void FlipUIDConv_scene_read_set_scan_led(FlipUIDConvApp* app) {
    if(!app->scanning) {
        return;
    }

    if(app->read_mode == FlipUIDConvReadModeNfc) {
        notification_message(app->notifications, &sequence_set_blue_255);
    } else {
        notification_message(app->notifications, &FlipUIDConv_sequence_set_magenta_255);
    }
}

static void FlipUIDConv_scene_read_update_display(FlipUIDConvApp* app) {
    widget_reset(app->widget);

    const char* scan_text =
        (app->read_mode == FlipUIDConvReadModeNfc) ? "Scanning for NFC" : "Scanning for RFID";
    const char* format = FlipUIDConv_uid_format_labels[app->uid_format];
    const char* uid_text = furi_string_size(app->uid_string) ? FlipUIDConv_app_get_uid_string(app) :
                                                               "...";
    const char* tag_text =
        furi_string_size(app->tag_type_string) ? furi_string_get_cstr(app->tag_type_string) : "-";
    const Icon* usb_icon =
        app->usb_status_connected ? &I_FlipUIDConv_UsbOn_10x10 : &I_FlipUIDConv_UsbOff_10x10;
    const Icon* sound_icon =
        app->sound_enabled ? &I_FlipUIDConv_Sound_10x10 : &I_FlipUIDConv_Mute_10x10;

    widget_add_string_element(
        app->widget, 0, 0, AlignLeft, AlignTop, FontPrimary, scan_text);
    widget_add_icon_element(app->widget, 108, 0, sound_icon);
    widget_add_icon_element(app->widget, 118, 0, usb_icon);

    char format_line[32];
    snprintf(format_line, sizeof(format_line), "Format: %s", format);
    widget_add_string_element(
        app->widget, 0, 12, AlignLeft, AlignTop, FontPrimary, format_line);

    char tag_line[32];
    snprintf(tag_line, sizeof(tag_line), "Type: %s", tag_text);
    widget_add_string_element(
        app->widget, 0, 24, AlignLeft, AlignTop, FontPrimary, tag_line);

    widget_add_string_element(
        app->widget, 0, 36, AlignLeft, AlignTop, FontPrimary, uid_text);
}

static bool FlipUIDConv_scene_read_input_callback(InputEvent* event, void* context) {
    FlipUIDConvApp* app = context;
    if(event->type != InputTypeShort) {
        return false;
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
        return true;
    }

    return false;
}

void FlipUIDConv_scene_read_on_enter(void* context) {
    FlipUIDConvApp* app = context;

    app->usb_status_connected = furi_hal_hid_is_connected();
    app->scan_led_active = false;
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
            app->uid_bytes_len = 0;
            furi_string_reset(app->tag_type_string);
            FlipUIDConv_app_scan_start(app);
            app->scan_led_active = false;
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
            FlipUIDConv_app_refresh_uid(app);
            FlipUIDConv_scene_read_update_display(app);
            return true;
        case FlipUIDConvCustomEventInputRight:
            app->uid_format = (app->uid_format + 1) % COUNT_OF(FlipUIDConv_uid_format_labels);
            FlipUIDConv_app_refresh_uid(app);
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
            if(!app->scan_led_active) {
                app->scan_led_active = true;
                FlipUIDConv_scene_read_set_scan_led(app);
            }
        } else {
            app->scan_led_active = false;
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
    app->scan_led_active = false;
    app->uid_ready = false;
    app->uid_bytes_len = 0;
    furi_string_reset(app->uid_string);
    furi_string_reset(app->tag_type_string);
    notification_message(app->notifications, &sequence_reset_rgb);
    widget_reset(app->widget);
}
