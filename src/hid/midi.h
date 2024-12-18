#pragma once
#ifndef DSY_MIDI_H
#define DSY_MIDI_H

#include <stdint.h>
#include <stdlib.h>
#include <algorithm>
#include "per/uart.h"
#include "util/ringbuffer.h"
#include "util/FIFO.h"
#include "hid/midi_parser.h"
#include "hid/midi_util.h"
#include "hid/usb_midi.h"
#include "sys/dma.h"
#include "sys/system.h"

namespace daisy
{



/** @brief   Transport layer for sending and receiving MIDI data over UART
 *  @details This is the mode of communication used for TRS and DIN MIDI
 *           There is an additional 2kB of RAM data used within this class
 *           for processing bulk data from the UART peripheral
 *  @ingroup midi
*/
class MidiUartTransport
{
  public:
    typedef void (*MidiRxParseCallback)(uint8_t* data,
                                        size_t   size,
                                        void*    context);

    MidiUartTransport() {}
    ~MidiUartTransport() {}

    /** @brief Configuration structure for UART MIDI */
    struct Config
    {
        UartHandler::Config::Peripheral periph;
        Pin                             rx;
        Pin                             tx;

        /** Pointer to buffer for DMA UART rx byte transfer in background.
         *
         *  @details By default this uses a shared buffer in DMA_BUFFER_MEM_SECTION,
         *           which can only be utilized for a single UART peripheral. To
         *           use MIDI with multiple UART peripherals, you must provide your own
         *           buffer, allocated to a DMA-capable memory section.
         */
        uint8_t* rx_buffer;

        /** Capacity in bytes of rx_buffer.
         *
         *  @details This value determines the maximum Rx bytes readable by the UART in the background.
         *           By default it's set to the size of the default shared rx_buffer (256 bytes).
         *           While much smaller sizes can be used, data can get missed if the buffer is too small.
         */
        size_t rx_buffer_capacity;

        Config();
    };

    /** @brief Initialization of UART using config struct */
    inline void Init(Config config)
    {
        UartHandler::Config uart_config;

        //defaults
        uart_config.baudrate   = 31250;
        uart_config.stopbits   = UartHandler::Config::StopBits::BITS_1;
        uart_config.parity     = UartHandler::Config::Parity::NONE;
        uart_config.mode       = UartHandler::Config::Mode::TX_RX;
        uart_config.wordlength = UartHandler::Config::WordLength::BITS_8;

        //user settings
        uart_config.periph        = config.periph;
        uart_config.pin_config.rx = config.rx;
        uart_config.pin_config.tx = config.tx;

        rx_buffer_          = config.rx_buffer;
        rx_buffer_capacity_ = config.rx_buffer_capacity;

        /** zero the rx buffer to ensure emptiness regardless of source memory */
        std::fill(rx_buffer_, rx_buffer_ + rx_buffer_capacity_, 0);

        uart_.Init(uart_config);
    }

    /** @brief Start the UART peripheral in listening mode.
     *  This will fill an internal data structure in the background */
    inline void StartRx(MidiRxParseCallback parse_callback, void* context)
    {
        parse_context_  = context;
        parse_callback_ = parse_callback;
        dsy_dma_clear_cache_for_buffer((uint8_t*)this,
                                       sizeof(MidiUartTransport));
        uart_.DmaListenStart(rx_buffer_,
                             rx_buffer_capacity_,
                             MidiUartTransport::rxCallback,
                             this);
    }

    /** @brief returns whether the UART peripheral is actively listening in the background or not */
    inline bool RxActive() { return uart_.IsListening(); }

    /** @brief This is a no-op for UART transport - Rx is via DMA callback with circular buffer */
    inline void FlushRx() {}

    /** No-op for compatibility */
    inline void Receive() {}


    /** @brief sends the buffer of bytes out of the UART peripheral */
    inline void Tx(uint8_t* buff, size_t size)
    {
        uart_.BlockingTransmit(buff, size, 10);
    }

  private:
    UartHandler         uart_;
    uint8_t*            rx_buffer_;
    size_t              rx_buffer_capacity_;
    void*               parse_context_;
    MidiRxParseCallback parse_callback_;

    /** Static callback for Uart MIDI that occurs when
         *  new data is available from the peripheral.
         *  The new data is transferred from the peripheral to the
         *  MIDI instance's byte FIFO that feeds the MIDI parser.
         *
         *  TODO: Handle UartHandler errors better/at all.
         *  (If there is a UART error, there's not really any recovery
         *  option at the moment)
         */
    static void rxCallback(uint8_t*            data,
                           size_t              size,
                           void*               context,
                           UartHandler::Result res)
    {
        /** Read context as transport type */
        MidiUartTransport* transport
            = reinterpret_cast<MidiUartTransport*>(context);
        if(res == UartHandler::Result::OK)
        {
            if(transport->parse_callback_)
            {
                transport->parse_callback_(
                    data, size, transport->parse_context_);
            }
        }
    }
};

/**
    @brief Simple MIDI Handler \n
    Parses bytes from an input into valid MidiEvents. \n
    The MidiEvents fill a FIFO queue that the user can pop messages from.
    @author shensley
    @date March 2020
    @ingroup midi
*/
template <typename Transport,
          size_t KRxEventQueueSize      = 64,
          size_t kTxMessageQueueSize    = 64,
          size_t kTxISRMessageQueueSize = 32,
          size_t kTxBufferSize          = 256>
class MidiHandler
{
  public:
    MidiHandler() {}
    ~MidiHandler() {}

