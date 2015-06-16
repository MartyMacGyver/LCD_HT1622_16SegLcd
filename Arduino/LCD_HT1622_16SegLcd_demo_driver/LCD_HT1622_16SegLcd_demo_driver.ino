
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//    Arduino demo/driver for HT1622-based 16 segment LCDs
/*
    Copyright (c) 2015 Martin F. Falatic
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
/*
Board: ETM8809K2-02
Manufacturer: http://www.canton-electronics.com
Chipset: HT1622 or equivalent
LCD = 9 digit 16 seg + 3 dp + 'HZ' symbol + continuous backlight

Arduino is little-endian
H162x commands and addresses are MSB-first (3 bit mode + 9 bit command code)
Note that the very last bit is X (don't care)
Data is LSB-first, in address-sequential 4-bit nibbles as desired.

Addressing (each address hold 4 bits):
  Addr 0x00-0x03  = Digit 0
  Addr 0x04-0x07  = Digit 2
  Addr 0x08-0x0B  = Digit 4
  Addr 0x0C-0x0F  = Digit 6
  Addr 0x10-0x13  = Digit 8
  Addr 0x14-0x17  = Digit 1
  Addr 0x18-0x1B  = Digit 3
  Addr 0x1C-0x1F  = Digit 5
  Addr 0x21 bit 0 = Decimal 3
  Addr 0x21 bit 1 = Decimal 2
  Addr 0x22-0x25  = Digit 7
  Addr 0x27 bit 0 = Decimal 1
  Addr 0x27 bit 1 = Icon 'Hz'

Typical names for segments:

    /-----------\   /-----------\
   ||    'a'    || ||    'b'    ||
    \-----------/   \-----------/
   /-\ /--\      /-\      /--\ /-\
  |   |\   \    |   |    /   /|   |
  |   | \   \   |   |   /   / |   |
  |'h'|  \'k'\  |'m'|  /'n'/  |'c'|
  |   |   \   \ |   | /   /   |   |
  |   |    \   \|   |/   /    |   |
   \-/      \--/ \-/ \--/      \-/
    /-----------\   /-----------\
   ||    'u'    || ||    'p'    ||
    \-----------/   \-----------/
   /-\      /--\ /-\ /--\      /-\
  |   |    /   /|   |\   \    |   |
  |   |   /   / |   | \   \   |   |
  |'g'|  /'t'/  |'s'|  \'r'\  |'d'|
  |   | /   /   |   |   \   \ |   |
  |   |/   /    |   |    \   \|   |
   \-/ \--/      \-/      \--/ \-/
    /-----------\   /-----------\
   ||    'f'    || ||    'e'    ||
    \-----------/   \-----------/

Bit numbering via a little-endian uint16_t mapped to 4 addresses:

    /-----------\   /-----------\
   ||     C     || ||     4     ||
    \-----------/   \-----------/
   /-\ /--\      /-\      /--\ /-\
  |   |\   \    |   |    /   /|   |
  |   | \   \   |   |   /   / |   |
  | D |  \ E \  | 7 |  / 6 /  | 5 |
  |   |   \   \ |   | /   /   |   |
  |   |    \   \|   |/   /    |   |
   \-/      \--/ \-/ \--/      \-/
    /-----------\   /-----------\
   ||     F     || ||     3     ||
    \-----------/   \-----------/
   /-\      /--\ /-\ /--\      /-\
  |   |    /   /|   |\   \    |   |
  |   |   /   / |   | \   \   |   |
  | 9 |  / A /  | B |  \ 2 \  | 1 |
  |   | /   /   |   |   \   \ |   |
  |   |/   /    |   |    \   \|   |
   \-/ \--/      \-/      \--/ \-/
    /-----------\   /-----------\
   ||     8     || ||     0     ||
    \-----------/   \-----------/

Bit numbering over sequential addresses:

    /-----------\   /-----------\
   ||    0-0    || ||    2-0    ||
    \-----------/   \-----------/
   /-\ /--\      /-\      /--\ /-\
  |   |\   \    |   |    /   /|   |
  |   | \   \   |   |   /   / |   |
  |0-1|  \0-2\  |2-3|  /2-2/  |2-1|
  |   |   \   \ |   | /   /   |   |
  |   |    \   \|   |/   /    |   |
   \-/      \--/ \-/ \--/      \-/
    /-----------\   /-----------\
   ||    0-3    || ||    3-3    ||
    \-----------/   \-----------/
   /-\      /--\ /-\ /--\      /-\
  |   |    /   /|   |\   \    |   |
  |   |   /   / |   | \   \   |   |
  |1-1|  /1-2/  |1-3|  \3-2\  |3-1|
  |   | /   /   |   |   \   \ |   |
  |   |/   /    |   |    \   \|   |
   \-/ \--/      \-/      \--/ \-/
    /-----------\   /-----------\
   ||    1-0    || ||    3-0    ||
    \-----------/   \-----------/

Bit as int hex pos/bit values:

    /-----------\   /-----------\
   ||    1-1    || ||    3-1    ||
    \-----------/   \-----------/
   /-\ /--\      /-\      /--\ /-\
  |   |\   \    |   |    /   /|   |
  |   | \   \   |   |   /   / |   |
  |1-2|  \1-4\  |3-8|  /3-4/  |3-2|
  |   |   \   \ |   | /   /   |   |
  |   |    \   \|   |/   /    |   |
   \-/      \--/ \-/ \--/      \-/
    /-----------\   /-----------\
   ||    1-8    || ||    2-8    ||
    \-----------/   \-----------/
   /-\      /--\ /-\ /--\      /-\
  |   |    /   /|   |\   \    |   |
  |   |   /   / |   | \   \   |   |
  |0-2|  /0-4/  |0-8|  \2-4\  |2-2|
  |   | /   /   |   |   \   \ |   |
  |   |/   /    |   |    \   \|   |
   \-/ \--/      \-/      \--/ \-/
    /-----------\   /-----------\
   ||    0-1    || ||    2-1    ||
    \-----------/   \-----------/


*/
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Helper functions and variables
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

