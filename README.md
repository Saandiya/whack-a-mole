# Whack-a-Mole
Whack-A-Mole Project for ECE362

Project Description: 
1. Ability to display current score on 7-segment display using SPI and save top 3 high scores on OLED display using I2C EEPROM.
2. Ability to produce interrupts from pushbutton presses to indicate a 'whack' by the user and end the game when the countdown finishes.
3. Ability to select difficulty levels from A to D on the keypad using GPIO with each level having increasing clock frequency on 16 x 16 LED Matrix using Timer.
4. Ability to produce sound effects such as "game over", "you won", and "whack" stored on an SD card and played using DAC when triggered by an interrupt.


Main Features: Parts that will be used:
1.⁠ ⁠STM32F091 Microcontroller
2.⁠ 16x16 ⁠LED Matrix (https://nam04.safelinks.protection.outlook.com/?url=https%3A%2F%2Fwww.amazon.com%2Fdp%2FB088BTYJH6%2Fref%3Dsspa_mw_detail_4%3Fie%3DUTF8%26psc%3D1%26sp_csd%3Dd2lkZ2V0TmFtZT1zcF9waG9uZV9kZXRhaWw&data=05%7C02%7Cmohan76%40purdue.edu%7C76741d5543cd45fc1a6c08dce4867d05%7C4130bd397c53419cb1e58758d6d63f21%7C0%7C0%7C638636513488787692%7CUnknown%7CTWFpbGZsb3d8eyJWIjoiMC4wLjAwMDAiLCJQIjoiV2luMzIiLCJBTiI6Ik1haWwiLCJXVCI6Mn0%3D%7C0%7C%7C%7C&sdata=Gk7Wl3J5Zlae7T%2BLXg8B0c%2BCCLv7K6yFqSKAe7VROAk%3D&reserved=0)
3.⁠ ⁠Keypad
4.⁠ ⁠SD Card (to add sound effects)
5.⁠ ⁠OLED Display and 7-Segment Display
6.⁠ ⁠Resistors and Capacitors (for basic circuit connections)
7.⁠ ⁠Breadboard and Jumper Wires
8. Speakers
9. TRRS Jack

Explanation:
The GPIO is used to connect the LED Matrix and the keypad to the microcontroller. The keypad will be used to register the whacks by the user (4x4 game setup) and the LED matrix will be used to display the game. The keypad will also accept the username from the user at the start of the game and the user will start the game by selecting the difficulty level by pressing the A-D buttons on the keypad. The username will be displayed along with the high scores on the OLED if the user beats previous records. Each game lasts for 60 seconds and on each correct whack, the player receives points. Timers can also be used to introduce delays between events, like randomly delaying the LED lighting up to make the game unpredictable. The 7-segment display will also show the countdown to the end of the game. The DAC will be used to play sound effects such as "game over", "You win", and "whack" (if the whack hits a mole) when triggered by an interrupt. The sounds will be played on the speaker which will be connected using the TRRS audio jack.

External Interfaces: 
1.⁠ ⁠GPIO
2.⁠ ⁠SPI
3.⁠ ⁠I2C

Internal Peripherals: 
1.⁠ ⁠GPIO
2.⁠ ⁠Timers
3.⁠ ⁠SPI
4.⁠ ⁠DAC

YouTube: https://youtube.com/shorts/n-H49iqz39U?feature=share
