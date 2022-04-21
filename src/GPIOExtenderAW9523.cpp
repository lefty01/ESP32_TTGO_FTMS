/**
 *
 *
 * The MIT License (MIT)
 * Copyright © 2022 <Andreas Loeffler> <Zingo Andersen>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the “Software”), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "GPIOExtenderAW9523.h"

bool GPIOExtenderAW9523::begin()
{
    enabled = false;
    isInterrupted = false;
    uint8_t id = read(ID);
    Serial.printf("GPIOExtenderAW9523: Read ID: 0x%x = 0x%x\n",ID,id);

    if (id != ID_AW9523)
    {
      return false;
    }

    if (!write(OUTPUT_PORT0,0xff)) return false; //0-low 1-high
    if (!write(OUTPUT_PORT1,0xff)) return false; //0-low 1-high

    // All input
    if (!write(CONFIG_PORT0,0xff)) return false; //0-output 1-input
    if (!write(CONFIG_PORT1,0xff)) return false; //0-output 1-input

    if (!write(CTL,0x08)) return false; //0000x000  0-Open Drain 1-Push-Pull

#ifdef AW9523_IRQ_MODE
    if (!write(INT_PORT0,0x00)) return false; //0-enable 1-disable
    if (!write(INT_PORT1,0x00)) return false; //0-enable 1-disable
#else
    if (!write(INT_PORT0,0xff)) return false; //0-enable 1-disable
    if (!write(INT_PORT1,0xff)) return false; //0-enable 1-disable
#endif
    if (!write(LED_MODE_SWITCH0,0xff)) return false; //0-LED mode 1-GPIO mode
    if (!write(LED_MODE_SWITCH1,0xff)) return false; //0-LED mode 1-GPIO mode
    enabled = true;
    getPins();
    return true;
}

bool GPIOExtenderAW9523::isAvailable()
{
    return enabled;
}

uint16_t GPIOExtenderAW9523::getPins(void)
{
    if (enabled)
    {
      isInterrupted = false;
      uint8_t port0 = read(INPUT_PORT0);
      uint8_t port1 = read(INPUT_PORT1);

#ifdef AW9523_IRQ_MODE
      uint8_t int0 = read(INT_PORT0);
      uint8_t int1 = read(INT_PORT0);
      Serial.printf("GPIOExtenderAW9523 Read port:0x%x%x  interrupt:0x%x%x \n",port1,port0,int0,int1);
#endif
      return ((port1 << 8) | port0);
    }
    return 0;
}  

bool GPIOExtenderAW9523::checkInterrupt(void)
{
    return (enabled && isInterrupted);
}

#ifdef AW9523_IRQ_MODE
void IRAM_ATTR GPIOExtenderAW9523::gotInterrupt(void)
{
    Serial.printf("AW9523 Interrupt\n");

    if (enabled)
    {
      // let set this and poll this bit later to avoid I2C reads in the interrupt routine
      isInterrupted = true;
      // TODO Is it ok to read I2C here? If so we can rework this for simpler code
      //      to maybe avoid the poll in loop()
    }
}
#endif

void GPIOExtenderAW9523::loopHandler(void)
{
    if (enabled)
    {

#ifdef AW9523_IRQ_MODE
        if (GPIOExtender.checkInterrupt())
#endif
        {
            static uint16_t prevKeys = 0;
            uint16_t pins = getPins();
            uint16_t keys = ((~pins) & 0x3f );

            if ( keys != prevKeys)
            {
            // New key status
            //Serial.printf("GPIOExtender KEY IN:0x%x (0x%x)\n",keys,pins);

            if((keys & AW9523_KEY_UP) && AW9523_KEY_UP)
            {
                Serial.printf("AW9523_KEY_UP\n");
                handle_event(EventType::KEY_UP);
                //handle_event(EventType::TREADMILL_INC_UP);
            }
            if((keys & AW9523_KEY_LEFT) && AW9523_KEY_LEFT)
            {
                Serial.printf("AW9523_KEY_LEFT\n");
                handle_event(EventType::KEY_LEFT);
                //handle_event(EventType::TREADMILL_SPEED_DOWN);
            }
            if((keys & AW9523_KEY_DOWV) && AW9523_KEY_DOWV)
            {
                Serial.printf("AW9523_KEY_DOWV\n");
                handle_event(EventType::KEY_DOWN);
                //handle_event(EventType::TREADMILL_INC_DOWN);
            }
            if((keys & AW9523_KEY_RIGHT) && AW9523_KEY_RIGHT)
            {
                Serial.printf("AW9523_KEY_RIGHT\n");
                handle_event(EventType::KEY_RIGHT);
                //handle_event(EventType::TREADMILL_SPEED_UP);
            }
            if((keys & AW9523_KEY_OK) && AW9523_KEY_OK)
            {
                Serial.printf("AW9523_KEY_OK\n");
                handle_event(EventType::KEY_OK);
                //handle_event(EventType::TREADMILL_START);
            }
            if((keys & AW9523_KEY_BACK) && AW9523_KEY_BACK)
            {
                Serial.printf("AW9523_KEY_BACK\n");
                handle_event(EventType::KEY_BACK);
                //handle_event(EventType::TREADMILL_STOP);
            }

            if((keys & 0xffc0)  != 0)  Serial.printf("Non key pins: 0x%x\n",keys);
            prevKeys = keys;
            }
        }
    }
}

void GPIOExtenderAW9523::logPins(void)
{
    if (enabled) {
        uint8_t port0 = read(INPUT_PORT0);
        uint8_t port1 = read(INPUT_PORT1);
        Serial.printf("GPIOExtenderAW9523 Read port:0x%x%x\n",port1,port0);
    }
}

// Currently only support one button at time
bool GPIOExtenderAW9523::pressEvent(EventType eventButton)
{
    Serial.printf("GPIOExtenderAW9523: pressEvent(0x%x)\n",static_cast<uint32_t>(eventButton));
    if (enabled)
    {
        // convert event to HW line
        uint16_t line=-1;
        switch(eventButton)
        {
            case EventType::TREADMILL_START:      line = AW9523_TREADMILL_START; break;
            case EventType::TREADMILL_SPEED_DOWN: line = AW9523_TREADMILL_SPEED_DOWN; break;
            case EventType::TREADMILL_INC_DOWN:   line = AW9523_TREADMILL_INC_DOWN; break;
            case EventType::TREADMILL_STOP:       line = AW9523_TREADMILL_STOP; break;
            case EventType::TREADMILL_SPEED_UP:   line = AW9523_TREADMILL_SPEED_UP; break;
            case EventType::TREADMILL_INC_UP:     line = AW9523_TREADMILL_INC_UP; break;
            default:
                // Cant handle Event
                Serial.printf("GPIOExtenderAW9523 ERROR: Unknown event 0x%x\n",static_cast<uint32_t>(eventButton));
                return false;
        }
        if (line ==-1)
        {
            Serial.printf("GPIOExtenderAW9523 ERROR: event not converted to line 0x%x\n",static_cast<uint32_t>(eventButton));
            return false;
        }
        Serial.printf("GPIOExtenderAW9523: pressEvent(0x%x) -> 0x%x\n",static_cast<uint32_t>(eventButton),line);

        if (line & 0xff)
        {
            uint8_t port0Bit = line & 0xff;
            Serial.printf("GPIOExtenderAW9523: pressEvent(0x%x) -> 0x%x port0=0x%x ~x%x\n",static_cast<uint32_t>(eventButton),line,port0Bit,~port0Bit);

            // pinMode(pin, OUTPUT);
            if (!write(CONFIG_PORT0,~port0Bit)) return false; //0-output 1-input
            // digitalWrite(pin, LOW);
            if (!write(OUTPUT_PORT0,~port0Bit)) return false; //0-low 1-high

            delay(TREADMILL_BUTTON_PRESS_SIGNAL_TIME_MS);

            // digitalWrite(pin, HIGH);
            if (!write(OUTPUT_PORT0,0xff)) return false; //0-low 1-high
            // pinMode(pin, INPUT);
            if (!write(CONFIG_PORT0,0xff)) return false; //0-output 1-input
        }
        else
        {
            uint8_t port1Bit = (line & 0xff00) >> 8;
            Serial.printf("GPIOExtenderAW9523: pressEvent(0x%x) -> 0x%x port1=0x%x ~x%x\n",static_cast<uint32_t>(eventButton),line,port1Bit,~port1Bit);
            // pinMode(pin, OUTPUT);
            if (!write(CONFIG_PORT1,~port1Bit)) return false; //0-output 1-input
            // digitalWrite(pin, LOW);
            if (!write(OUTPUT_PORT1,~port1Bit)) return false; //0-low 1-high

            delay(TREADMILL_BUTTON_PRESS_SIGNAL_TIME_MS);

            // digitalWrite(pin, HIGH);
            if (!write(OUTPUT_PORT1,0xff)) return false; //0-low 1-high
            // pinMode(pin, INPUT);
            if (!write(CONFIG_PORT1,0xff)) return false; //0-output 1-input

        }
        return true;
    }
    return false;
}


uint8_t GPIOExtenderAW9523::read(uint8_t reg)
{
    wire->beginTransmission(AW9523_ADDR);
    wire->write(reg);
    wire->endTransmission(true);
    wire->requestFrom(AW9523_ADDR, static_cast<uint8_t>(1));
    return wire->read();
}

bool GPIOExtenderAW9523::write(uint8_t reg, uint8_t data)
{
    wire->beginTransmission(AW9523_ADDR);
    wire->write(reg);
    wire->write(data);
    uint8_t ret = wire->endTransmission();  // 0 if OK
    if (ret != 0)
    {
        Serial.printf("GPIOExtenderAW9523 ERROR: write reg: 0x%x value:0x%x -> Error: 0x%x\n",reg, data, ret);
        return false;
    }
    return true;
}
