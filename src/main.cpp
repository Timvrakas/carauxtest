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
  uint16_t param_len = 0;
  uint8_t checksum_calc = 0;
  uint8_t checksum_recv = 0;
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
          // Serial.println("================START================");
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
        msg.checksum_calc = d;
        msg.checksum_calc += e;
      }
      else
      {
        //Serial.println("At B");
        msg.length = c;
        msg.checksum_calc = c;
      }
      // Serial.print("Size: ");
      // Serial.println(msg.length);
      state = 2;
      pos = 0;
    }
    else if (state == 2)
    {
      msg.mode = c;
      msg.checksum_calc += c;
      // Serial.print("Mode: ");
      // Serial.println(msg.mode);
      state = 3;
    }
    else if (state == 3)
    {
      msg.cmd = c;
      msg.checksum_calc += c;
      // Serial.print("CMD: ");
      // Serial.print(c, 16);
      if (c == 0x00 || c == 0x01)
      {
        while (Serial1.available() < 1)
        {
          delay(10);
        }
        uint8_t d = Serial1.read();
        pos++;
        // Serial.print(" ");
        // Serial.print(d, 16);
        msg.cmd = msg.cmd | (d << 8);
        msg.checksum_calc += d;

        if (msg.mode == 2 && pos < msg.length)
        { // We only keep going in mode 2 and only if there are more bytes to in message
          {
            while (Serial1.available() < 1)
            {
              delay(10);
            }
            uint8_t e = Serial1.read();
            pos++;
            // Serial.print(" ");
            // Serial.print(e, 16);
            msg.cmd = msg.cmd | (e << 16);
            msg.checksum_calc += e;
            if (pos < msg.length) // we most be in mode 2, and we continue only if there is one more byte
            {
              while (Serial1.available() < 1)
              {
                delay(10);
              }
              uint8_t f = Serial1.read();
              pos++;
              // Serial.print(" ");
              // Serial.print(f, 16);
              msg.cmd = msg.cmd | (f << 24);
              msg.checksum_calc += f;
            }
          }
        }
      }
      //Serial.println();
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
      msg.param_len = msg.length - pos + 1;
      msg.params[0] = c;
      Serial1.readBytes(msg.params + 1, msg.length - pos);
      //Serial.print("Read Parameter Bytes: ");
      //Serial.println(read + 1);
      // Serial.print("Parameters: ");
      for (size_t i = 0; i < msg.param_len; i++)
      {
        msg.checksum_calc += msg.params[i];
      }
      // Serial.println();
      state = 5;
    }
    else if (state == 5)
    {
      msg.checksum_calc = (msg.checksum_calc ^ 0xFF) + 1;
      msg.checksum_recv = c;
      msg.valid = true;
      if (msg.checksum_calc != msg.checksum_recv)
      {
        msg.valid = false;
        Serial.print("Checksum Error! Got: ");
        Serial.print(msg.checksum_recv, 16);
        Serial.print(", calculated: ");
        Serial.println(msg.checksum_calc, 16);
      }
      break;
    }
  }
  // Serial.println("----------------DONE----------------");
  // Serial.println();
  return msg;
}

