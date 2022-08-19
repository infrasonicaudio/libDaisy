#include "gpio.h"
#include "stm32h7xx_hal.h"

using namespace daisy;

namespace daisy
{

class GPIOInterruptHandler
{
  public:
    enum EXTIChannel : uint8_t
    {
        EXTI0,
        EXTI1,
        EXTI2,
        EXTI3,
        EXTI4,
        EXTI9_5,
        EXTI15_10,
        EXTI_NUM_CHANNELS
    };

    static GPIOInterruptHandler &instance()
    {
        static GPIOInterruptHandler instance;
        return instance;
    }

    EXTIChannel GetChannel(const Pin pin)
    {
        switch(pin.pin)
        {
            case 0: return EXTI0;
            case 1: return EXTI1;
            case 2: return EXTI2;
            case 3: return EXTI3;
            case 4: return EXTI4;
            default: return pin.pin < 10 ? EXTI9_5 : EXTI15_10;
        }
    }

    IRQn_Type GetIRQNType(const Pin pin)
    {
        switch(pin.pin)
        {
            case 0: return EXTI0_IRQn;
            case 1: return EXTI1_IRQn;
            case 2: return EXTI2_IRQn;
            case 3: return EXTI3_IRQn;
            case 4: return EXTI4_IRQn;
            default: return pin.pin < 10 ? EXTI9_5_IRQn : EXTI15_10_IRQn;
        }
    }

    void RegisterPin(const Pin pin, GPIO::InterruptCallback callback)
    {
        auto ch             = GetChannel(pin);
        exti_pins_[ch]      = pin;
        exti_callbacks_[ch] = callback;
    }

    void HandleInterrupt(const EXTIChannel ch)
    {
        Pin      pin   = exti_pins_[ch];
        uint32_t stpin = (1 << pin.pin);
        if(__HAL_GPIO_EXTI_GET_IT(stpin) != 0x00U)
        {
            __HAL_GPIO_EXTI_CLEAR_IT(stpin);
            auto callback = exti_callbacks_[ch];
            if(callback != nullptr)
                callback(pin);
        }
    }

  private:
    Pin                     exti_pins_[EXTI_NUM_CHANNELS];
    GPIO::InterruptCallback exti_callbacks_[EXTI_NUM_CHANNELS];
};

} // namespace daisy


void GPIO::Init(const Config &cfg)
{
    /** Copy Config */
    cfg_ = cfg;

    if(!cfg_.pin.IsValid())
        return;

    GPIO_InitTypeDef ginit;
    switch(cfg_.mode)
    {
        case Mode::OUTPUT: ginit.Mode = GPIO_MODE_OUTPUT_PP; break;
        case Mode::OPEN_DRAIN: ginit.Mode = GPIO_MODE_OUTPUT_OD; break;
        case Mode::ANALOG: ginit.Mode = GPIO_MODE_ANALOG; break;
        case Mode::INTERRUPT_RISING: ginit.Mode = GPIO_MODE_IT_RISING; break;
        case Mode::INTERRUPT_FALLING: ginit.Mode = GPIO_MODE_IT_FALLING; break;
        case Mode::INTERRUPT_BOTH:
            ginit.Mode = GPIO_MODE_IT_RISING_FALLING;
            break;
        case Mode::INPUT:
        default: ginit.Mode = GPIO_MODE_INPUT; break;
    }
    switch(cfg_.pull)
    {
        case Pull::PULLUP: ginit.Pull = GPIO_PULLUP; break;
        case Pull::PULLDOWN: ginit.Pull = GPIO_PULLDOWN; break;
        case Pull::NOPULL:
        default: ginit.Pull = GPIO_NOPULL;
    }
    switch(cfg_.speed)
    {
        case Speed::VERY_HIGH: ginit.Speed = GPIO_SPEED_FREQ_VERY_HIGH; break;
        case Speed::HIGH: ginit.Speed = GPIO_SPEED_FREQ_HIGH; break;
        case Speed::MEDIUM: ginit.Speed = GPIO_SPEED_FREQ_MEDIUM; break;
        case Speed::LOW:
        default: ginit.Speed = GPIO_SPEED_FREQ_LOW;
    }

    port_base_addr_ = GetGPIOBaseRegister();
    /** Start Relevant GPIO clock */
    switch(cfg_.pin.port)
    {
        case PORTA: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case PORTB: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case PORTC: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case PORTD: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        case PORTE: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
        case PORTF: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
        case PORTG: __HAL_RCC_GPIOG_CLK_ENABLE(); break;
        case PORTH: __HAL_RCC_GPIOH_CLK_ENABLE(); break;
        case PORTI: __HAL_RCC_GPIOI_CLK_ENABLE(); break;
        case PORTJ: __HAL_RCC_GPIOJ_CLK_ENABLE(); break;
        case PORTK: __HAL_RCC_GPIOK_CLK_ENABLE(); break;
        default: break;
    }
    /** Set pin based on stm32 schema */
    ginit.Pin = (1 << cfg_.pin.pin);
    HAL_GPIO_Init((GPIO_TypeDef *)port_base_addr_, &ginit);

    if(cfg_.mode == Mode::INTERRUPT_RISING
       || cfg_.mode == Mode::INTERRUPT_FALLING
       || cfg_.mode == Mode::INTERRUPT_BOTH)
    {
        auto &handler        = GPIOInterruptHandler::instance();
        auto  exti_irqn_type = handler.GetIRQNType(cfg_.pin);

        handler.RegisterPin(cfg_.pin, cfg_.callback);

        // TODO: Set priorities from cfg
        HAL_NVIC_SetPriority(exti_irqn_type, 0, 0);
        HAL_NVIC_EnableIRQ(exti_irqn_type);
    }
}
void GPIO::Init(Pin p, const Config &cfg)
{
    /** Copy config */
    cfg_ = cfg;
    /** Overwrite with explicit pin */
    cfg_.pin = p;
    Init(cfg_);
}
void GPIO::Init(Pin p, Mode m, Pull pu, Speed sp)
{
    // Populate Config struct, and init with overload
    cfg_.pin   = p;
    cfg_.mode  = m;
    cfg_.pull  = pu;
    cfg_.speed = sp;
    Init(cfg_);
}

