#include <Arduino.h>

void setup()
{
  delay(3000);
  // pinMode(1, OUTPUT);
  // digitalWrite(1, LOW);
  // pinMode(4, OUTPUT);
  // digitalWrite(4, LOW);
  Serial.begin(19200);
  Serial1.begin(19200);
  Serial.println("Setup!");
}

typedef struct msg_t
{
  uint16_t length = 0;
  uint8_t mode = 0;
  uint32_t cmd = 0x00000000;
  uint8_t params[1024];
  uint16_t param_len;
  uint8_t checksum = 0;
  bool valid = false;
} msg_t;

msg_t parse()
{

  msg_t msg;

  int state = 0;
  // 0 = Waiting for header
  // 1 = Waiting for size
  // 2 = Waiting for mode
  // 3 = Waiting for cmd
  // 4 = Waiting for params
  // 5 = Waiting for chksum

  uint16_t pos = 0;

  while (true)
  {
    while (Serial1.available() < 1)
    {
      delay(10);
    }

    // if (pos >= msg.length)
    // {
    //   state = 0;
    //   pos = 0;
    // }
    uint8_t c = Serial1.read();
    pos++;

    if (state == 0)
    {
      if (c == 0xFF)
      {
        while (Serial1.available() < 1)
        {
          delay(10);
        }
        c = Serial1.read();
        if (c == 0x55)
        {
          state = 1;
          msg.length = 3;
          Serial.println("================START================");
        }
      }
    }
    else if (state == 1)
    {
      //Serial.print(c, 16);
      if (c == 0x00)
      {
        //Serial.println("At A");
        while (Serial1.available() < 2)
        {
          delay(10);
        }
        uint8_t d = Serial1.read();
        uint8_t e = Serial1.read();
        msg.length = (d << 8) | e;
        msg.checksum = d;
        msg.checksum += e;
      }
      else
      {
        //Serial.println("At B");
        msg.length = c;
        msg.checksum = c;
      }
      Serial.print("Size: ");
      Serial.println(msg.length);
      state = 2;
      pos = 0;
    }
    else if (state == 2)
    {
      msg.mode = c;
      msg.checksum += c;
      Serial.print("Mode: ");
      Serial.println(msg.mode);
      state = 3;
    }
    else if (state == 3)
    {
      msg.cmd = c;
      msg.checksum += c;
      Serial.print("CMD: ");
      Serial.print(c, 16);
      if (c == 0x00 || c == 0x01)
      {
        while (Serial1.available() < 1)
        {
          delay(10);
        }
        uint8_t d = Serial1.read();
        pos++;
        Serial.print(" ");
        Serial.print(d, 16);
        msg.cmd = msg.cmd | (d << 8);
        msg.checksum += d;

        if (msg.mode == 2 && pos < msg.length)
        { // We only keep going in mode 2 and only if there are more bytes to in message
          {
            while (Serial1.available() < 1)
            {
              delay(10);
            }
            uint8_t e = Serial1.read();
            pos++;
            Serial.print(" ");
            Serial.print(e, 16);
            msg.cmd = msg.cmd | (e << 16);
            msg.checksum += e;
            if (pos < msg.length) // we most be in mode 2, and we continue only if there is one more byte
            {
              while (Serial1.available() < 1)
              {
                delay(10);
              }
              uint8_t f = Serial1.read();
              pos++;
              Serial.print(" ");
              Serial.print(f, 16);
              msg.cmd = msg.cmd | (f << 24);
              msg.checksum += f;
            }
          }
        }
      }
      Serial.println();
      //Serial.print("CMD: ");
      //Serial.println(msg.cmd, 16);

      if (pos < msg.length)
      {
        state = 4;
      }
      else
      {
        state = 5;
      }
    }
    else if (state == 4)
    {
      msg.param_len = msg.length - pos;
      msg.params[0] = c;
      Serial1.readBytes(msg.params + 1, msg.length - pos);
      //Serial.print("Read Parameter Bytes: ");
      //Serial.println(read + 1);
      Serial.print("Parameters: ");
      for (size_t i = 0; i < (msg.length - pos + 1); i++)
      {
        Serial.print(msg.params[i], 16);
        Serial.print(" ");
        msg.checksum += msg.params[i];
      }
      Serial.println();
      state = 5;
    }
    else if (state == 5)
    {
      msg.checksum = (msg.checksum ^ 0xFF) + 1;
      msg.valid = true;
      if (msg.checksum != c)
      {
        msg.valid = false;
        Serial.print("Checksum Error! Got: ");
        Serial.print(c, 16);
        Serial.print(", calculated: ");
        Serial.println(msg.checksum, 16);
      }
      break;
    }
  }
  Serial.println("----------------DONE----------------");
  Serial.println();
  return msg;
}

