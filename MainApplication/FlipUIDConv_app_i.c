#include "FlipUIDConv_app_i.h"

#include <furi.h>
#include <stdio.h>

#define TAG "FlipUIDConv"

typedef struct {
    FuriThreadId thread_id;
    uint8_t reset_counter;
    bool detected;
    Iso14443_3aError error;
    NfcPoller* poller;
    Iso14443_3aData iso14443_3a_data;
} AsyncPollerContext;

/* Conversion methods: UID formatting */
static uint8_t FlipUIDConv_reverse_bits_u8(uint8_t value) {
    value = (value >> 4) | (value << 4);
    value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2);
    value = ((value & 0xAA) >> 1) | ((value & 0x55) << 1);
    return value;
}

static uint64_t FlipUIDConv_uid_to_uint64_be(const uint8_t* uid, size_t uid_len) {
    uint64_t value = 0;
    size_t start = uid_len > 8 ? uid_len - 8 : 0;
    for(size_t i = start; i < uid_len; i++) {
        value = (value << 8) | uid[i];
    }
    return value;
}

static uint64_t FlipUIDConv_uid_to_uint64_le(const uint8_t* uid, size_t uid_len) {
    uint64_t value = 0;
    size_t count = uid_len > 8 ? 8 : uid_len;
    for(size_t i = 0; i < count; i++) {
        value |= ((uint64_t)uid[i]) << (8 * i);
    }
    return value;
}

static void FlipUIDConv_format_uid(
    FuriString* output,
    const uint8_t* uid,
    size_t uid_len,
    FlipUIDConvUidFormat format) {
    furi_string_reset(output);
    if(uid_len == 0) {
        furi_string_set(output, "N/A");
        return;
    }

    uint64_t value = 0;
    uint8_t reversed_uid[8] = {0};
    switch(format) {
    case FlipUIDConvUidFormatCompact:
    case FlipUIDConvUidFormatSpaced:
        for(size_t i = 0; i < uid_len; i++) {
            if(format == FlipUIDConvUidFormatSpaced && i > 0) {
                furi_string_cat(output, " ");
            }
            furi_string_cat_printf(output, "%02X", uid[i]);
        }
        return;
    case FlipUIDConvUidFormatHex10:
        if(uid_len >= 5) {
            for(size_t i = 0; i < 5; i++) {
                reversed_uid[i] = FlipUIDConv_reverse_bits_u8(uid[i]);
            }
            value = FlipUIDConv_uid_to_uint64_be(reversed_uid, 5);
            furi_string_cat_printf(output, "%010llX", (unsigned long long)value);
        } else {
            furi_string_set(output, "N/A");
        }
        return;
    case FlipUIDConvUidFormatHex8:
        if(uid_len >= 4) {
            for(size_t i = 0; i < 4; i++) {
                reversed_uid[i] = FlipUIDConv_reverse_bits_u8(uid[i]);
            }
            value = FlipUIDConv_uid_to_uint64_be(reversed_uid, 4);
            furi_string_cat_printf(output, "%08llX", (unsigned long long)value);
        } else {
            furi_string_set(output, "N/A");
        }
        return;
    case FlipUIDConvUidFormatIk3Is:
        if(uid_len >= 5) {
            value = FlipUIDConv_uid_to_uint64_le(uid, 5);
            furi_string_cat_printf(output, "%013llu", (unsigned long long)value);
        } else {
            furi_string_set(output, "N/A");
        }
        return;
    case FlipUIDConvUidFormatIk2:
        if(uid_len >= 5) {
            for(size_t i = 0; i < 5; i++) {
                reversed_uid[i] = FlipUIDConv_reverse_bits_u8(uid[i]);
            }
            value = FlipUIDConv_uid_to_uint64_be(reversed_uid, 5);
            furi_string_cat_printf(output, "%013llu", (unsigned long long)value);
        } else {
            furi_string_set(output, "N/A");
        }
        return;
    case FlipUIDConvUidFormatZkCodier:
        if(uid_len >= 5) {
            value = FlipUIDConv_uid_to_uint64_le(uid, 5);
            char hex_value[11] = {0};
            char trimmed_hex[10] = {0};
            snprintf(hex_value, sizeof(hex_value), "%010llX", (unsigned long long)value);
            size_t out_index = 0;
            for(size_t i = 0; i < 10; i++) {
                if(i == 5) {
                    continue;
                }
                trimmed_hex[out_index++] = hex_value[i];
            }
            trimmed_hex[out_index] = '\0';
            for(size_t i = 0; i < out_index; i++) {
                uint8_t digit = 0;
                if(trimmed_hex[i] >= '0' && trimmed_hex[i] <= '9') {
                    digit = trimmed_hex[i] - '0';
                } else if(trimmed_hex[i] >= 'A' && trimmed_hex[i] <= 'F') {
                    digit = trimmed_hex[i] - 'A' + 10;
                }
                furi_string_cat_printf(output, "%02u", digit);
            }
        } else {
            furi_string_set(output, "N/A");
        }
        return;
    case FlipUIDConvUidFormatIs:
        value = FlipUIDConv_uid_to_uint64_le(uid, uid_len);
        furi_string_cat_printf(output, "%llu", (unsigned long long)value);
        return;
    }
}