void GPIO::DeInit()
{
    if(cfg_.pin.IsValid())
        HAL_GPIO_DeInit((GPIO_TypeDef *)port_base_addr_, (1 << cfg_.pin.pin));
}
bool GPIO::Read()
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)port_base_addr_,
                            (1 << cfg_.pin.pin));
}
void GPIO::Write(bool state)
{
    HAL_GPIO_WritePin((GPIO_TypeDef *)port_base_addr_,
                      (1 << cfg_.pin.pin),
                      (GPIO_PinState)state);
}
void GPIO::Toggle()
{
    HAL_GPIO_TogglePin((GPIO_TypeDef *)port_base_addr_, (1 << cfg_.pin.pin));
}

uint32_t *GPIO::GetGPIOBaseRegister()
{
    switch(cfg_.pin.port)
    {
        case PORTA: return (uint32_t *)GPIOA;
        case PORTB: return (uint32_t *)GPIOB;
        case PORTC: return (uint32_t *)GPIOC;
        case PORTD: return (uint32_t *)GPIOD;
        case PORTE: return (uint32_t *)GPIOE;
        case PORTF: return (uint32_t *)GPIOF;
        case PORTG: return (uint32_t *)GPIOG;
        case PORTH: return (uint32_t *)GPIOH;
        case PORTI: return (uint32_t *)GPIOI;
        case PORTJ: return (uint32_t *)GPIOJ;
        case PORTK: return (uint32_t *)GPIOK;
        default: return NULL;
    }
}

// #include "stm32h7xx_hal.h"
// #include "per/gpio.h"

