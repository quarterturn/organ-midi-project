/*
NewSoftSerial.cpp - Multi-instance software serial library
Copyright (c) 2006 David A. Mellis.  All rights reserved.
Interrupt-driven receive and other improvements by ladyada 
Tuning, circular buffer, derivation from class Print,
multi-instance support, porting to 8MHz processors by
Mikal Hart

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

The latest version of this library can always be found at
http://arduiniana.org.
*/

// When set, _DEBUG co-opts pins 11 and 13 for debugging with an
// oscilloscope or logic analyzer.  Beware: it also slightly modifies
// the bit times, so don't rely on it too much at high baud rates
#define _DEBUG 0

// 
// Includes
// 
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "Arduino.h"
#include "NewSoftSerial.h"
//
// Lookup table
//
typedef struct _DELAY_TABLE
{
  long baud;
  unsigned short rx_delay_centering;
  unsigned short rx_delay_intrabit;
  unsigned short rx_delay_stopbit;
  unsigned short tx_delay;
} DELAY_TABLE;

#if F_CPU == 16000000

static const DELAY_TABLE PROGMEM table[] = 
{
  //  baud    rxcenter   rxintra    rxstop    tx
  { 115200,   /*5*/1,         17,        17,       /*15*/13,    },
  { 57600,    /*15*/10,      37,        37,       34,    },
  { 38400,    25,        57,        57,       54,    },
  { 31250,    31,        70,        70,       68,    },
  { 28800,    34,        77,        77,       74,    },
  { 19200,    54,        117,       117,      114,   },
  { 14400,    74,        156,       156,      153,   },
  { 9600,     114,       236,       236,      233,   },
  { 4800,     233,       474,       474,      471,   },
  { 2400,     471,       950,       950,      947,   },
  { 1200,     947,       1902,      1902,     1899,  },
  { 300,      3804,      7617,      7617,     7614,  },
};

#define XMIT_START_ADJUSTMENT 5

#elif F_CPU == 8000000

static const DELAY_TABLE table[] PROGMEM = 
{
  //  baud    rxcenter   rxintra    rxstop    tx
  { 115200,  1,          5,         5,      3,      },
  { 57600,   1,          15,        15,     /*12*/13,      },
  { 38400,   /*6*/2,          25,        /*25*/26,     /*22*/23,     },
  { 31250,   /*9*/7,     32,        33,     29,     },
  { 28800,   11,         35,        35,     32,     },
  { 19200,   20,         55,        55,     52,     },
  { 14400,   30,         75,        75,     72,     },
  { 9600,    50,         114,       114,    112,    },
  { 4800,    110,        233,       233,    230,    },
  { 2400,    229,        472,       472,    469,    },
  { 1200,    467,        948,       948,    945,    },
  { 300,     1895,       3805,      3805,   3802,   },
};

#define XMIT_START_ADJUSTMENT 4

#else

#error This version of NewSoftSerial supports only 16 and 8MHz processors

#endif

//
// Statics
//
NewSoftSerial *NewSoftSerial::active_object = 0;
char NewSoftSerial::_receive_buffer[_NewSS_MAX_RX_BUFF]; 
volatile uint8_t NewSoftSerial::_receive_buffer_tail = 0;
volatile uint8_t NewSoftSerial::_receive_buffer_head = 0;

//
// Debugging
//
// This function generates a brief pulse on pin 8-13
// for debugging or measuring on an oscilloscope.
inline void DebugPulse(uint8_t pin, uint8_t count)
{
#if _DEBUG
  uint8_t a = PORTB;
  while (count--)
  {
    PORTB = a | _BV(pin-8);
    PORTB = a;
  }
#endif
}

//
// Private methods
//

/* static */ 
inline void NewSoftSerial::tunedDelay(uint16_t delay) { 
  uint8_t tmp=0;

  asm volatile("sbiw    %0, 0x01 \n\t"
    "ldi %1, 0xFF \n\t"
    "cpi %A0, 0xFF \n\t"
    "cpc %B0, %1 \n\t"
    "brne .-10 \n\t"
    : "+r" (delay), "+a" (tmp)
    : "0" (delay)
    );
}

