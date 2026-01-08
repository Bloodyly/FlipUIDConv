#pragma once

#include <furi.h>
#include <furi_hal.h>

#define FLIPUIDCONV_VERSION_APP "0.1"
#define FLIPUIDCONV_DEVELOPED   "Abbyss"
#define FLIPUIDCONV_GITHUB      "https://github.com/flipperdevices/flipperzero-good-faps"

typedef enum {
    FlipUIDConvViewVariableItemList,
    FlipUIDConvViewWidget,
} FlipUIDConvView;

typedef enum {
    FlipUIDConvReadModeNfc,
    FlipUIDConvReadModeRfid,
} FlipUIDConvReadMode;

typedef enum {
    FlipUIDConvUidFormatCompact,
    FlipUIDConvUidFormatSpaced,
    FlipUIDConvUidFormatHex10,
    FlipUIDConvUidFormatHex8,
    FlipUIDConvUidFormatIk3Is,
    FlipUIDConvUidFormatIk2,
    FlipUIDConvUidFormatZkCodier,
    FlipUIDConvUidFormatZxUa,
    FlipUIDConvUidFormatHitag,
    FlipUIDConvUidFormatWiegand32,
    FlipUIDConvUidFormatWiegand26,
    FlipUIDConvUidFormatMsb,
    FlipUIDConvUidFormatLsb,
} FlipUIDConvUidFormat;

typedef enum {
    FlipUIDConvOutputScreen,
    FlipUIDConvOutputUsbHid,
} FlipUIDConvOutput;