    struct Config
    {
        typename Transport::Config transport_config;
        bool                       running_status_enabled = false;
    };

    /** Initializes the MidiHandler
     *  \param config Configuration structure used to define specifics to the MIDI Handler.
     */
    void Init(Config config)
    {
        config_ = config;
        transport_.Init(config_.transport_config);
        tx_buffer_.Init(config_.running_status_enabled);
        parser_.Init();
    }

    /** Starts listening on the selected input mode(s).
     * MidiEvent Queue will begin to fill, and can be checked with HasEvents() */
    void StartReceive()
    {
        transport_.StartRx(MidiHandler::ParseCallback, this);
    }

    /** Starts listening, but uses a custom callback of the users design.
     *  This callback is responsible for either calling the public parse function,
     *  or forwarding the raw bytes to some other system.
     */
    void StartReceiveWithCallback(MidiUartTransport::MidiRxParseCallback cb)
    {
        transport_.StartRx(cb, this);
    }

    /** Start listening */
    void Listen()
    {
        // In case of UART Error, (particularly
        //  overrun error), UART disables itself.
        // Flush the buff, and restart.
        if(!transport_.RxActive())
        {
            parser_.Reset();
            transport_.FlushRx();
            StartReceive();
        }
        // tinyusb doesn't interrupt on new data,
        // requires constant polling, so we have to
        // pull packets from the queue. This is a no-op
        // for UART MIDI transport.
        transport_.Receive();
    }

    /** Checks if there are unhandled messages in the queue
    \return True if there are events to be handled, else false.
     */
    bool HasEvents() const { return !rx_event_q_.IsEmpty(); }

    bool RxActive() { return transport_.RxActive(); }

    /** Pops the oldest unhandled MidiEvent from the internal queue
    \return The event to be handled
     */
    MidiEvent PopEvent() { return rx_event_q_.PopFront(); }

    /**
        Immediately send raw bytes as message, bypassing queue
    */
    void SendMessage(const uint8_t* bytes, size_t size)
    {
        transport_.Tx(const_cast<uint8_t*>(bytes), size);
    }

    void EnqueueMessage(const MidiTxMessage& msg) { tx_msg_q_.PushBack(msg); }

    /** Higher priority message queue for ISR (clock, etc)
     *  These are transmitted in FIFO order, but *before*
     *  the non-ISR message queue
     */
    void EnqueueMessageFromISR(const MidiTxMessage& msg)
    {
        tx_msg_q_isr_.PushBack(msg);
    }

    /** Transmit enqueued messages*/
    void TransmitEnqueuedMessages()
    {
        // First process and transmit ISR queue
        processAndTransmitMessageQueue(tx_msg_q_isr_);
        // Then non-ISR queue
        processAndTransmitMessageQueue(tx_msg_q_);
    }

    /** Feed in bytes to parser state machine from an external source.
        Populates internal FIFO queue with MIDI Messages.

        \note  Normally application code won't need to use this method directly.
        \param byte MIDI byte to be parsed
    */
    void Parse(uint8_t byte)
    {
        MidiEvent event;
        if(parser_.Parse(byte, &event))
        {
            rx_event_q_.PushBack(event);
        }
    }

  private:
    Config     config_;
    Transport  transport_;
    MidiParser parser_;

    FIFO<MidiEvent, KRxEventQueueSize>          rx_event_q_;
    FIFO<MidiTxMessage, kTxMessageQueueSize>    tx_msg_q_;
    FIFO<MidiTxMessage, kTxISRMessageQueueSize> tx_msg_q_isr_;
    MidiTxBuffer<kTxBufferSize>                 tx_buffer_;

    static void ParseCallback(uint8_t* data, size_t size, void* context)
    {
        MidiHandler* handler = reinterpret_cast<MidiHandler*>(context);
        for(size_t i = 0; i < size; i++)
        {
            handler->Parse(data[i]);
        }
    }

    template <typename Queue>
    void processAndTransmitMessageQueue(Queue& queue)
    {
        MidiTxMessage msg;
        while(!queue.IsEmpty())
        {
            msg = queue.Front();
            // If we can't fit this message, don't pop it,
            // the tx buffer is full
            if(!tx_buffer_.WriteMessage(msg.data, msg.size))
            {
                break;
            }
            queue.PopFront();
        }
        if(!tx_buffer_.IsEmpty())
        {
            transport_.Tx(const_cast<uint8_t*>(tx_buffer_.GetData()),
                          tx_buffer_.GetSize());
            tx_buffer_.Consume();
        }
    }
};

/**
 *  @{
 *  @ingroup midi
 *  @brief shorthand accessors for MIDI Handlers
 * */
using MidiUartHandler = MidiHandler<MidiUartTransport>;
using MidiUsbHandler  = MidiHandler<MidiUsbTransport>;
/** @} */
} // namespace daisy
#endif
