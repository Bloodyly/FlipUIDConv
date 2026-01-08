#include "../FlipUIDConv_app_i.h"

typedef enum {
    FlipUIDConvSettingsItemReadMode,
    FlipUIDConvSettingsItemUidFormat,
    FlipUIDConvSettingsItemOutputMode,
} FlipUIDConvSettingsItem;

static const char* FlipUIDConv_read_mode_labels[] = {
    "NFC",
    "RFID",
};

static const char* FlipUIDConv_uid_format_labels[] = {
    "Compact",
    "Spaced",
    "Hex10",
    "Hex8",
    "IK3/IS",
    "IK2",
    "ZK/Codier",
    "ZX/UA",
    "Hitag",
    "Wiegand32",
    "Wiegand26",
    "MSB",
    "LSB",
};

static const char* FlipUIDConv_output_mode_labels[] = {
    "Screen",
    "USB HID",
};

static void FlipUIDConv_scene_settings_read_mode_changed(VariableItem* item) {
    FlipUIDConvApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->read_mode = (FlipUIDConvReadMode)index;
    variable_item_set_current_value_text(item, FlipUIDConv_read_mode_labels[index]);
}

static void FlipUIDConv_scene_settings_uid_format_changed(VariableItem* item) {
    FlipUIDConvApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->uid_format = (FlipUIDConvUidFormat)index;
    variable_item_set_current_value_text(item, FlipUIDConv_uid_format_labels[index]);
}

static void FlipUIDConv_scene_settings_output_mode_changed(VariableItem* item) {
    FlipUIDConvApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    app->output_mode = (FlipUIDConvOutput)index;
    variable_item_set_current_value_text(item, FlipUIDConv_output_mode_labels[index]);
}

static void FlipUIDConv_scene_settings_menu_callback(void* context, uint32_t index) {
    FlipUIDConvApp* app = context;
    UNUSED(index);
    scene_manager_next_scene(app->scene_manager, FlipUIDConvSceneRead);
}

void FlipUIDConv_scene_settings_on_enter(void* context) {
    FlipUIDConvApp* app = context;
    VariableItemList* item_list = app->variable_item_list;

    VariableItem* item = variable_item_list_add(
        item_list,
        "Transponder",
        COUNT_OF(FlipUIDConv_read_mode_labels),
        FlipUIDConv_scene_settings_read_mode_changed,
        app);
    variable_item_set_current_value_index(item, app->read_mode);
    variable_item_set_current_value_text(item, FlipUIDConv_read_mode_labels[app->read_mode]);

    item = variable_item_list_add(
        item_list,
        "UID format",
        COUNT_OF(FlipUIDConv_uid_format_labels),
        FlipUIDConv_scene_settings_uid_format_changed,
        app);
    variable_item_set_current_value_index(item, app->uid_format);
    variable_item_set_current_value_text(item, FlipUIDConv_uid_format_labels[app->uid_format]);

    item = variable_item_list_add(
        item_list,
        "Output",
        COUNT_OF(FlipUIDConv_output_mode_labels),
        FlipUIDConv_scene_settings_output_mode_changed,
        app);
    variable_item_set_current_value_index(item, app->output_mode);
    variable_item_set_current_value_text(item, FlipUIDConv_output_mode_labels[app->output_mode]);
    variable_item_list_set_enter_callback(
        item_list, FlipUIDConv_scene_settings_menu_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipUIDConvViewVariableItemList);
}

bool FlipUIDConv_scene_settings_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void FlipUIDConv_scene_settings_on_exit(void* context) {
    FlipUIDConvApp* app = context;
    variable_item_list_reset(app->variable_item_list);
}
