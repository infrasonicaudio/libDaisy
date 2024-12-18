# TinyUSB Device Driver Implementation

This branch of libDaisy is heavily modified for experimental tinyUSB support.
Read on for notes about code changes and configuration.

For the specific project in which it is being utilized, the descriptors are hard-coded
for a composite CDC + MIDI + USB Audio device with very specific capabilities.
This could be made much more generic in the future. The `usb_descriptors.c` file is part of the
project rather than part of libDaisy (and thus is missing from libDaisy code at the moment),
but the rest of the tinyusb config lives in libDaisy.

## USB Init and Config

### Changes

* The system clock configuration has been changed, specifically PLL3, to avoid a sample rate
  mismatch between actual USB sample clock rate and internal SAI sample clock
* `USBHandle` initialization should happen first, and separately from any other peripheral init
* `USBHandle` init is using the ST USBD library middleware - with tinyUSB this is way overkill, and
   much simpler direct peripheral init would work just fine

### TODOs

- [ ] Stop using ST USBD middleware
- [ ] Change USB host support to use tinyusb
- [ ] Move USB HS HAL callbacks out of system.cpp and into usb.cpp
- [ ] Develop and document user-makefile (or Cmake) driven descriptors configuration so
      modification of tinyusb config/descriptors files inside libDaisy is not required and
      application does not need its own usb_descriptors.c file

## USB CDC (Serial)

### Changes

* USB serial Tx now calls through to tinyUSB cdc functions (`tud_cdc_`)
* Only one of internal (FS) or external (HS) can be used at a time for CDC - not both
* Logger sync Tx is disabled, does not work with tinyusb
* Increased tinyusb CDC buffer sizes to >= 128 to match Logger

### TODOs

- [ ] Figure out if there's a way to setup CDC on both interfaces (int/ext aka FS/HS)
- [ ] Rx callbacks in USBHandler probably don't work anymore, need to fix/adapt for tinyusb
- [ ] BUG: sync start (wait for pc) of USB loggers doesn't work anymore
- [ ] BUG: Funky behavior with tx overflow with lots of prints in between calls to tud_task().
      Messages get clipped and then repeat.

## USB MIDI

### Changes

- USB MIDI transport now uses TinyUSB instead of ST middleware
- Implemnt tud_midi_rx_cb() to handle new data
- Increased tinyusb MIDI buffer sizes to 128

### TODOs

- [ ] Figure out if there's a way to setup MIDI on both interfaces (int/ext aka FS/HS)
- [ ] Reimplement USB Host MIDI using tinyusb
- [ ] Add support for multiple "cables" (maybe?)
- [ ] BUG: Tx retry count doesn't do anything anymore, reimplement if necessary or just remove

## USB Audio

For the application project, the USB audio device descriptor is for a two-channel
"musical instrument" type device with pretty generic attributes for everything:

* 48kHz, 16-bit streams (only option)
* Fixed internal clock config in clock select descriptor chunk (should be changed back to variable
  if/when flexible sample rates are added)
* Input (speaker) stream volume and mute control, channel-independent and global

Currently, UAC2 descriptor macros (in libDaisy file `tusb_config.h`) contain a
ton of hardcoded stuff - maybe this is fine, but should probably be user-configurable
to some degree.

There are still occasional dropped packets on receiving a stream from the host.

### Changes

* SAI max buffer size increased to 4096
* DMA non-cached RAM2 region increased to 128kB in linker script
* DMA IRQn priorities all lowered to 1 (to accommodate USB IRQn to supersede)
* RCC for PLL3 changed to get closer to 48kHz true SAI sample rate
* Call `tud_task()` from `PendSV_Handler()` - triggered on USB interrupts and systick (1ms)
    * This ensures the task is processed low-priority, and as soon as possible after USB ISR
    * Also avoids conflict with any longer-running main loop tasks

### TODOs

- [ ] Configurable sample rates/bitdepths, input endpoint features (vol, mute), etc
- [ ] Should the audio I/O features be directly integrated into libDaisy audio stack somehow?