const uint8_t respA[] = {0xFF, 0x55, 0x06, 0x04, 0x00, 0x01, 0x00, 0x00, 0x32, 0xC3};

const uint8_t respB[] = {0xFF, 0x55, 0x04, 0x04, 0x00, 0x2D, 0x00, 0xCB};

const uint8_t respC[] = {0xFF, 0x55, 0x04, 0x04, 0x00, 0x30, 0x00, 0xC8};

const uint8_t respD[] = {0xFF, 0x55, 0x06, 0x04, 0x00, 0x01, 0x00, 0x00, 0x16, 0xDF};

const uint8_t respE[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01, 0xDB};

const uint8_t respF[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x21, 0x54, 0x49, 0x4d, 0x00, 0xEA};

const uint8_t respG[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x23, 0x54, 0x49, 0x4d, 0x00, 0xE8};

const uint8_t respH[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x25, 0x54, 0x49, 0x4d, 0x00, 0xE6};

const uint8_t respI[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x36, 0x00, 0x00, 0x00, 0x01, 0xBE};

const uint8_t respJ[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x01, 0xD5};

const uint8_t respK[] = {0xFF, 0x55, 0x0C, 0x04, 0x00, 0x1D, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0xC1};

void loop()
{

  while (Serial1.available() > 1)
  {
    msg_t msg = parse();
    if (msg.valid)
    {
      if (msg.mode == 4 && msg.cmd == 0x3200)
      { //Request Image
        Serial.println("Sending A!");
        Serial1.write(respA, sizeof(respA));
      }

      if (msg.mode == 4 && msg.cmd == 0x2C00)
      {
        Serial.println("Sending B!");
        Serial1.write(respB, sizeof(respB));
      }

      if (msg.mode == 4 && msg.cmd == 0x2F00)
      {
        Serial.println("Sending C!");
        Serial1.write(respC, sizeof(respC));
      }

      if (msg.mode == 4 && msg.cmd == 0x1600)
      {
        Serial.println("Sending D!");
        Serial1.write(respD, sizeof(respD));
      }

      if (msg.mode == 4 && msg.cmd == 0x1800)
      {
        Serial.println("Sending E !");
        Serial1.write(respE, sizeof(respE));
      }

      if (msg.mode == 4 && msg.cmd == 0x2000)
      {
        Serial.println("Sending F !");
        Serial1.write(respF, sizeof(respF));
      }

      if (msg.mode == 4 && msg.cmd == 0x2200)
      {
        Serial.println("Sending G !");
        Serial1.write(respG, sizeof(respG));
      }

      if (msg.mode == 4 && msg.cmd == 0x2400)
      {
        Serial.println("Sending H !");
        Serial1.write(respH, sizeof(respH));
      }

      if (msg.mode == 4 && msg.cmd == 0x3500)
      {
        Serial.println("Sending I !");
        Serial1.write(respI, sizeof(respI));
      }

      if (msg.mode == 4 && msg.cmd == 0x1E00)
      {
        Serial.println("Sending J !");
        Serial1.write(respJ, sizeof(respJ));
      }

      if (msg.mode == 4 && msg.cmd == 0x1C00)
      {
        Serial.println("Sending K !");
        Serial1.write(respK, sizeof(respK));
      }

    }
    // uint8_t c = Serial1.read();
    // Serial.println(c, 16);
  }

  while (Serial.available() > 0)
  {
    uint8_t c = Serial.read();
    if (c == 'a')
    {
      Serial.println("Sending!");
      Serial1.write(respA, sizeof(respA));
    }
  }

  delay(10);
}