# Driving HT1622/TM1622 LCD displays with Arduino

## Datasheets

* [HT1621](https://www.holtek.com/documents/10179/116711/HT1621v321.pdf)
* [HT1622](https://www.holtek.com/documents/10179/116711/HT1622v270.pdf)
* [TM1622](https://datasheet.lcsc.com/lcsc/2009211536_TM-Shenzhen-Titan-Micro-Elec-TM1622_C92286.pdf)

---

## 16-segment displays - typical names for segments

```
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
```

---

## Board example: DM8BA10 10-character 16-segment alphanumeric LCD

LCD = 10 digit 16 seg + 9 dp + continuous backlight

Status: Active

Manufacturer: eletechsup, Canton Electronics

Wiring:

| Pin  | Details                                       |
|------|-----------------------------------------------|
| LED+ | 3.3V or 5V for backlight via 100 ohm resistor |
| VDD  | 5VDC (required)                               |
| GND  | Ground                                        |
| DATA | 5V logic                                      |
| RW   | 5V logic                                      |
| CS   | 5V logic                                      |

Addressing (each address hold 4 bits):

| Addresses  | Element   |
|------------|-----------|
| 0x00-0x03  | Digit 0   |
| 0x00-0x03  | Digit 0   |
| 0x04-0x07  | Digit 2   |
| 0x08-0x0B  | Digit 4   |
| 0x0C-0x0F  | Digit 6   |
| 0x10-0x13  | Digit 8   |
| 0x14-0x17  | Digit 1   |
| 0x18-0x1B  | Digit 3   |
| 0x1C-0x1F  | Digit 5   |
| 0x20       | (unused)  |
| 0x21 bit 3 | Decimal 3 |
| 0x21 bit 2 | Decimal 2 |
| 0x22-0x25  | Digit 7   |
| 0x26       | (unused)  |
| 0x27 bit 3 | Decimal 1 |
| 0x27 bit 2 | Icon 'Hz' |
| 0x28+      | (unused)  |

Segment mapping:

```
  Bit numbering via a little-endian            Bit numbering over
   uint16_t mapped to 4 addresses             sequential addresses

    /-----------\   /-----------\         /-----------\   /-----------\ 
   ||     F     || ||     7     ||       ||    3-3    || ||    1-3    || 
    \-----------/   \-----------/         \-----------/   \-----------/ 
   /-\ /--\      /-\      /--\ /-\       /-\ /--\      /-\      /--\ /-\ 
  |   |\   \    |   |    /   /|   |     |   |\   \    |   |    /   /|   | 
  |   | \   \   |   |   /   / |   |     |   | \   \   |   |   /   / |   | 
  | E |  \ D \  | 4 |  / 5 /  | 6 |     |3-2|  \3-1\  |1-0|  /1-1/  |1-2| 
  |   |   \   \ |   | /   /   |   |     |   |   \   \ |   | /   /   |   | 
  |   |    \   \|   |/   /    |   |     |   |    \   \|   |/   /    |   | 
   \-/      \--/ \-/ \--/      \-/       \-/      \--/ \-/ \--/      \-/ 
    /-----------\   /-----------\         /-----------\   /-----------\ 
   ||     C     || ||     0     ||       ||    3-9    || ||    0-0    || 
    \-----------/   \-----------/         \-----------/   \-----------/ 
   /-\      /--\ /-\ /--\      /-\       /-\      /--\ /-\ /--\      /-\ 
  |   |    /   /|   |\   \    |   |     |   |    /   /|   |\   \    |   | 
  |   |   /   / |   | \   \   |   |     |   |   /   / |   | \   \   |   | 
  | A |  / 9 /  | 8 |  \ 1 \  | 2 |     |2-2|  /2-1/  |2-0|  \0-1\  |0-2| 
  |   | /   /   |   |   \   \ |   |     |   | /   /   |   |   \   \ |   | 
  |   |/   /    |   |    \   \|   |     |   |/   /    |   |    \   \|   | 
   \-/ \--/      \-/      \--/ \-/       \-/ \--/      \-/      \--/ \-/ 
    /-----------\   /-----------\         /-----------\   /-----------\ 
   ||     B     || ||     3     ||       ||    2-3    || ||    0-3    || 
    \-----------/   \-----------/         \-----------/   \-----------/ 
```

![All segments active](/images/DM8BA10_active.png)

![Front (LCD side)](/images/DM8BA10_front.png)

![Rear (component side)](/images/DM8BA10_rear.png)

---

## Board example: ETM8809K2-02 9-character 16-segment alphanumeric LCD

LCD = 9 digit 16 seg + 3 dp + 'HZ' symbol + continuous backlight

Status: Discontinued

Manufacturer: Canton Electronics

Wiring:

| Pin  | Details                                       |
|------|-----------------------------------------------|
| VDD  | 5VDC (3.3V doesn't seem to work)              |
| K    | Ground                                           |
| VLCD | -not used-                                    |
| DATA | 5V logic                                      |
| RW   | 5V logic                                      |
| RD   | -not used-                                    |
| CS   | 5V logic                                      |
| A    | 3.3V or 5V for backlight via 100 ohm resistor |
| K    | GND (same as other one)                       |

Addressing (each address hold 4 bits):

| Addresses  | Element   |
|------------|-----------|
| 0x00-0x03  | Digit 0   |
| 0x00-0x03  | Digit 0   |
| 0x04-0x07  | Digit 2   |
| 0x08-0x0B  | Digit 4   |
| 0x0C-0x0F  | Digit 6   |
| 0x10-0x13  | Digit 8   |
| 0x14-0x17  | Digit 1   |
| 0x18-0x1B  | Digit 3   |
| 0x1C-0x1F  | Digit 5   |
| 0x20       | (unused)  |
| 0x21 bit 3 | Decimal 3 |
| 0x21 bit 2 | Decimal 2 |
| 0x22-0x25  | Digit 7   |
| 0x26       | (unused)  |
| 0x27 bit 3 | Decimal 1 |
| 0x27 bit 2 | Icon 'Hz' |
| 0x28+      | (unused)  |

Segment mapping:

```
  Bit numbering via a little-endian            Bit numbering over
   uint16_t mapped to 4 addresses             sequential addresses

    /-----------\   /-----------\         /-----------\   /-----------\ 
   ||     F     || ||     7     ||       ||    3-3    || ||    1-3    || 
    \-----------/   \-----------/         \-----------/   \-----------/ 
   /-\ /--\      /-\      /--\ /-\       /-\ /--\      /-\      /--\ /-\ 
  |   |\   \    |   |    /   /|   |     |   |\   \    |   |    /   /|   | 
  |   | \   \   |   |   /   / |   |     |   | \   \   |   |   /   / |   | 
  | E |  \ D \  | 4 |  / 5 /  | 6 |     |3-2|  \3-1\  |1-0|  /1-1/  |1-2| 
  |   |   \   \ |   | /   /   |   |     |   |   \   \ |   | /   /   |   | 
  |   |    \   \|   |/   /    |   |     |   |    \   \|   |/   /    |   | 
   \-/      \--/ \-/ \--/      \-/       \-/      \--/ \-/ \--/      \-/ 
    /-----------\   /-----------\         /-----------\   /-----------\ 
   ||     C     || ||     0     ||       ||    3-9    || ||    0-0    || 
    \-----------/   \-----------/         \-----------/   \-----------/ 
   /-\      /--\ /-\ /--\      /-\       /-\      /--\ /-\ /--\      /-\ 
  |   |    /   /|   |\   \    |   |     |   |    /   /|   |\   \    |   | 
  |   |   /   / |   | \   \   |   |     |   |   /   / |   | \   \   |   | 
  | A |  / 9 /  | 8 |  \ 1 \  | 2 |     |2-2|  /2-1/  |2-0|  \0-1\  |0-2| 
  |   | /   /   |   |   \   \ |   |     |   | /   /   |   |   \   \ |   | 
  |   |/   /    |   |    \   \|   |     |   |/   /    |   |    \   \|   | 
   \-/ \--/      \-/      \--/ \-/       \-/ \--/      \-/      \--/ \-/ 
    /-----------\   /-----------\         /-----------\   /-----------\ 
   ||     B     || ||     3     ||       ||    2-3    || ||    0-3    || 
    \-----------/   \-----------/         \-----------/   \-----------/ 
```

![All segments active](/images/ETM8809K2-02_active.png)

![Front (LCD side)](/images/ETM8809K2-02_front.png)

![Rear (component side)](/images/ETM8809K2-02_rear.png)

---
