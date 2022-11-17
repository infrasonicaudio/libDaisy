#pragma once
#ifndef DSY_DEV_TLC59281_H
#define DSY_DEV_TLC59281_H

#include "sr_595.h"

namespace daisy
{

/**
 * @brief "Bit bang" transport for TLC59281 using SR595 device driver
 */
class Tlc59281BitBangTransport
{
  public:
    struct Config
    {
        Pin clk_pin;
        Pin data_pin;
        Pin latch_pin;
    };

    Tlc59281BitBangTransport()  = default;
    ~Tlc59281BitBangTransport() = default;

    void Init(const Config& config)
    {
        dsy_gpio_pin pins[ShiftRegister595::NUM_PINS];
        pins[ShiftRegister595::PIN_LATCH] = config.latch_pin;
        pins[ShiftRegister595::PIN_CLK]   = config.clk_pin;
        pins[ShiftRegister595::PIN_DATA]  = config.data_pin;
        sr_.Init(pins, 2); // 2 devices = 16 bits
    };

    void Set(uint8_t idx, bool state) { sr_.Set(idx, state); };

    void Write() { sr_.Write(); };

  private:
    ShiftRegister595 sr_;
};

// class TLC59281SpiTransport
// {
//   public:
//     struct Config
//     {

//       SpiHandle::Config::Peripheral spi_periph;

//       struct
//       {
//         Pin clk;
//         Pin data;
//         Pin latch;
//       } pin_config;

//       void Defaults()
//       {
//         spi_periph = SpiHandle::Config::Peripheral::SPI_1;
//         pin_config.clk    = Pin(GPIOPort::PORTG, 11);
//         pin_config.data   = Pin(GPIOPort::PORTB, 5);
//         pin_config.latch  = Pin(GPIOPort::PORTG, 10);
//       }
//     };

//     void Init()
//     {
//       Config config;
//       config.Defaults();
//       Init(config);
//     };

//     void Init(const Config &config)
//     {
//       SpiHandle::Config spi_cfg;
//       spi_cfg.periph = config.spi_periph;
//       spi_cfg.mode = SpiHandle::Config::Mode::MASTER;
//       spi_cfg.direction = SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;
//       spi_cfg.nss = SpiHandle::Config::NSS::HARD_OUTPUT;
//       spi_cfg.clock_polarity = SpiHandle::Config::ClockPolarity::HIGH;
//       spi_cfg.clock_phase = SpiHandle::Config::ClockPhase::ONE_EDGE;
//       spi_cfg.datasize = 16; //
//       spi_cfg.pin_config.sclk = config.pin_config.clk;
//       spi_cfg.pin_config.mosi = config.pin_config.data;
//       spi_cfg.pin_config.miso = Pin();
//       spi_cfg.pin_config.nss = config.pin_config.latch;
//     };

//   private:
//     SpiHandle spi_;
// };

/**
 * @brief Device support for TLC59281 and TLC59282 constant current LED drivers 
 * @author Nick Donaldson 
 * @date November 2022
 * @tparam Deisred peripheral transport implementation. Use predefined typealiases for convenience.
 */
template <typename Transport>
class Tlc59281
{
  public:
    struct Config
    {
        typename Transport::Config transport_config;
    };

    void Init()
    {
        Config config;
        config.transport_config.Defaults();
        Init(config);
    };

    void Init(const Config& config) {
      transport_.Init(config.transport_config);
      Clear();
    };

    void Set(uint8_t idx, bool state)
    {
      transport_.Set(idx, state);
    }

    void Write()
    {
      transport_.Write();
    };

    void Clear()
    {
      for (uint8_t i = 0; i < kNumChannels; i++)
      {
        transport_.Set(i, false);
      }
      Write();
    };

  private:
    static const uint8_t kNumChannels = 16;
    Transport transport_;
};

using Tlc59281BitBang = Tlc59281<Tlc59281BitBangTransport>;

} // namespace daisy

#endif