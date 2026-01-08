#include "FlipUIDConv_app_i.h"

#include <furi.h>

#define TAG "FlipUIDConv"

typedef struct {
    FuriThreadId thread_id;
    uint8_t reset_counter;
    bool detected;
    Iso14443_3aError error;
    NfcPoller* poller;
    Iso14443_3aData iso14443_3a_data;
} AsyncPollerContext;

static void FlipUIDConv_format_uid_hex(
    FuriString* output,
    const uint8_t* uid,
    size_t uid_len,
    FlipUIDConvUidFormat format) {
    furi_string_reset(output);
    for(size_t i = 0; i < uid_len; i++) {
        if(format == FlipUIDConvUidFormatSpaced && i > 0) {
            furi_string_cat(output, " ");
        }
        furi_string_cat_printf(output, "%02X", uid[i]);
    }
}

static void FlipUIDConv_set_uid_text(FlipUIDConvApp* app, const char* text) {
    furi_string_set(app->uid_string, text);
    app->uid_ready = true;
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

static bool FlipUIDConv_scan_nfc(FlipUIDConvApp* app) {
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
                FlipUIDConv_format_uid_hex(
                    app->uid_string, uid, uid_len, app->uid_format);
                app->uid_ready = true;
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

static bool FlipUIDConv_scan_rfid(FlipUIDConvApp* app) {
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
            FlipUIDConv_format_uid_hex(
                app->uid_string, data, data_size, app->uid_format);
            free(data);
        } else {
            FlipUIDConv_set_uid_text(app, "N/A");
        }
    }

    lfrfid_worker_free(worker);
    protocol_dict_free(dict);
    return found;
}

static int32_t FlipUIDConv_scan_thread(void* context) {
    FlipUIDConvApp* app = context;

    app->uid_ready = false;
    furi_string_reset(app->uid_string);

    bool detected = false;
    if(app->read_mode == FlipUIDConvReadModeNfc) {
        detected = FlipUIDConv_scan_nfc(app);
    } else {
        detected = FlipUIDConv_scan_rfid(app);
    }

    if(detected) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, FlipUIDConvCustomEventUidDetected);
    }

    app->scanning = false;
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