/** OLD C Stuff */
extern "C"
{
#include "util/hal_map.h"

    static void start_clock_for_pin(const dsy_gpio *p)
    {
        switch(p->pin.port)
        {
            case DSY_GPIOA: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
            case DSY_GPIOB: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
            case DSY_GPIOC: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
            case DSY_GPIOD: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
            case DSY_GPIOE: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
            case DSY_GPIOF: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
            case DSY_GPIOG: __HAL_RCC_GPIOG_CLK_ENABLE(); break;
            case DSY_GPIOH: __HAL_RCC_GPIOH_CLK_ENABLE(); break;
            case DSY_GPIOI: __HAL_RCC_GPIOI_CLK_ENABLE(); break;
            default: break;
        }
    }

    void dsy_gpio_init(const dsy_gpio *p)
    {
        GPIO_TypeDef    *port;
        GPIO_InitTypeDef ginit;
        switch(p->mode)
        {
            case DSY_GPIO_MODE_INPUT: ginit.Mode = GPIO_MODE_INPUT; break;
            case DSY_GPIO_MODE_OUTPUT_PP:
                ginit.Mode = GPIO_MODE_OUTPUT_PP;
                break;
            case DSY_GPIO_MODE_OUTPUT_OD:
                ginit.Mode = GPIO_MODE_OUTPUT_OD;
                break;
            case DSY_GPIO_MODE_ANALOG: ginit.Mode = GPIO_MODE_ANALOG; break;
            default: ginit.Mode = GPIO_MODE_INPUT; break;
        }
        switch(p->pull)
        {
            case DSY_GPIO_NOPULL: ginit.Pull = GPIO_NOPULL; break;
            case DSY_GPIO_PULLUP: ginit.Pull = GPIO_PULLUP; break;
            case DSY_GPIO_PULLDOWN: ginit.Pull = GPIO_PULLDOWN; break;
            default: ginit.Pull = GPIO_NOPULL; break;
        }
        ginit.Speed = GPIO_SPEED_LOW;
        port        = dsy_hal_map_get_port(&p->pin);
        ginit.Pin   = dsy_hal_map_get_pin(&p->pin);
        start_clock_for_pin(p);
        HAL_GPIO_Init(port, &ginit);
    }

    void dsy_gpio_deinit(const dsy_gpio *p)
    {
        GPIO_TypeDef *port;
        uint16_t      pin;
        port = dsy_hal_map_get_port(&p->pin);
        pin  = dsy_hal_map_get_pin(&p->pin);
        HAL_GPIO_DeInit(port, pin);
    }

    uint8_t dsy_gpio_read(const dsy_gpio *p)
    {
        return HAL_GPIO_ReadPin(dsy_hal_map_get_port(&p->pin),
                                dsy_hal_map_get_pin(&p->pin));
        //    return HAL_GPIO_ReadPin((GPIO_TypeDef *)gpio_hal_port_map[p->pin.port],
        //                            gpio_hal_pin_map[p->pin.pin]);
    }

    void dsy_gpio_write(const dsy_gpio *p, uint8_t state)
    {
        return HAL_GPIO_WritePin(dsy_hal_map_get_port(&p->pin),
                                 dsy_hal_map_get_pin(&p->pin),
                                 (GPIO_PinState)(state > 0 ? 1 : 0));
        //    HAL_GPIO_WritePin((GPIO_TypeDef *)gpio_hal_port_map[p->pin.port],
        //                      gpio_hal_pin_map[p->pin.pin],
        //                      (GPIO_PinState)(state > 0 ? 1 : 0));
    }
    void dsy_gpio_toggle(const dsy_gpio *p)
    {
        return HAL_GPIO_TogglePin(dsy_hal_map_get_port(&p->pin),
                                  dsy_hal_map_get_pin(&p->pin));
        //    HAL_GPIO_TogglePin((GPIO_TypeDef *)gpio_hal_port_map[p->pin.port],
        //                       gpio_hal_pin_map[p->pin.pin]);
    }
}

extern "C"
{
    void EXTI0_IRQHandler(void)
    {
        GPIOInterruptHandler::instance().HandleInterrupt(
            GPIOInterruptHandler::EXTI0);
    }

    void EXTI1_IRQHandler(void)
    {
        GPIOInterruptHandler::instance().HandleInterrupt(
            GPIOInterruptHandler::EXTI1);
    }

    void EXTI2_IRQHandler(void)
    {
        GPIOInterruptHandler::instance().HandleInterrupt(
            GPIOInterruptHandler::EXTI2);
    }

    void EXTI3_IRQHandler(void)
    {
        GPIOInterruptHandler::instance().HandleInterrupt(
            GPIOInterruptHandler::EXTI3);
    }

    void EXTI4_IRQHandler(void)
    {
        GPIOInterruptHandler::instance().HandleInterrupt(
            GPIOInterruptHandler::EXTI4);
    }

    void EXTI9_5_IRQHandler(void)
    {
        GPIOInterruptHandler::instance().HandleInterrupt(
            GPIOInterruptHandler::EXTI9_5);
    }

    void EXTI15_10_IRQHandler(void)
    {
        GPIOInterruptHandler::instance().HandleInterrupt(
            GPIOInterruptHandler::EXTI15_10);
    }
}