// Function that printf and related will use to print
int serial_putchar(char c, FILE* f) {
  if (c == '\n') serial_putchar('\r', f);
  return Serial.write(c) == 1 ? 0 : 1;
}

FILE serial_stdout;

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// HT162x commands (Start with 0b100, ends with one "don't care" byte)
#define  CMD_SYS_DIS  0x00  // SYS DIS    (0000-0000-X) Turn off system oscillator, LCD bias gen [Default]
#define  CMD_SYS_EN   0x01  // SYS EN     (0000-0001-X) Turn on  system oscillator
#define  CMD_LCD_OFF  0x02  // LCD OFF    (0000-0010-X) Turn off LCD display [Default]
#define  CMD_LCD_ON   0x03  // LCD ON     (0000-0011-X) Turn on  LCD display
#define  CMD_RC_INT   0x10  // RC INT     (0001-10XX-X) System clock source, on-chip RC oscillator
#define  CMD_BIAS_COM 0x29  // BIAS & COM (0010-10X1-X) 1/3 bias, 4 commons // HT1621 only

#define CS   8 // Active low
#define WR   9 // Active low
#define DATA 10

#define MSB_FORMAT  false
#define LSB_FORMAT  true
#define WRITE_DELAY_USECS 2  // Tclk min. on data sheet - overhead is more than this at low clock speeds

#define NUM_DIGITS  9

const uint8_t digitAddr[] = {0x00, 0x14, 0x04, 0x18, 0x08, 0x1C, 0x0C, 0x22, 0x10};

