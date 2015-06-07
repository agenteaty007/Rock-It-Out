UCR EE/CS 120B Spring 2015
Rock It Out - Musical Game
Alberto Tam Yong

Files
---------------------------------
Main Source Code: \atamy001_RockItOut_main\atamy001_RockItOut_main\atamy001_RockItOut_main.c

Libraries: \atamy001_RockItOut_main\atamy001_RockItOut_main\lib
Included library details:
bit.h contains SetBit and GetBit functions to ease bit manipulation
io.c and io.h contain the functions to manipulate the LCD display
timer.h contains functions for the timerISR
usart.h contains the functions to ease USART communication

Under \other_files, you'll find some reports, Arduino code, and reference materials.

High Level Description of Project
---------------------------------
The embedded system features the ATmega1284 that runs the game, menus, and interfaces all devices together. The LCD display shows information and text for the menus and game. The shift registers control the LED Matrix and provide players intuitive feedback, such as what notes to play and their score in a bargraph form. The Arduino UNO uses a microphone to pick up the sounds and analyzes them through some basic algorithms detecting peaks and nodes; then, it sends out a 2-byte package with the frequency obtained to the ATmega1284.

User Guide
---------------------------------
Button labeling (from left to right): Up, Down, Back, Select
During startup, use the Select button to enter the menu
Navigate through the menu using Up and Down buttons
Select your options using the Select button
For a soft restart in the middle of a game, use the Select button to end the game

Special considerations:
While singing or playing music at the microphone, do not go further than 3 feet away
Sustaining a note or repeating it will increase the accuracy of the reading from the microphone

Technologies and Components
---------------------------------
1 X ATmega1284
1 X LCD Display
2 X Shift Registers
3 X Large Breadboards (830-pin)
1 X LED Matrix - Dual Color
1 X Arduino UNO
1 X Microphone
Programmer: 1 X OLIMEX AVR Programmer, and 1 X ICSP Adapter Board
Power: 1 X 5V Power Supply Board, and 1 X 6V 1A Power Adapter
Assorted components: Jumpers, 8 X 330 ohm Resistors, 1 X 10K ohm Potentiometer, 4 X Tactile Buttons
Software: AVR Studio 6.2 and Arduino IDE 1.6.3

Link to Demo Video
---------------------------------
https://youtu.be/1uPxHBodMcQ