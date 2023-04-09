/*
The MIT License (MIT)
Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.
Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "EventQueueMemPool.h"
#include <mbed.h>
#include <functional>
#include <string>

/**
 * Provides a simple broadcast radio abstraction, built upon the raw nrf51822 RADIO module.
 *
 * The nrf51822 RADIO module supports a number of proprietary modes of operation in addition to the typical BLE usage.
 * This class uses one of these modes to enable simple, point to multipoint communication directly between micro:bits.
 *
 * TODO: The protocols implemented here do not currently perform any significant form of energy management,
 * which means that they will consume far more energy than their BLE equivalent. Later versions of the protocol
 * should look to address this through energy efficient broadcast techniques / sleep scheduling. In particular, the GLOSSY
 * approach to efficienct rebroadcast and network synchronisation would likely provide an effective future step.
 *
 * TODO: Meshing should also be considered - again a GLOSSY approach may be effective here, and highly complementary to
 * the master/slave arachitecture of BLE.
 *
 * TODO: This implementation only operates whilst the BLE stack is disabled. The nrf51822 provides a timeslot API to allow
 * BLE to cohabit with other protocols. Future work to allow this colocation would be benefical, and would also allow for the
 * creation of wireless BLE bridges.
 *
 * NOTE: This API does not contain any form of encryption, authentication or authorization. It's purpose is solely for use as a
 * teaching aid to demonstrate how simple communications operates, and to provide a sandpit through which learning can take place.
 * For serious applications, BLE should be considered a substantially more secure alternative.
 */

// Status Flags
#define MICROBIT_RADIO_STATUS_INITIALISED       0x0001
#define MICROBIT_RADIO_STATUS_DEEPSLEEP_IRQ     0x0002
#define MICROBIT_RADIO_STATUS_DEEPSLEEP_INIT    0x0004

#define DEVICE_COMPONENT_STATUS_IDLE_TICK       0x1000 //whatever

// Default configuration values
#define MICROBIT_RADIO_BASE_ADDRESS             0x75626974 // ubit
#define MICROBIT_RADIO_DEFAULT_GROUP            111
#define MICROBIT_RADIO_DEFAULT_TX_POWER         7
#define MICROBIT_RADIO_DEFAULT_FREQUENCY        7
#define MICROBIT_RADIO_MAX_PACKET_SIZE          32
#define MICROBIT_RADIO_HEADER_SIZE              4
#define MICROBIT_RADIO_MAXIMUM_RX_BUFFERS       10
#define MICROBIT_RADIO_POWER_LEVELS             10

// Known Protocol Numbers
#define MICROBIT_RADIO_PROTOCOL_DATAGRAM        1       // A simple, single frame datagram. a little like UDP but with smaller packets. :-)
//#define MICROBIT_RADIO_PROTOCOL_EVENTBUS        2       // Transparent propogation of events from one micro:bit to another.

#define MICROBIT_DATAGRAM_INT          0
#define MICROBIT_DATAGRAM_KEY_INT      1
#define MICROBIT_DATAGRAM_STRING       2
#define MICROBIT_DATAGRAM_DOUBLE       4
#define MICROBIT_DATAGRAM_KEY_DOUBLE   5


#define DEVICE_OK                 0
#define DEVICE_INVALID_PARAMETER -1
#define DEVICE_NOT_SUPPORTED     -2

using stringT = String;

template <class T>
T getVal(uint8_t const * w) {
    T out = 0;
    uint8_t * ptr = (uint8_t*)&out;
    memcpy(ptr, w, sizeof(out));
    return out;
}

inline stringT getString(uint8_t const * w) { // first octet is length, rest are characters
    return { w+1, size_t(w[0]) };
}

namespace codal
{
    struct FrameBuffer
    {
        uint8_t         length;                             // The length of the remaining bytes in the packet. includes protocol/version/group fields, excluding the length field itself.
        uint8_t         version;                            // Protocol version code.
        uint8_t         group;                              // ID of the group to which this packet belongs.
        uint8_t         protocol;                           // Inner protocol number c.f. those issued by IANA for IP protocols

        uint8_t         payload[MICROBIT_RADIO_MAX_PACKET_SIZE];    // User / higher layer protocol data
        int             rssi;                               // Received signal strength of this frame.
    };

    class MicroBitRadio
    {
        volatile uint16_t status = 0;
        uint8_t           band   = MICROBIT_RADIO_DEFAULT_FREQUENCY; // The radio transmission and reception frequency band.
        uint8_t           power  = MICROBIT_RADIO_DEFAULT_TX_POWER;  // The radio output power level of the transmitter.
        uint8_t           group  = MICROBIT_RADIO_DEFAULT_GROUP;     // The radio group to which this micro:bit belongs.
        int               rssi   = 0;

        EventQueueMemPool<FrameBuffer, MICROBIT_RADIO_MAXIMUM_RX_BUFFERS>  rxQueue;

        rtos::Mutex       txMutex;
        std::unique_ptr<rtos::Thread> pollThread;

    public:
        static MicroBitRadio    *instance;  // A singleton reference, used purely by the interrupt service routine.

        MicroBitRadio();

        int setTransmitPower(int power);

        int setFrequencyBand(int band);

        FrameBuffer * getBuffer(bool commit = true) { return rxQueue.getBuffer(commit); }

        int setRSSI(int rssi);

        int getRSSI();

        int enable();

        int disable();

        int setGroup(uint8_t group);

        int send(FrameBuffer *buffer);

        uint8_t * initSendBuffer(FrameBuffer & fb, uint8_t type);

        int sendNumber(int num);
        int sendNumber(double num);
        int sendString(const char * str);
        int sendString(const char * str, size_t len);
        int sendKeyVal(const char * key, int val);
        int sendKeyVal(const char * key, double val);


        void handleQueue();                    // check queue and handle/decode received requests
        void decoder(FrameBuffer const & fb);  // decode and dispatch messages

        // datagram callbacks:
        std::function<void(int)>                        intCallback;
        std::function<void(double)>                     doubleCallback;
        std::function<void(stringT const &)>            strCallback;
        std::function<void(int, stringT const &)>       keyIntValCallback;
        std::function<void(double, stringT const &)>    keyDoubleValCallback;

        std::function<void(FrameBuffer const &)>        unknownCallback;

    private:
        void poll();           // check events and process filled buffers into the queue
        void pollLoop();       // check events and process filled buffers into the queue

    };
}