static void FlipUIDConv_set_uid_text(FuriString* output, const char* text) {
    furi_string_set(output, text);
}

/* Conversion methods: Tag type formatting */
static void FlipUIDConv_set_tag_type_text(FuriString* output, const char* text) {
    furi_string_set(output, text);
}

static void FlipUIDConv_format_nfc_tag_type(const Iso14443_3aData* data, FuriString* output) {
    uint16_t atqa = ((uint16_t)data->atqa[1] << 8) | data->atqa[0];
    uint8_t sak = data->sak;

    if((sak == 0x08) && (atqa == 0x0004)) {
        FlipUIDConv_set_tag_type_text(output, "MIFARE Classic 1K");
    } else if((sak == 0x18) && (atqa == 0x0002)) {
        FlipUIDConv_set_tag_type_text(output, "MIFARE Classic 4K");
    } else if((sak == 0x09) && (atqa == 0x0004)) {
        FlipUIDConv_set_tag_type_text(output, "MIFARE Mini");
    } else if((sak == 0x20)) {
        FlipUIDConv_set_tag_type_text(output, "MIFARE DESFire");
    } else if((sak == 0x10)) {
        FlipUIDConv_set_tag_type_text(output, "MIFARE Plus");
    } else if((sak == 0x00) && (atqa == 0x0044)) {
        FlipUIDConv_set_tag_type_text(output, "MIFARE Ultralight/NTAG");
    } else if(iso14443_3a_supports_iso14443_4(data)) {
        FlipUIDConv_set_tag_type_text(output, "ISO14443-4 (Type 4A)");
    } else {
        FlipUIDConv_set_tag_type_text(output, "NFC-A");
    }
}

static uint16_t FlipUIDConv_hid_keycode_from_char(char c) {
    if(c >= 'a' && c <= 'z') {
        return HID_KEYBOARD_A + (c - 'a');
    }
    if(c >= 'A' && c <= 'Z') {
        return (KEY_MOD_LEFT_SHIFT << 8) | (HID_KEYBOARD_A + (c - 'A'));
    }
    if(c >= '1' && c <= '9') {
        return HID_KEYBOARD_1 + (c - '1');
    }
    if(c == '0') {
        return HID_KEYBOARD_0;
    }
    if(c == ' ') {
        return HID_KEYBOARD_SPACEBAR;
    }
    if(c == '-') {
        return HID_KEYBOARD_MINUS;
    }
    if(c == '/') {
        return HID_KEYBOARD_SLASH;
    }
    return HID_KEYBOARD_NONE;
}

static void FlipUIDConv_send_hid_if_connected(const char* text) {
    if(!text || !furi_hal_hid_is_connected()) {
        return;
    }

    for(size_t i = 0; text[i] != '\0'; i++) {
        uint16_t keycode = FlipUIDConv_hid_keycode_from_char(text[i]);
        if(keycode == HID_KEYBOARD_NONE) {
            continue;
        }
        furi_hal_hid_kb_press(keycode);
        furi_delay_ms(5);
        furi_hal_hid_kb_release(keycode);
        furi_delay_ms(5);
    }

    furi_hal_hid_kb_press(HID_KEYBOARD_ENTER);
    furi_delay_ms(5);
    furi_hal_hid_kb_release(HID_KEYBOARD_ENTER);
}

