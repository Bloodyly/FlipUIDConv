#include "../nfc_rfid_detector_app_i.h"

typedef enum {
    NfcRfidDetectorSettingsItemReadMode,
    NfcRfidDetectorSettingsItemUidFormat,
    NfcRfidDetectorSettingsItemStart,
} NfcRfidDetectorSettingsItem;

static const char* nfc_rfid_detector_read_mode_labels[] = {
    "NFC",
    "RFID",
};

static const char* nfc_rfid_detector_uid_format_labels[] = {
    "Compact",
    "Spaced",
};

static void nfc_rfid_detector_scene_settings_read_mode_changed(VariableItem* item) {
    NfcRfidDetectorApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->read_mode = (NfcRfidDetectorReadMode)index;
    variable_item_set_current_value_text(item, nfc_rfid_detector_read_mode_labels[index]);
}

static void nfc_rfid_detector_scene_settings_uid_format_changed(VariableItem* item) {
    NfcRfidDetectorApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->uid_format = (NfcRfidDetectorUidFormat)index;
    variable_item_set_current_value_text(item, nfc_rfid_detector_uid_format_labels[index]);
}

static void nfc_rfid_detector_scene_settings_menu_callback(void* context, uint32_t index) {
    NfcRfidDetectorApp* app = context;
    if(index == NfcRfidDetectorSettingsItemStart) {
        scene_manager_next_scene(app->scene_manager, NfcRfidDetectorSceneRead);
    }
}

void nfc_rfid_detector_scene_settings_on_enter(void* context) {
    NfcRfidDetectorApp* app = context;
    VariableItemList* item_list = app->variable_item_list;

    variable_item_list_set_header(item_list, "UID Reader");

    VariableItem* item = variable_item_list_add(
        item_list,
        "Transponder",
        COUNT_OF(nfc_rfid_detector_read_mode_labels),
        nfc_rfid_detector_scene_settings_read_mode_changed,
        app);
    variable_item_set_current_value_index(item, app->read_mode);
    variable_item_set_current_value_text(item, nfc_rfid_detector_read_mode_labels[app->read_mode]);

    item = variable_item_list_add(
        item_list,
        "UID format",
        COUNT_OF(nfc_rfid_detector_uid_format_labels),
        nfc_rfid_detector_scene_settings_uid_format_changed,
        app);
    variable_item_set_current_value_index(item, app->uid_format);
    variable_item_set_current_value_text(item, nfc_rfid_detector_uid_format_labels[app->uid_format]);

    variable_item_list_add(item_list, "Start scan", 0, NULL, NULL);
    variable_item_list_set_enter_callback(
        item_list, nfc_rfid_detector_scene_settings_menu_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, NfcRfidDetectorViewVariableItemList);
}

bool nfc_rfid_detector_scene_settings_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void nfc_rfid_detector_scene_settings_on_exit(void* context) {
    NfcRfidDetectorApp* app = context;
    variable_item_list_reset(app->variable_item_list);
}