// This function sets the current object as the "active"
// one and returns true if it replaces another 
bool NewSoftSerial::activate(void)
{
  if (active_object != this)
  {
    _buffer_overflow = false;
    uint8_t oldSREG = SREG;
    cli();
    _receive_buffer_head = _receive_buffer_tail = 0;
    active_object = this;
    SREG = oldSREG;
    return true;
  }

  return false;
}

//
// The receive routine called by the interrupt handler
//
void NewSoftSerial::recv()
{

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Preserve the registers that the compiler misses
// (courtesy of Arduino forum user *etracer*)
  asm volatile(
    "push r18 \n\t"
    "push r19 \n\t"
    "push r20 \n\t"
    "push r21 \n\t"
    "push r22 \n\t"
    "push r23 \n\t"
    "push r26 \n\t"
    "push r27 \n\t"
    ::);
#endif  

  uint8_t d = 0;

  // If RX line is high, then we don't see any start bit
  // so interrupt is probably not for us
  if (!rx_pin_read())
  {
    // Wait approximately 1/2 of a bit width to "center" the sample
    tunedDelay(_rx_delay_centering);
    DebugPulse(13, 1);

    // Read each of the 8 bits
    for (uint8_t i=0x1; i; i <<= 1)
    {
      tunedDelay(_rx_delay_intrabit);
      DebugPulse(13, 1);
      uint8_t noti = ~i;
      if (rx_pin_read())
        d |= i;
      else // else clause added to ensure function timing is ~balanced
        d &= noti;
    }

    // skip the stop bit
    tunedDelay(_rx_delay_stopbit);
    DebugPulse(13, 1);

    // if buffer full, set the overflow flag and return
    if ((_receive_buffer_tail + 1) % _NewSS_MAX_RX_BUFF != _receive_buffer_head) 
    {
      // save new data in buffer: tail points to where byte goes
      _receive_buffer[_receive_buffer_tail] = d; // save new byte
      _receive_buffer_tail = (_receive_buffer_tail + 1) % _NewSS_MAX_RX_BUFF;
    } 
    else 
    {
#if _DEBUG // for scope: bring pin 11 high: overflow indicator
      PORTB |= _BV(3);
#endif
      _buffer_overflow = true;
    }
  }

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Restore the registers that the compiler misses
  asm volatile(
    "pop r27 \n\t"
    "pop r26 \n\t"
    "pop r23 \n\t"
    "pop r22 \n\t"
    "pop r21 \n\t"
    "pop r20 \n\t"
    "pop r19 \n\t"
    "pop r18 \n\t"
    ::);
#endif
}

void NewSoftSerial::tx_pin_write(uint8_t pin_state)
{
 	if (pin_state == LOW) 
    *_transmitPortRegister &= ~_transmitBitMask;
	else 
    *_transmitPortRegister |= _transmitBitMask;
}

uint8_t NewSoftSerial::rx_pin_read()
{
  return *_receivePortRegister & _receiveBitMask;
}

//
// Interrupt handling
//

/* static */
inline void NewSoftSerial::handle_interrupt()
{
  if (active_object)
  {
    active_object->recv();
  }
}

ISR(PCINT0_vect)
{
  NewSoftSerial::handle_interrupt();
}

ISR(PCINT1_vect)
{
  NewSoftSerial::handle_interrupt();
}

ISR(PCINT2_vect)
{
  NewSoftSerial::handle_interrupt();
}

//
// Constructor
//
NewSoftSerial::NewSoftSerial(uint8_t receivePin, uint8_t transmitPin) : 
  _rx_delay_centering(0),
  _rx_delay_intrabit(0),
  _rx_delay_stopbit(0),
  _tx_delay(0),
  _buffer_overflow(false)
{
  setTX(transmitPin);
  setRX(receivePin);
}

//
// Public methods
//
void NewSoftSerial::setTX(uint8_t tx)
{
  pinMode(tx, OUTPUT);
  digitalWrite(tx, HIGH);
  _transmitBitMask = digitalPinToBitMask(tx);
	uint8_t port = digitalPinToPort(tx);
  _transmitPortRegister = portOutputRegister(port);
}