static NfcCommand iso14443_3a_async_callback(NfcGenericEvent event, void* context) {
    AsyncPollerContext* ctx = (AsyncPollerContext*)context;

    if(event.protocol == NfcProtocolIso14443_3a) {
        Iso14443_3aPollerEvent* poller_event = (Iso14443_3aPollerEvent*)event.event_data;

        if(poller_event->type == Iso14443_3aPollerEventTypeReady) {
            const NfcDeviceData* device_data = nfc_poller_get_data(ctx->poller);
            if(device_data) {
                const Iso14443_3aData* poller_data = (const Iso14443_3aData*)device_data;
                iso14443_3a_copy(&ctx->iso14443_3a_data, poller_data);
                ctx->error = Iso14443_3aErrorNone;
                ctx->detected = true;
                furi_thread_flags_set(ctx->thread_id, ISO14443_3A_ASYNC_FLAG_COMPLETE);
                return NfcCommandStop;
            }
        } else if(poller_event->type == Iso14443_3aPollerEventTypeError) {
            ctx->error = poller_event->data->error;
            ctx->detected = false;
            ctx->reset_counter++;
            if(ctx->reset_counter >= 3) {
                ctx->reset_counter = 0;
                furi_thread_flags_set(ctx->thread_id, ISO14443_3A_ASYNC_FLAG_COMPLETE);
                return NfcCommandReset;
            }
            furi_thread_flags_set(ctx->thread_id, ISO14443_3A_ASYNC_FLAG_COMPLETE);
            return NfcCommandStop;
        }
    }

    return NfcCommandContinue;
}

static bool FlipUIDConv_scan_nfc(FlipUIDConvApp* app, FuriString* output, FuriString* tag_type) {
    Nfc* nfc = nfc_alloc();
    if(!nfc) {
        return false;
    }

    AsyncPollerContext async_ctx = {
        .thread_id = furi_thread_get_current_id(),
        .reset_counter = 0,
        .detected = false,
        .error = Iso14443_3aErrorNone,
        .poller = NULL,
    };

    bool found = false;

    while(app->scanning && !found) {
        async_ctx.detected = false;
        async_ctx.error = Iso14443_3aErrorNone;

        NfcPoller* poller = nfc_poller_alloc(nfc, NfcProtocolIso14443_3a);
        async_ctx.poller = poller;
        nfc_poller_start(poller, iso14443_3a_async_callback, &async_ctx);

        uint32_t flags = furi_thread_flags_wait(
            ISO14443_3A_ASYNC_FLAG_COMPLETE, FuriFlagWaitAny, 200);
        if(flags != (unsigned)FuriFlagErrorTimeout) {
            furi_thread_flags_clear(ISO14443_3A_ASYNC_FLAG_COMPLETE);
        }

        nfc_poller_stop(poller);
        nfc_poller_free(poller);

        if(flags != (unsigned)FuriFlagErrorTimeout && async_ctx.detected &&
           async_ctx.error == Iso14443_3aErrorNone) {
            size_t uid_len = 0;
            const uint8_t* uid = iso14443_3a_get_uid(&async_ctx.iso14443_3a_data, &uid_len);
            if(uid && uid_len > 0) {
                FlipUIDConv_format_uid(output, uid, uid_len, app->uid_format);
                FlipUIDConv_format_nfc_tag_type(&async_ctx.iso14443_3a_data, tag_type);
                found = true;
            }
        }

        if(!found) {
            furi_delay_ms(50);
        }
    }

    nfc_free(nfc);
    return found;
}

static void FlipUIDConv_rfid_read_callback(
    LFRFIDWorkerReadResult result,
    ProtocolId protocol,
    void* context) {
    FlipUIDConvApp* app = context;
    if(result == LFRFIDWorkerReadDone) {
        app->rfid_protocol_id = protocol;
        app->uid_ready = true;
    }
}

