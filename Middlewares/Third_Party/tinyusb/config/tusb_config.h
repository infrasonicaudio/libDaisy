#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#include "tusb_audio_descriptors.h"

#ifdef __cplusplus
extern "C"
{
#endif


//-------------------------------------------------------------------+
// Board Specific Configuration
//--------------------------------------------------------------------+

// RHPort number used for device can be defined by board.mk, default to port 0
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT 1
#endif

// RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUD_MAX_SPEED
//#define BOARD_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#define BOARD_TUD_MAX_SPEED OPT_MODE_FULL_SPEED
#endif

#define CFG_QUIRK_OS_GUESSING   1

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// defined by compiler flags for flexibility
#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_STM32H7
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

// Enable Device stack
#define CFG_TUD_ENABLED 1

// Default is max speed that hardware controller could support with on-chip PHY
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

    //--------------------------------------------------------------------
    // DEVICE CONFIGURATION
    //--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

//------------- CLASS -------------//
#define CFG_TUD_AUDIO 1
#define CFG_TUD_HID 0
#define CFG_TUD_CDC 1
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 1
#define CFG_TUD_VENDOR 0

#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 256
#define CFG_TUD_CDC_EP_BUFSIZE 256

#define CFG_TUD_MIDI_RX_BUFSIZE 256
#define CFG_TUD_MIDI_TX_BUFSIZE 256

/// START -----
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN TUD_AUDIO_SPEAKER_STEREO_FB_DESC_LEN

// Needs enabled if Full-Speed on OSX, also set feedback EP size to 3
// This is handled by quirk guessing + callbacks - should default to 0 here
#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_FORMAT_CORRECTION 0

#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX 2

// Audio format type I specifications
#define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE 48000

// 16bit in 16bit slots
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX 2
#define CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX 16

// EP and buffer size - for isochronous EP´s, the buffer and EP size are equal (different sizes would not make sense)
#define CFG_TUD_AUDIO_ENABLE_EP_OUT 1

#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX                        \
    TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE,       \
                      CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX, \
                      CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ \
         CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX * 8
// END----

// #define CFG_TUD_AUDIO_FUNC_1_DESC_LEN TUD_AUDIO_DEVICE_STEREO_DESC_LEN

// // How many formats are used, need to adjust USB descriptor if changed
// #define CFG_TUD_AUDIO_FUNC_1_N_FORMATS 1

// // Audio format type I specifications
// #define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE 48000
// #define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX 2
// #define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX 2

// // 16bit in 16bit slots
// #define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX 2
// #define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_TX 16
// #define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX 2
// #define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX 16

// // EP and buffer size - for isochronous EP´s, the buffer and EP size are equal (different sizes would not make sense)
// #define CFG_TUD_AUDIO_ENABLE_EP_IN 1

// #define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_IN                             \
//     TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE,                \
//                       CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX, \
//                       CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX)

//     /**
// #define CFG_TUD_AUDIO_FUNC_1_FORMAT_2_EP_SZ_IN                             \
//     TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE,                \
//                       CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_TX, \
//                       CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX)
// */

// // tinyusb FIFOs 4x larger than necessary - helps avoid cb sync errors
// #define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ       \
//     TU_MAX(CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_IN, \
//            CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_IN) \
//         * 4

// #define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX       \
//     TU_MAX(                                     \
//         CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_IN, \
//         CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_IN) // Maximum EP IN size for all AS alternate settings used

// // EP and buffer size - for isochronous EP´s, the buffer and EP size are equal (different sizes would not make sense)
// #define CFG_TUD_AUDIO_ENABLE_EP_OUT 1

// #define CFG_TUD_AUDIO_UNC_1_FORMAT_1_EP_SZ_OUT                             \
//     TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE,                \
//                       CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX, \
//                       CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)
//     /**
// #define CFG_TUD_AUDIO_UNC_1_FORMAT_2_EP_SZ_OUT                             \
//     TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE,                \
//                       CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_RX, \
//                       CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)
// */

// #define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ      \
//     TU_MAX(CFG_TUD_AUDIO_UNC_1_FORMAT_1_EP_SZ_OUT, \
//            CFG_TUD_AUDIO_UNC_1_FORMAT_1_EP_SZ_OUT) \
//         * 4

// #define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX      \
//     TU_MAX(                                     \
//         CFG_TUD_AUDIO_UNC_1_FORMAT_1_EP_SZ_OUT, \
//         CFG_TUD_AUDIO_UNC_1_FORMAT_1_EP_SZ_OUT) // Maximum EP IN size for all AS alternate settings used

// Enable feedback EP
#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP 1

// Number of Standard AS Interface Descriptors (4.9.1) defined per audio function -
// this is required to be able to remember the current alternate settings of these interfaces -
// We restrict us here to have a constant number for all audio functions (which means this has to be
// the maximum number of AS interfaces an audio function has and a second audio function with less
// AS interfaces just wastes a few bytes)
// #define CFG_TUD_AUDIO_FUNC_1_N_AS_INT 2
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT 1

// Size of control request buffer
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ 64

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
