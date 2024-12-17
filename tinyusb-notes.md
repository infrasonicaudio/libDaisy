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

- [ ] Adopt latest TinyUSB to allow both USB OTG peripherals to operate simultaneously
- [ ] Stop using ST USBD middleware
- [ ] Change USB host support to use tinyusb
- [ ] Move USB HS HAL callbacks out of system.cpp and into usb.cpp
- [ ] Develop and document user-makefile (or Cmake) driven descriptors configuration so
      modification of tinyusb config/descriptors files inside libDaisy is not required and
      application does not need its own usb_descriptors.c file

## USB CDC (Serial)

This is the most straightforward of the USB device class ports, there's not much complexity
involved at all.

### Changes

* USB serial Tx now calls through to tinyUSB cdc functions (`tud_cdc_`)

### TODOs

- [ ] Rx callbacks in USBHandler probably don't work anymore, need to fix/adapt for tinyusb

## USB MIDI

TODO

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

### Changes

* SAI max buffer size increased to 4096
* DMA non-cached RAM2 region increased to 128kB in linker script

### TODOs

- [ ] Configurable sample rates/bitdepths, input endpoint features (vol, mute), etc
- [ ] Should the audio I/O features be directly integrated into libDaisy audio stack somehow?
