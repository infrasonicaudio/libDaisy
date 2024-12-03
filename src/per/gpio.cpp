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

    Init();
}

void GPIO::Init()
{
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
void GPIO::Init(Pin p, Mode m, Pull pu, Speed sp, InterruptCallback cb)
{
    // Populate Config struct, and init with overload
    cfg_.pin      = p;
    cfg_.mode     = m;
    cfg_.pull     = pu;
    cfg_.speed    = sp;
    cfg_.callback = cb;
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
