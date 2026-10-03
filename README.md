# Penalty Shootout – ESP32 CYD Football Game ⚽

A touchscreen penalty shootout game for the ESP32 CYD. Take five penalty kicks, aim inside the goal, and try to score past the moving goalkeeper.

## Features

- 320×240 ILI9341 football field graphics
- XPT2046 touchscreen controls
- Five-shot penalty shootout format
- Moving goalkeeper with diving animation
- Goal and miss results
- Score HUD and game-over screen
- Buzzer sound effects and title music

## Hardware

- ESP32-2432S028 / ESP32 CYD (Cheap Yellow Display)
- 2.8-inch 320×240 ILI9341 display
- XPT2046 resistive touchscreen
- On-board buzzer

## Arduino setup

Open `penalty_shootout.ino` in the Arduino IDE and select **ESP32 Dev Module** (or the matching CYD board profile).

Required libraries:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit ILI9341](https://github.com/adafruit/Adafruit_ILI9341)
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)

`SPI` and `math.h` are provided by the ESP32 Arduino environment.

## Display used for the project

👉 [ESP32 touchscreen item link](https://temu.to/k/ea65oy4oun8)

Disclosure: this link may be an affiliate link. It does not change the price for you.

## Video

🎬 [Watch Penalty Shootout on YouTube](https://youtube.com/shorts/kxJehX_gJHs)

## License

This project is released under the MIT License. See [LICENSE](LICENSE).
