#pragma once
#ifndef DSY_DOTSTAR_H
#define DSY_DOTSTAR_H

#include "per/i2c.h"
#include "per/spi.h"
#include <cmsis_gcc.h>

namespace daisy
{
/**
 * \brief SPI Transport for DotStars
 */
class DotStarSpiTransport
{
  public:
    struct Config
    {
        SpiHandle::Config::Peripheral    periph;
        SpiHandle::Config::BaudPrescaler baud_prescaler;
        Pin                              clk_pin;
        Pin                              data_pin;

        void Defaults()
        {
            periph         = SpiHandle::Config::Peripheral::SPI_1;
            baud_prescaler = SpiHandle::Config::BaudPrescaler::PS_4;
            clk_pin        = Pin(PORTG, 11);
            data_pin       = Pin(PORTB, 5);
        };
    };

    inline void Init(Config &config)
    {
        SpiHandle::Config spi_cfg;
        spi_cfg.periph    = config.periph;
        spi_cfg.mode      = SpiHandle::Config::Mode::MASTER;
        spi_cfg.direction = SpiHandle::Config::Direction::TWO_LINES_TX_ONLY;
        spi_cfg.clock_polarity  = SpiHandle::Config::ClockPolarity::LOW;
        spi_cfg.clock_phase     = SpiHandle::Config::ClockPhase::ONE_EDGE;
        spi_cfg.datasize        = 8;
        spi_cfg.nss             = SpiHandle::Config::NSS::SOFT;
        spi_cfg.baud_prescaler  = config.baud_prescaler;
        spi_cfg.pin_config.sclk = config.clk_pin;
        spi_cfg.pin_config.mosi = config.data_pin;
        spi_cfg.pin_config.miso = Pin();
        spi_cfg.pin_config.nss  = Pin();

        spi_.Init(spi_cfg);
    };

    bool Write(const uint8_t *data, size_t size)
    {
        return spi_.BlockingTransmit(const_cast<uint8_t *>(data), size)
               == SpiHandle::Result::OK;
    };

  private:
    SpiHandle spi_;
};


/** \brief Device support for Adafruit DotStar LEDs (Opsco SK9822)
    \author Nick Donaldson
    \date March 2023
*/
template <typename Transport>
class DotStar
{
  public:
    enum class Result
    {
        OK,
        ERR_INVALID_ARGUMENT,
        ERR_TRANSPORT
    };

    struct Config
    {
        enum ColorOrder : uint8_t
        {
            //      R          G          B
            RGB = ((0 << 4) | (1 << 2) | (2)),
            RBG = ((0 << 4) | (2 << 2) | (1)),
            GRB = ((1 << 4) | (0 << 2) | (2)),
            GBR = ((2 << 4) | (0 << 2) | (1)),
            BRG = ((1 << 4) | (2 << 2) | (0)),
            BGR = ((2 << 4) | (1 << 2) | (0)),
        };

        typename Transport::Config
                   transport_config; /**< Transport-specific configuration */
        ColorOrder color_order;      /**< Pixel color channel ordering */
        uint16_t   num_pixels;       /**< Number of pixels/LEDs (max 64) */

        void Defaults()
        {
            transport_config.Defaults();
            color_order = ColorOrder::RGB;
            num_pixels  = 1;
        };
    };

    DotStar(){};
    ~DotStar(){};

    Result Init(Config &config)
    {
        if(config.num_pixels > kMaxNumPixels)
        {
            return Result::ERR_INVALID_ARGUMENT;
        }
        transport_.Init(config.transport_config);
        num_pixels_ = config.num_pixels;
        // first color byte is always global brightness (hence +1 offset)
        r_offset_ = ((config.color_order >> 4) & 0b11) + 1;
        g_offset_ = ((config.color_order >> 2) & 0b11) + 1;
        b_offset_ = (config.color_order & 0b11) + 1;
        dither_   = 0;
        // Minimum brightness by default. These LEDs can
        // be very current hungry. See datasheet for details.
        SetAllGlobalBrightness(1);
        SetGamma(2.6f);
        Clear();
        return Result::OK;
    };


