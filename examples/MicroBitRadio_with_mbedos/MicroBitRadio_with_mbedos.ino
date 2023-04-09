#include <MicroBitRadio_mbedos.h>
#include <PrintStream.h>

#include <Scheduler.h>

codal::MicroBitRadio radio;


void printPacket(codal::FrameBuffer const & fb) {
    Serial << "L: " << (int)fb.length << " V: " << (int)fb.version << " G: " << (int)fb.group << " P:" << (int)fb.protocol << " Rssi: " << fb.rssi << "; ";
  
    for (uint8_t i = 0; i < min(32,fb.length); ++i) {
        hex(Serial) << fb.payload[i] << ((i&7) == 0 ? "  " : " ");
    }
    dec(Serial) << " -> ";

    switch (fb.payload[0]) {
        case 0: Serial << getVal<int32_t>(fb.payload+9); break;
        case 1: Serial << (const char*)(fb.payload+14) << "=" << getVal<int32_t>(fb.payload+9); break;
        case 2: Serial << (const char*)(fb.payload+10); break;
        case 4: Serial.print(getVal<double>(fb.payload+9)); break;
        case 5: Serial << (const char*)(fb.payload+18) << "="; Serial.print(getVal<double>(fb.payload+9)); break;
    }
    Serial.println();
}

void radioSender() {
    delay(100);
    radio.sendNumber((int)random(0,100));
}


void setup() {
    Serial.begin(115200);

    while (!Serial);

    radio.unknownCallback       = &printPacket; // unknown packets are printed here

    radio.intCallback           = [](int i){ Serial.println(i); };
    radio.doubleCallback        = [](double d){ Serial.println(std::to_string(d).c_str()); };
    radio.keyIntValCallback     = [](int i, stringT const& key){ Serial << key << "=" << i << "\n"; };
    radio.keyDoubleValCallback  = [](double d, stringT const & key){ Serial << key << "=" << d << "\n"; };
    radio.strCallback           = [](stringT const& str) { Serial.println(str); };

    //String
    Serial.println(radio.enable());

}

void loop() {
    radio.handleQueue();
    yield();
}

