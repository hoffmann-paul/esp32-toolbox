# ESP32 Toolbox
## How to set up
### Components
- 1x ESP32
- 1x LiquidCrystal LCD Display
- 6x PullUp Buttons
- _1x Breadboard Powerrail_
- _3x Jumper Cabel f-f_
- _14x Jumper Cabel m-f_


_The Components in Italic Style only needed for wiring_
### Wiring
| ESP32 | Component |
| ----- | ----- |
| VIN | VCC (LCD-Display) |
| D21 | SDA (LCD-Display) |
| D22 | SCL (LCD-Display) |
| D13 | Menu Button |
| D12 | OK Button |
| D14 | Right Button |
| D27 | Left Button |
| D26 | Up Button |
| D25 | Down Button |

### Code
Import the sketch.ino file in the Arudino IDE and upload it to the esp32