    /**
     * @brief Sets gamma correction factor for 8-bit software brightness calc
     *
     * @param gamma Gamma (usually between 1 and 3)
     */
    void SetGamma(float gamma)
    {
        for(size_t i = 0; i < 256; i++)
        {
            gammaCorrectionLUT_8Bit_[i] = static_cast<uint8_t>(
                roundf(255.0f * powf(i / 255.0f, gamma)));
        }
    }

    /**
     * \brief Set global hardware brightness (const current) for all pixels
     * \details "Global brightness" for the SK9822 device sets the
     *          equivalent constant current for the LEDs, not a pre-multiplied PWM
     *          brightness scaling for the pixel's RGB value. See SK9822 datasheet
     *          for details.
     * \warning Recommend not going above 10, especially for SK9822-EC20 which may
     *          overheat if you do.
     *
     * \param b 5-bit global brightness setting (0 - 31)
     */
    void SetAllGlobalBrightness(uint16_t b)
    {
        for(uint16_t i = 0; i < num_pixels_; i++)
        {
            SetPixelGlobalBrightness(i, b);
        }
        b_global_ = b;
    };

    /**
     * \brief Set global hardware brightness (const current) for a single pixel
     * \details "Global brightness" for the SK9822 device sets the
     *          equivalent constant current for the LEDs. See datasheet
     *          for details.
     * \warning Recommend not going above 10, especially for SK9822-EC20 which may
     *          overheat if you do.
     *
     * \param idx Index of the pixel for which to set global brightness
     * \param b 5-bit global brightness setting (0 - 31)
     */
    Result SetPixelGlobalBrightness(uint16_t idx, uint16_t b)
    {
        if(idx >= num_pixels_)
        {
            return Result::ERR_INVALID_ARGUMENT;
        }
        uint8_t *pixel = (uint8_t *)(&pixels_[idx]);
        pixel[0]       = 0xE0 | std::min(b, (uint16_t)31);
        return Result::OK;
    };

    uint16_t GetPixelColor(uint16_t idx)
    {
        if(idx >= num_pixels_)
            return 0;
        uint32_t       c     = 0;
        const uint8_t *pixel = (uint8_t *)&pixels_[idx];
        c                    = c | (pixel[r_offset_] << 16);
        c                    = c | (pixel[g_offset_] << 8);
        c                    = c | pixel[b_offset_];
        return c;
    }

    /**
     * \brief Sets color of a single pixel
     *
     * \param idx Index of the pixel
     * \param color Color object to apply to the pixel
     * \param brightness 8-bit software brightness for pixel
     */
    void SetPixelColor(uint16_t     idx,
                       const Color &color,
                       uint16_t     brightness = 255)
    {
        SetPixelColor(
            idx, color.Red8(), color.Green8(), color.Blue8(), brightness);
    }

    /**
     * \brief Sets color of a single pixel
     * \param color 32-bit integer representing 24-bit RGB color. MSB ignored.
     * \param brightness 8-bit software brightness for pixel
     */
    void
    SetPixelColor(uint16_t idx, uint32_t color, uint16_t brightness = 255)
    {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        SetPixelColor(idx, r, g, b, brightness);
    }

    /**
     * \brief Sets color of a single pixel
     *
     * \param idx Index of the pixel
     * \param r 8-bit red value to apply to pixel
     * \param g 8-bit green value to apply to pixel
     * \param b 8-bit blue value to apply to pixel
     * \param brightness 8-bit software brightness for pixel
     */
    Result SetPixelColor(uint16_t idx,
                         uint8_t  r,
                         uint8_t  g,
                         uint8_t  b,
                         uint16_t brightness = 255)
    {
        if(idx >= num_pixels_)
        {
            return Result::ERR_INVALID_ARGUMENT;
        }
        uint8_t *pixel   = (uint8_t *)(&pixels_[idx]);
        pixel[r_offset_] = r;
        pixel[g_offset_] = g;
        pixel[b_offset_] = b;
        brightness_[idx] = brightness;
        return Result::OK;
    };

    /**
     * \brief Fills all pixels with color
     * \param color Color with which to fill all pixels
     * \param brightness 8-bit software brightness for all pixels
     */
    void Fill(const Color &color, uint16_t brightness = 255)
    {
        for(uint16_t i = 0; i < num_pixels_; i++)
        {
            SetPixelColor(i, color, brightness);
        }
    }