static bool FlipUIDConv_scan_rfid(FlipUIDConvApp* app, FuriString* output, FuriString* tag_type) {
    ProtocolDict* dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    if(!dict) {
        return false;
    }

    LFRFIDWorker* worker = lfrfid_worker_alloc(dict);
    if(!worker) {
        protocol_dict_free(dict);
        return false;
    }

    app->uid_ready = false;
    lfrfid_worker_start_thread(worker);
    lfrfid_worker_read_start(worker, LFRFIDWorkerReadTypeAuto, FlipUIDConv_rfid_read_callback, app);

    while(app->scanning && !app->uid_ready) {
        furi_delay_ms(50);
    }

    lfrfid_worker_stop(worker);
    lfrfid_worker_stop_thread(worker);

    bool found = app->uid_ready;
    if(found && app->rfid_protocol_id != PROTOCOL_NO) {
        size_t data_size = protocol_dict_get_data_size(dict, app->rfid_protocol_id);
        if(data_size > 0) {
            uint8_t* data = malloc(data_size);
            protocol_dict_get_data(dict, app->rfid_protocol_id, data, data_size);
            FlipUIDConv_format_uid(output, data, data_size, app->uid_format);
            free(data);
        } else {
            FlipUIDConv_set_uid_text(output, "N/A");
        }
        FlipUIDConv_set_tag_type_text(
            tag_type, protocol_dict_get_name(dict, app->rfid_protocol_id));
    } else {
        FlipUIDConv_set_tag_type_text(tag_type, "Unknown");
    }

    lfrfid_worker_free(worker);
    protocol_dict_free(dict);
    return found;
}

static int32_t FlipUIDConv_scan_thread(void* context) {
    FlipUIDConvApp* app = context;

    app->uid_ready = false;
    furi_string_reset(app->uid_string);
    furi_string_reset(app->tag_type_string);
    FuriString* scanned_uid = furi_string_alloc();
    FuriString* scanned_tag_type = furi_string_alloc();

    while(app->scanning) {
        bool detected = false;
        furi_string_reset(scanned_uid);
        furi_string_reset(scanned_tag_type);
        app->uid_ready = false;
        app->led_tag_found = false;
        if(app->read_mode == FlipUIDConvReadModeNfc) {
            detected = FlipUIDConv_scan_nfc(app, scanned_uid, scanned_tag_type);
        } else {
            detected = FlipUIDConv_scan_rfid(app, scanned_uid, scanned_tag_type);
        }

        if(detected && app->scanning) {
            app->led_tag_found = true;
            if(furi_string_cmp(scanned_uid, app->uid_string) != 0) {
                furi_string_set(app->uid_string, furi_string_get_cstr(scanned_uid));
                app->uid_ready = true;
                FlipUIDConv_send_hid_if_connected(furi_string_get_cstr(app->uid_string));
            }
            furi_string_set(app->tag_type_string, furi_string_get_cstr(scanned_tag_type));
            view_dispatcher_send_custom_event(
                app->view_dispatcher, FlipUIDConvCustomEventUidDetected);
            furi_delay_ms(2000);
        } else {
            furi_delay_ms(50);
        }
    }

    app->scanning = false;
    furi_string_free(scanned_uid);
    furi_string_free(scanned_tag_type);
    return 0;
}

void FlipUIDConv_app_scan_start(FlipUIDConvApp* app) {
    furi_assert(app);
    if(app->scanning) {
        return;
    }

    app->scanning = true;
    app->scan_thread =
        furi_thread_alloc_ex("NfcRfidScan", 2048, FlipUIDConv_scan_thread, app);
    furi_thread_start(app->scan_thread);
}

void FlipUIDConv_app_scan_stop(FlipUIDConvApp* app) {
    furi_assert(app);
    if(!app->scanning && !app->scan_thread) {
        return;
    }

    app->scanning = false;
    if(app->scan_thread) {
        furi_thread_join(app->scan_thread);
        furi_thread_free(app->scan_thread);
        app->scan_thread = NULL;
    }
}

const char* FlipUIDConv_app_get_uid_string(FlipUIDConvApp* app) {
    furi_assert(app);
    return furi_string_get_cstr(app->uid_string);
}
