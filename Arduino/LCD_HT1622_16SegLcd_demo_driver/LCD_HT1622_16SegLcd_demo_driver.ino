
/*
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    Arduino demo/driver for HT1622-based 16 segment LCDs

    Copyright (c) 2015-2021 Martin F. Falatic
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Arduino is little-endian
H162x commands and addresses are MSB-first (3 bit mode + 9 bit command code)
Note that the very last bit is X (don't care)
Data is LSB-first, in address-sequential 4-bit nibbles as desired.
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
*/

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Helper functions and variables
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// HT162x commands (Start with 0b100, ends with one "don't care" byte)
#define  CMD_SYS_DIS  0x00  // SYS DIS    (0000-0000-X) Turn off system oscillator, LCD bias gen [Default]
#define  CMD_SYS_EN   0x01  // SYS EN     (0000-0001-X) Turn on  system oscillator
#define  CMD_LCD_OFF  0x02  // LCD OFF    (0000-0010-X) Turn off LCD display [Default]
#define  CMD_LCD_ON   0x03  // LCD ON     (0000-0011-X) Turn on  LCD display
#define  CMD_RC_INT   0x18  // RC INT     (0001-10XX-X) System clock source, on-chip RC oscillator
#define  CMD_BIAS_COM 0x29  // BIAS & COM (0010-10X1-X) 1/3 bias, 4 commons // HT1621 only

#define  HT1622_ADDRS 0x40  // HT1622 has 64 possible 4-bit addresses

#define CS   27 // Active low
#define WR   15 // Active low
#define DATA  4

#define LSB_FORMAT  true
#define MSB_FORMAT  false

#define SETUP_DELAY_USECS 1
#define HOLD_DELAY_USECS  1
#define WRITE_DELAY_USECS 2  // Tclk min. on data sheet - overhead is more than this at low clock speeds
#define RESET_DELAY_USECS 1000  // Not strictly necessary

#define NUM_DIGITS  9

const uint8_t digitAddr[NUM_DIGITS] = {
  0x00, 0x14, 0x04, 0x18, 0x08, 0x1C, 0x0C, 0x22, 0x10,
};

// TODO
//boolean VAL_DP1 = false;
//boolean VAL_DP2 = false;
//boolean VAL_DP3 = false;
//boolean VAL_HZ  = false;