void print_msg(msg_t msg)
{
  Serial.println("================START================");
  Serial.print("length: ");
  Serial.println(msg.length);
  Serial.print("mode: ");
  Serial.println(msg.mode);
  Serial.print("cmd: ");
  Serial.println(msg.cmd, 16);
  Serial.print("params: ");
  for (size_t i = 0; i < msg.param_len; i++)
  {
    Serial.print(msg.params[i], 16);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println(msg.valid ? "Valid Checksum" : "Invalid Checksum");
  Serial.println("----------------DONE----------------");
  Serial.println();
}

void loop()
{
  while (Serial1.available() > 1)
  {
    msg_t msg = parse();

    if (msg.valid)
    {
      if (msg.mode == 4)
      {
        switch (msg.cmd)
        {
        case 0x3200:
        {
          Serial.println("Uploading Picture");
          const uint8_t resp[] = {0xFF, 0x55, 0x06, 0x04, 0x00, 0x01, 0x00, 0x00, 0x32, 0xC3};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x2C00:
        {
          Serial.println("Shuffle Mode = 0");
          const uint8_t resp[] = {0xFF, 0x55, 0x04, 0x04, 0x00, 0x2D, 0x00, 0xCB};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x2F00:
        {
          Serial.println("Repeat Mode = 0");
          const uint8_t resp[] = {0xFF, 0x55, 0x04, 0x04, 0x00, 0x30, 0x00, 0xC8};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x1600:
        {
          Serial.println("Switching to Main Library");
          const uint8_t resp[] = {0xFF, 0x55, 0x06, 0x04, 0x00, 0x01, 0x00, 0x00, 0x16, 0xDF};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x1800:
        {
          Serial.println("Get Count of ???");
          const uint8_t resp[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01, 0xDB};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x2000:
        {
          //Serial.println("Get Song Title");
          const uint8_t resp[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x21, 0x54, 0x49, 0x4d, 0x00, 0xEA};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x2200:
        {
          //Serial.println("Get Song Artist");
          const uint8_t resp[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x23, 0x54, 0x49, 0x4d, 0x00, 0xE8};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x2400:
        {
          //Serial.println("Get Song Album");
          const uint8_t resp[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x25, 0x54, 0x49, 0x4d, 0x00, 0xE6};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x3500:
        {
          //Serial.println("Get number of songs in this playlist");
          const uint8_t resp[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x36, 0x00, 0x00, 0x00, 0x05, 0xBA};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x1E00:
        {
          //Serial.println("Get Playlist Position");
          const uint8_t resp[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x02, 0xD4};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x1C00:
        {
          //Serial.println("Get Time/Status info");
          const uint8_t resp[] = {0xFF, 0x55, 0x07, 0x04, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x01, 0xD5};
          Serial1.write(resp, sizeof(resp));
          break;
        }

        case 0x2900:
        {
          Serial.print("Mode 4 Command: ");
          switch (msg.params[0])
          {
          case 0x01:
            Serial.println("Play/Pause");
            break;

          case 0x02:
            Serial.println("Stop");
            break;

          case 0x03:
            Serial.println("Skip++");
            break;

          case 0x04:
            Serial.println("Skip--");
            break;

          case 0x05:
            Serial.println("FF");
            break;

          case 0x06:
            Serial.println("RW");
            break;

          case 0x07:
            Serial.println("Stop FF/RW");
            break;

          default:
            Serial.println("Unknown");
            break;
          }
          break;
        }

        case 0x3700:
        {
          Serial.print("Jump to Song: ");
          Serial.println(msg.params[3]);
          break;
        }

        default:
          print_msg(msg);
          break;
        }
      }
      else if (msg.mode == 2)
      {
        switch (msg.cmd)
        {
        case 0x0:
        {
          Serial.println("Clear Key Presses");
          break;
        }

        case 0x2000000:
        {
          Serial.println("Unknown 00 00 00 02 Key Press");
          break;
        }

        default:
          print_msg(msg);
          break;
        }
      }
      else
      {
        switch (msg.cmd)
        {
        case 0x401:
        {
          Serial.println("Switch to Mode 4");
          break;
        }

        case 0x201:
        {
          Serial.println("Switch to Mode 2");
          break;
        }

        default:
          print_msg(msg);
          break;
        }
      }
    }
  }

  while (Serial.available() > 0)
  {
    uint8_t c = Serial.read();
    if (c == 'a')
    {
      Serial.println("Sending Picture Upload");
      const uint8_t resp[] = {0xFF, 0x55, 0x06, 0x04, 0x00, 0x01, 0x00, 0x00, 0x32, 0xC3};
      Serial1.write(resp, sizeof(resp));
    }
  }

  delay(10);
}