    /**
     * \brief Fills all pixels with color
     * \param color 32-bit integer representing 24-bit RGB color. MSB ignored.
     * \param brightness 8-bit software brightness for all pixels
     */
    void Fill(uint32_t color, uint16_t brightness = 255)
    {
        for(uint16_t i = 0; i < num_pixels_; i++)
        {
            SetPixelColor(i, color, brightness);
        }
    }

    /**
     * \brief Fill all pixels with color
     * \param r 8-bit red value to apply to pixels
     * \param g 8-bit green value to apply to pixels
     * \param b 8-bit blue value to apply to pixels
     * \param brightness 8-bit software brightness for all pixels
     */
    void Fill(uint8_t r, uint8_t g, uint8_t b, uint16_t brightness = 255)
    {
        for(uint16_t i = 0; i < num_pixels_; i++)
        {
            SetPixelColor(i, r, g, b, brightness);
        }
    }

    /** \brief Clears all current color data.
     *         Does not reset global brightnesses.
     *         Does not write pixel buffer data to LEDs.
     */
    void Clear(bool reset_global_brightness = true)
    {
        if(reset_global_brightness)
        {
            SetAllGlobalBrightness(b_global_);
        }
        for(uint16_t i = 0; i < num_pixels_; i++)
        {
            SetPixelColor(i, 0);
        }
    };

    /** \brief Writes current pixel buffer data to LEDs */
    Result Show()
    {
        uint8_t data[4] = {};

        const uint8_t shift = 8 - kDitherBits;
        const uint8_t sf[4] = {0x00, 0x00, 0x00, 0x00};
        const uint8_t ef[4] = {0xFF, 0xFF, 0xFF, 0xFF};

        if(!transport_.Write(sf, 4))
        {
            return Result::ERR_TRANSPORT;
        }

        for(uint16_t i = 0; i < num_pixels_; i++)
        {
            const uint8_t *pixel = (uint8_t *)&pixels_[i];

            // Apply 8-bit brightness and shift according to dither depth
            // (i.e. if 2 bits of dither, we shift so multiplication is a 10-bit number)
            uint16_t r = (pixel[r_offset_] * brightness_[i]) >> shift;
            uint16_t g = (pixel[g_offset_] * brightness_[i]) >> shift;
            uint16_t b = (pixel[b_offset_] * brightness_[i]) >> shift;

            // apply dither and truncate
            r = r > 0 ? __USAT((r + dither_) >> kDitherBits, 8) : 0;
            g = g > 0 ? __USAT((g + dither_) >> kDitherBits, 8) : 0;
            b = b > 0 ? __USAT((b + dither_) >> kDitherBits, 8) : 0;

            // gamma correction
            r = gammaCorrectionLUT_8Bit_[r];
            g = gammaCorrectionLUT_8Bit_[g];
            b = gammaCorrectionLUT_8Bit_[b];

            data[0]         = pixel[0]; // copy global brightness
            data[r_offset_] = r;
            data[g_offset_] = g;
            data[b_offset_] = b;

            if(!transport_.Write(data, 4))
            {
                return Result::ERR_TRANSPORT;
            }
        }
        if(!transport_.Write(ef, 4))
        {
            return Result::ERR_TRANSPORT;
        }
        dither_ = (dither_ + 1) & ((1 << kDitherBits) - 1);
        // dither_ = rand() & ((1 << kDitherBits) - 1);
        return Result::OK;
    };

  private:
    static constexpr size_t  kMaxNumPixels = 64;
    static constexpr uint8_t kDitherBits   = 2;
    Transport                transport_;
    uint16_t                 num_pixels_;
    uint32_t                 pixels_[kMaxNumPixels];
    uint8_t                  brightness_[kMaxNumPixels];
    uint8_t                  gammaCorrectionLUT_8Bit_[256];
    uint8_t                  r_offset_, g_offset_, b_offset_;
    uint16_t                 b_global_;
    int16_t                  dither_;
};

using DotStarSpi = DotStar<DotStarSpiTransport>;

} // namespace daisy

#endif
