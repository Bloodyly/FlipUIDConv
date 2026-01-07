#include "../nfc_rfid_detector_app_i.h"

static void nfc_rfid_detector_scene_read_show_prompt(NfcRfidDetectorApp* app) {
    widget_reset(app->widget);

    FuriString* text = furi_string_alloc();
    const char* mode = (app->read_mode == NfcRfidDetectorReadModeNfc) ? "NFC" : "RFID";
    const char* format =
        (app->uid_format == NfcRfidDetectorUidFormatSpaced) ? "Spaced HEX" : "Compact HEX";

    furi_string_printf(text, "Scan %s tag\nUID format: %s\n\nHold tag close.", mode, format);
    widget_add_text_box_element(
        app->widget, 0, 0, 128, 52, AlignLeft, AlignTop, furi_string_get_cstr(text), true);
    furi_string_free(text);
}

static void nfc_rfid_detector_scene_read_show_uid(NfcRfidDetectorApp* app) {
    widget_reset(app->widget);

    FuriString* text = furi_string_alloc();
    furi_string_printf(
        text, "UID (HEX):\n%s", nfc_rfid_detector_app_get_uid_string(app));
    widget_add_text_box_element(
        app->widget, 0, 0, 128, 52, AlignLeft, AlignTop, furi_string_get_cstr(text), true);
    furi_string_free(text);
}

void nfc_rfid_detector_scene_read_on_enter(void* context) {
    NfcRfidDetectorApp* app = context;

    nfc_rfid_detector_scene_read_show_prompt(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, NfcRfidDetectorViewWidget);

    nfc_rfid_detector_app_scan_start(app);
}

bool nfc_rfid_detector_scene_read_on_event(void* context, SceneManagerEvent event) {
    NfcRfidDetectorApp* app = context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcRfidDetectorCustomEventUidDetected) {
            notification_message(app->notifications, &sequence_success);
            nfc_rfid_detector_scene_read_show_uid(app);
            return true;
        }
    }

    return false;
}

void nfc_rfid_detector_scene_read_on_exit(void* context) {
    NfcRfidDetectorApp* app = context;
    nfc_rfid_detector_app_scan_stop(app);
    widget_reset(app->widget);
}