void NewSoftSerial::setRX(uint8_t rx)
{
  pinMode(rx, INPUT);
  digitalWrite(rx, HIGH);  // pullup!
  _receivePin = rx;
  _receiveBitMask = digitalPinToBitMask(rx);
 	uint8_t port = digitalPinToPort(rx);
  _receivePortRegister = portInputRegister(port);
}

void NewSoftSerial::begin(long speed)
{
  _rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;

  for (unsigned i=0; i<sizeof(table)/sizeof(table[0]); ++i)
  {
    long baud = pgm_read_dword(&table[i].baud);
    if (baud == speed)
    {
      _rx_delay_centering = pgm_read_word(&table[i].rx_delay_centering);
      _rx_delay_intrabit = pgm_read_word(&table[i].rx_delay_intrabit);
      _rx_delay_stopbit = pgm_read_word(&table[i].rx_delay_stopbit);
      _tx_delay = pgm_read_word(&table[i].tx_delay);
      break;
    }
  }

  // Set up RX interrupts, but only if we have a valid RX baud rate
  if (_rx_delay_stopbit)
  {
    if (_receivePin < 8) 
    {
      // a PIND pin, PCINT16-23
      PCICR |= _BV(2);
      PCMSK2 |= _BV(_receivePin);
    } 
    else if (_receivePin <= 13) 
    {
      // a PINB pin, PCINT0-7
      PCICR |= _BV(0);    
      PCMSK0 |= _BV(_receivePin-8);
    } 
    else if (_receivePin <= 21) 
    {
      // a PINC pin, PCINT8-14/15
      PCICR |= _BV(1);
      PCMSK1 |= _BV(_receivePin-14);
    } 

    tunedDelay(_tx_delay); // if we were low this establishes the end
  }

#if _DEBUG
  pinMode(13, OUTPUT);
  pinMode(11, OUTPUT);
#endif
  activate();
}

// Read data from buffer
int NewSoftSerial::read(void)
{
  uint8_t d;

  // A newly activated object never has any rx data
  if (activate())
    return -1;

  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  d = _receive_buffer[_receive_buffer_head]; // grab next byte
  _receive_buffer_head = (_receive_buffer_head + 1) % _NewSS_MAX_RX_BUFF;
  return d;
}

uint8_t NewSoftSerial::available(void)
{
  // A newly activated object never has any rx data
  if (activate())
    return 0;

  return (_receive_buffer_tail + _NewSS_MAX_RX_BUFF - _receive_buffer_head) % _NewSS_MAX_RX_BUFF;
}

size_t NewSoftSerial::write(uint8_t b)
{
  if (_tx_delay == 0)
    return 0;

  activate();

  uint8_t oldSREG = SREG;
  cli();  // turn off interrupts for a clean txmit

  // Write the start bit
  DebugPulse(13, 1);
  tx_pin_write(LOW);
  tunedDelay(_tx_delay + XMIT_START_ADJUSTMENT);

  // Write each of the 8 bits
  for (byte mask = 0x01; mask; mask <<= 1) 
  {
    if (b & mask) // choose bit
      tx_pin_write(HIGH); // send 1
    else
      tx_pin_write(LOW); // send 0
  
    DebugPulse(13, 1);
    tunedDelay(_tx_delay);
    DebugPulse(13, 1);
  }

  tx_pin_write(HIGH);
  SREG = oldSREG; // turn interrupts back on. hooray!
  tunedDelay(_tx_delay);
  DebugPulse(13, 1);
}

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
void NewSoftSerial::enable_timer0(bool enable) 
{ 
  if (enable) 
#if defined(__AVR_ATmega8__)
  	sbi(TIMSK, TOIE0);
#else
	  sbi(TIMSK0, TOIE0);
#endif
  else 
#if defined(__AVR_ATmega8__)
  	cbi(TIMSK, TOIE0);
#else
	  cbi(TIMSK0, TOIE0);
#endif
}

void NewSoftSerial::flush()
{
  if (active_object == this)
  {
    uint8_t oldSREG = SREG;
    cli();
    _receive_buffer_head = _receive_buffer_tail = 0;
    SREG = oldSREG;
  }
}