// Based on a font presented at http://windways.org/personal_page/stockticker/
const uint16_t SegCharData[] = {
  0x0000, 0x0023, 0x00A0, 0xAA88, 0xB99B, 0xBCCB, 0xD385, 0x0080, //  !"#$%&'
  0x0044, 0x4400, 0xCCCC, 0x8888, 0x0040, 0x8008, 0x0001, 0x0440, // ()*+,-./
  0x3773, 0x0062, 0x9339, 0x113b, 0xA02A, 0xB11B, 0xB30B, 0x1032, // 01234567
  0xb33b, 0xb03b, 0x0880, 0x0480, 0x8044, 0x8109, 0x4408, 0x1838, // 89:;<=>?
  0x3379, 0xB23A, 0x19BB, 0x3311, 0x19B3, 0xB311, 0xB210, 0x331B, // @ABCDEFG
  0xA22A, 0x1991, 0x0323, 0xA244, 0x2301, 0x6262, 0x6226, 0x3333, // HIJKLMNO
  0xB238, 0x3337, 0xB23C, 0xB11B, 0x1890, 0x2323, 0x2640, 0x2626, // PQRSTUVW
  0x4444, 0xA828, 0x1551, 0x0891, 0x4004, 0x1980, 0x0404, 0x0101, // XYZ[\]^_
  0x4000, 0xCB00, 0xAB00, 0x8300, 0x8B80, 0x8701, 0x8898, 0xB980, // `abcdefg
  0xAA00, 0x0800, 0x0B80, 0x08C4, 0x0880, 0x8A0A, 0x8A00, 0x8B00, // hijklmno
  0xB280, 0xB880, 0x8200, 0xB900, 0x8888, 0x0B00, 0x0600, 0x0606, // pqrstuvw
  0x4444, 0x4840, 0x8500, 0x8891, 0x2200, 0x1988, 0x4040, 0xFFFF, // xyz{|}~
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Send up to 16 bits, MSB (default) or LSB
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void HT162x_SendBits(uint16_t data, uint8_t bits, boolean LSB_FIRST = MSB_FORMAT)
{
  uint16_t mask;
  mask = (LSB_FIRST ? 1 : 1 << bits-1 );
  //printf("Writing %d bits of 0x%04x with mask 0x%04x in %s\n", bits, data, mask, LSB_FIRST?"LSB":"MSB");

  for (uint8_t i = bits; i > 0; i--)
  {
    digitalWrite(WR, LOW);
    //delayMicroseconds(WRITE_DELAY_USECS);
    data & mask ? digitalWrite(DATA, HIGH) : digitalWrite(DATA, LOW);
    delayMicroseconds(WRITE_DELAY_USECS);
    digitalWrite(WR, HIGH);
    delayMicroseconds(WRITE_DELAY_USECS);
    LSB_FIRST ? data >>= 1 : data <<= 1;
  }
  delayMicroseconds(WRITE_DELAY_USECS);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void HT162x_Command(uint8_t cmd)
{
  digitalWrite(CS, LOW);
  HT162x_SendBits(0b100, 3);
  HT162x_SendBits(cmd, 8);
  HT162x_SendBits(1, 1);
  digitalWrite(CS, HIGH);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void HT162x_WriteData(uint8_t addr, uint16_t sdata, uint8_t bits = 4)
{
  digitalWrite(CS, LOW);
  HT162x_SendBits(0b101, 3);
  HT162x_SendBits(addr, 6);
  HT162x_SendBits(sdata, bits, MSB_FORMAT);
  digitalWrite(CS, HIGH);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void TestSegments(uint8_t addr)
{
    const int SegDelay = 250;
    
    // Test cycle (single uint16_t writes)
    for (uint8_t i = 0; i < 16; i++) {
      HT162x_WriteData(addr, 1<<i, 16);
      delay(SegDelay);
    }
    HT162x_WriteData(addr, 0, 16);
    delay(SegDelay);
    
    // Test cycle (sequential nibbles)
    for (uint8_t j = 0; j < 4; j++) {
      for (uint8_t i = 0; i < 4; i++) {
        HT162x_WriteData(addr+j, 1<<i, 4);
        delay(SegDelay);
      }
      HT162x_WriteData(addr, 0, 16);
    }
    HT162x_WriteData(addr, 0, 16);
    delay(SegDelay);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void TestChars()
{
  const int CharDelay = 50;

  int aSize = ARRAY_SIZE(SegCharData);
  printf("Asize = %d\n", aSize);
  int charData = 0;
  for (int n = 0; n < 10000; n++)
  {
    for (uint8_t pos = 0; pos < NUM_DIGITS; pos++)
    {
      HT162x_WriteData(digitAddr[pos], SegCharData[charData], 16);
      delay(CharDelay);
      charData = (charData < aSize-1 ? charData+1 : 0);
    }
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void RandomSegments(uint16_t cycles)
{
    for (int j = 0; j < cycles; j++)
    {
      for (int n = 0; n < NUM_DIGITS; n++)
      {
        uint16_t sdata = random(0x10);
        uint16_t ndigit = random(NUM_DIGITS);
        uint8_t addr = digitAddr[n] + random(4);
        HT162x_WriteData(addr, sdata);
      }
      delay(1);
    }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
union QuadUnion {
  uint32_t x32;
  struct {
    uint16_t lo;
    uint16_t hi;
  } x16;
  struct {
    uint8_t lo0;
    uint8_t hi0;
    uint8_t lo1;
    uint8_t hi1;
  } x8;
};

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//#############################################################################
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void setup()
{
  Serial.begin(115200);

  // Set up stdout for printf() use
  fdev_setup_stream(&serial_stdout, serial_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &serial_stdout;
  //printf("My favorite number is %6d!\n", 42);

  QuadUnion foo;
  foo.x32 = 0xaa55ff00; //0b10101010010101011111111100000000; //0xaa55ff00;
  printf("Foo = 0x%08x\n", foo.x32);
  printf("Foo = 0x%04x,0x%04x\n", foo.x16.lo, foo.x16.hi);
  printf("Foo = 0x%02x,0x%02x,0x%02x,0x%02x\n", foo.x8.lo0, foo.x8.hi0, foo.x8.lo1, foo.x8.hi1);

  // Set up I/O and display
  pinMode(CS, OUTPUT);
  pinMode(WR, OUTPUT);
  pinMode(DATA, OUTPUT);
  delay(50);

  HT162x_Command(CMD_SYS_EN);
  HT162x_Command(CMD_RC_INT);
  //HT162x_Command(CMD_BIAS_COM); // Only for HT1621
  HT162x_Command(CMD_LCD_OFF);
  for (uint8_t addr = 0x00; addr < 0x3F; addr++)
  {
    HT162x_WriteData(addr, 0xff);
  }

  HT162x_Command(CMD_LCD_ON); // Should turn it back on

  delay(100);
  for (uint8_t addr = 0x00; addr < 0x3F; addr++)
  {
    HT162x_WriteData(addr, 0x00);
  }
  delay(500);
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//#############################################################################
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void loop()
{
  while (1)
  {
//    for (uint8_t pos = 0; pos < NUM_DIGITS; pos++)
//    {
//      TestSegments(digitAddr[pos]);
//    }

    RandomSegments(10000);

//    TestChars();

    delay(1000);
  }
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