// Based on a font presented at http://windways.org/personal_page/stockticker/
const uint16_t SegCharDataLSBfirst[] = {
  0x0000, 0x004C, 0x0050, 0x5511, 0xD99D, 0xD33D, 0xBC1A, 0x0010, //  !"#$%&'
  0x0022, 0x2200, 0x3333, 0x1111, 0x0020, 0x1001, 0x0008, 0x0220, // ()*+,-./
  0xCEEC, 0x0064, 0x9CC9, 0x88CD, 0x5045, 0xD88D, 0xDC0D, 0x80C4, // 01234567
  0xDCCD, 0xD0CD, 0x0110, 0x0210, 0x1022, 0x1809, 0x2201, 0x81C1, // 89:;<=>?
  0xCCE9, 0xD4C5, 0x89DD, 0xCC88, 0x89DC, 0xDC88, 0xD480, 0xCC8D, // @ABCDEFG
  0x5445, 0x8998, 0x0C4C, 0x5422, 0x4C08, 0x6464, 0x6446, 0xCCCC, // HIJKLMNO
  0xD4C1, 0xCCCE, 0xD4C3, 0xD88D, 0x8190, 0x4C4C, 0x4620, 0x4646, // PQRSTUVW
  0x2222, 0x5141, 0x8AA8, 0x0198, 0x2002, 0x8910, 0x0202, 0x0808, // XYZ[\]^_
  0x2000, 0x3D00, 0x5D00, 0x1C00, 0x1D10, 0x1E08, 0x1191, 0xD910, // `abcdefg
  0x5500, 0x0100, 0x0D10, 0x0132, 0x0110, 0x1505, 0x1500, 0x1D00, // hijklmno
  0xD410, 0xD110, 0x1400, 0xD900, 0x1111, 0x0D00, 0x0600, 0x0606, // pqrstuvw
  0x2222, 0x2120, 0x1A00, 0x1198, 0x4400, 0x8911, 0x2020, 0xFFFF, // xyz{|}~
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
char * valToHex(int val, int prec, const char * prefix) {
      static char fmt[32], output[32];
      sprintf(fmt, "%s%%0%dX", prefix, prec);
      sprintf(output, fmt, val);
      return output;
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Send up to 16 bits, MSB (default) or LSB
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void HT162x_SendBits(uint16_t data, uint8_t bits, boolean LSB_FIRST = MSB_FORMAT)
{
  // Data is shifted out bitwise, either LSB-first or MSB-first.
  // The mask is used to isolate the bit being transmitted.

  uint16_t mask = LSB_FIRST ? 1 : 1 << bits-1;

  for (uint8_t i = bits; i > 0; i--)
  {
    delayMicroseconds(WRITE_DELAY_USECS);
    digitalWrite(WR, LOW);
    data & mask ? digitalWrite(DATA, HIGH) : digitalWrite(DATA, LOW);
    delayMicroseconds(WRITE_DELAY_USECS);
    digitalWrite(WR, HIGH);
    delayMicroseconds(HOLD_DELAY_USECS);
    LSB_FIRST ? data >>= 1 : data <<= 1;
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void HT162x_Command(uint8_t cmd)
{
  delayMicroseconds(SETUP_DELAY_USECS);
  digitalWrite(CS, LOW);
  delayMicroseconds(SETUP_DELAY_USECS);
  HT162x_SendBits(0b100, 3);
  HT162x_SendBits(cmd, 8);
  HT162x_SendBits(1, 1);
  delayMicroseconds(SETUP_DELAY_USECS);
  digitalWrite(CS, HIGH);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void HT162x_WriteData(uint8_t addr, uint16_t sdata, uint8_t bits = 4)
{
  // Note: bits needs to be a multiple of 4 as data is in nibbles
  delayMicroseconds(SETUP_DELAY_USECS);
  digitalWrite(CS, LOW);
  delayMicroseconds(SETUP_DELAY_USECS);
  HT162x_SendBits(0b101, 3);
  HT162x_SendBits(addr, 6);
  for (int n = (bits/4)-1; n >= 0; n--) {
    uint16_t nib = (sdata & (0xf) << 4*n) >> (4*n);
    HT162x_SendBits(nib, 4, LSB_FORMAT);
  }
  delayMicroseconds(SETUP_DELAY_USECS);
  digitalWrite(CS, HIGH);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void AllElements(uint8_t state)
{
  for (uint8_t addr = 0; addr < HT1622_ADDRS; addr++)
  {
    HT162x_WriteData(addr, (state ? 0xF : 0x0), 4);
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void AllSegments(uint8_t state)
{
  for (uint8_t pos = 0; pos < NUM_DIGITS; pos++)
  {
    HT162x_WriteData(digitAddr[pos], (state ? 0xFFFF : 0x0000), 16);
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void RandomElements(int segDelay = 1, int cycles = 9)
{
  for (int cnt = 0; cnt < cycles; cnt++)
  {
    uint8_t addr = random(HT1622_ADDRS);
    uint16_t sdata = random(0xF+1);
    HT162x_WriteData(addr, sdata, 4);
    delay(segDelay);
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void RandomSegments(int segDelay = 1, int cycles = 1)
{
  for (int cnt = 0; cnt < cycles; cnt++)
  {
    for (int n = 0; n < NUM_DIGITS; n++)
    {
      uint8_t addr = digitAddr[n] + random(4);
      uint16_t sdata = random(0xF+1);
      HT162x_WriteData(addr, sdata, 4);
      delay(segDelay);
    }
  }
}
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void TestSegments(int segDelay = 200, boolean clear_as = true, int cycles = 1)
{
  for (int cnt = 0; cnt < cycles; cnt++)
  {
    for (uint8_t pos = 0; pos < NUM_DIGITS; pos++)
    {
      // Test cycle (single uint16_t writes)
      uint16_t sdata = 0;
      for (uint8_t i = 0x00; i < 16; i++) {
        sdata = clear_as ? 1 << i: sdata | (1 << i);
        HT162x_WriteData(digitAddr[pos], sdata, 16);
        delay(segDelay);
      }
      if (clear_as) {
        HT162x_WriteData(digitAddr[pos], 0, 16);
      }
    }
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void TestChars(int CharDelay = 150, int cycles = 1)
{
  int aSize = ARRAY_SIZE(SegCharDataLSBfirst);

  for (int cnt = 0; cnt < cycles; cnt++)
  {
    int charData = 0;
    for (int n = 0; n < aSize / NUM_DIGITS * 2; n++)
    {
      for (uint8_t pos = 0; pos < NUM_DIGITS; pos++)
      {
        HT162x_WriteData(digitAddr[pos], SegCharDataLSBfirst[charData], 16);
        delay(CharDelay);
        charData = (charData < aSize-1 ? charData+1 : 0);
      }
    }
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void SegmentIdentifier()
{
  Serial.println("Press keys to idenitify segments");
  Serial.println("('d' to move forward and 'a' to move backward)");
  uint8_t  addr = 0;  // 0 to max addr
  uint8_t  spos = 0;  // 0 to 3
  int keypress = -2;
  while (1) {
    if (keypress == -2) {
      keypress = -1;
    }
    else {
      keypress = Serial.read();
      if (keypress == -1) {
        continue;
      }
    }

    uint8_t oldAddr = addr;
    if (keypress == 'd') {
      spos += 1;
      if (spos > 3) {
        spos = 0;
        addr += 1;
      }
      if (addr >= HT1622_ADDRS) {
        addr = 0;
      }
    }
    else if (keypress == 'a') {
      spos -= 1;
      if (spos > 3) {
        spos = 3;
        addr -= 1;
      }
      if (addr >= HT1622_ADDRS) {
        addr = HT1622_ADDRS - 1;
      }
    }
    else if (keypress == 'q' || keypress == 0x1B) {
      break;
    }

    Serial.print("  Keypress = ");
    Serial.print(valToHex(keypress, 4, "0x"));
    HT162x_WriteData(oldAddr, 0, 4);  // Clear current segment
    uint16_t sdata = 1 << spos;
    HT162x_WriteData(addr, sdata, 4);
    Serial.print("  Addr = ");
    Serial.print(valToHex(addr, 2, "0x"));
    Serial.print("  Data = ");
    Serial.print(valToHex(sdata, 1, "0x"));
    Serial.println();
  };
  Serial.print("  Final keypress = ");
  Serial.print(valToHex(keypress, 4, "0x"));
  Serial.println();
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//#############################################################################
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void setup()
{
  Serial.begin(115200);
  delayMicroseconds(RESET_DELAY_USECS);
  Serial.println();
  Serial.println("Initializing");
  // Set up I/O and display
  pinMode(CS, OUTPUT);
  pinMode(WR, OUTPUT);
  pinMode(DATA, OUTPUT);
  delay(50);

  HT162x_Command(CMD_SYS_EN);
  HT162x_Command(CMD_RC_INT);
  //HT162x_Command(CMD_BIAS_COM); // Only for HT1621
  HT162x_Command(CMD_LCD_OFF);
  AllElements(0);
  HT162x_Command(CMD_LCD_ON); // Should turn it back on
  delay(1000);
  Serial.println("Initialization complete");
  Serial.println();
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//#############################################################################
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void loop()
{
  AllElements(1);
  delay(1000);
  AllElements(0);
  delay(1000);
  AllSegments(1);
  delay(1000);
  AllSegments(0);
  delay(1000);

  SegmentIdentifier();

  RandomElements(10, 900);
  AllElements(0);
  
  RandomSegments(10, 100);
  AllElements(0);
  
  TestSegments(25, false, 1);
  AllElements(0);
  
  TestSegments(100, true, 1);
  AllElements(0);
  
  TestChars(50, 1);
  AllElements(0);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
