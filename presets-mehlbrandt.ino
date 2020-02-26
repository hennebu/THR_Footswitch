/* THR Footswitch
 * Arduino program for sending preset data to a Yamaha THR10 guitar amplifier.
 * Copyright (C) 2016 Uwe Brandt, idea and SR sending_patch by Mathis Rosenhauer,
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses

THR Footswitch
- reads the Number of the last used patch from the SD-card file Number.txt
- shows this number on the OLED screen (I2C)
- counts by pushing button up and down
- reads the related patch from the card
- forms and sends MIDI-Data to the THR via USB
- shows the current number on the screen
- shows the title of the patch scrolling on the screen
- sets PIN 5 HIGH to switch the transistor for power the arduino
- monitors the time the arduino is working, after 5 min not pushing a button the arduino goes off (PIN 5).
- pushing both buttons at the same time switches the arduino off
*/

#include <SPI.h>
#include <SD.h>
#include <Usb.h>
#include <usbh_midi.h>
#define PATCH_FILE "THR10.YDL"
#define NUMBER_FILE "NUMBER.TXT"
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>
#define I2C_ADDRESS 0x3C    // OLED address

USB Usb;
USBH_MIDI Midi(&Usb);
SSD1306AsciiAvrI2c oled;
File patchfile;
File numberfile;

const long interval = 300000;        // time until switch off (milliseconds)
const byte button_l_pin = 2;         // Pin 2  Button left
const byte button_r_pin = 3;         // Pin 3 Button right
const byte sdcard_ss_pin = 4;        // SPI-Pin for SD-Card
const byte ledPin =  5;              // the number of the LED pin

unsigned long lastCangeMillis = millis();   // last time a button was pressed
byte buttonState = 0;                    // variable for reading the pushbutton status
byte patchNumber = 1;                    // Patch number
char patchNumberCh[3]; 
byte numberfileSize;
boolean ledState = LOW;                     // 

String stringName = ""; // name of patches
byte patchName[31] = {'L','A','S','T',' ','P','A','T','C','H',' ','+',' ','+',' ','L','A','S','T',' ','P','A','T','C','H',' ','+',' ','+',' ','\0'};  // 25 characters
byte cursorPointer = 6;
byte stringPointer = 0;
// --------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------- setup ---------
void setup() {
  pinMode(button_r_pin, INPUT);
  pinMode(button_l_pin, INPUT); 
  pinMode(ledPin, OUTPUT);
  ledState = HIGH;
  digitalWrite(ledPin, ledState);
    while (digitalRead(button_r_pin) == LOW);
 oled.begin(&Adafruit128x64, I2C_ADDRESS);
 oled.clear(); 


  
      // ------------------------------------------------ USB-host setup
    if (Usb.Init() == -1) {
        Serial.println(F("USB host init failed."));
        while(1); //halt
    }
    delay(200);
    while(Usb.getUsbTaskState() != USB_STATE_RUNNING){
     oled.setFont(System5x7);  
     oled.set2X();
     oled.setCursor(cursorPointer, 2);
     oled.println("search for");
     oled.setCursor(cursorPointer, 6);
     oled.println("  THR1O");
     oled.println();
     if (lastCangeMillis + 30000 < millis()) switch_off(); 
  Usb.Task();
  delay(100); 
}
      //  ---------------------------------------------- SD card setup 
    if (!SD.begin(sdcard_ss_pin)) {
 //   Serial.println(F("SD initialization failed!"));
    return;
  }  
// ---------------------------------------------------- read the char-array
  numberfile = SD.open("NUMBER.txt");
  if (numberfile) {
   numberfileSize = numberfile.size();
   for (int i = 0; i <  numberfileSize +1 ; i++){
     patchNumberCh[i] = numberfile.read();
   }
   numberfile.close();
   } else {
//   Serial.println(F("3. ERROR - file NUMBER.TXT not opened"));  
 }
  // ------------------------------------------------- change the char-array into integer
 patchNumber = 0;
 int mult = 1;
 while (numberfileSize  > 0)
 {
   patchNumber = patchNumber + ((patchNumberCh[numberfileSize-1] - 48) * mult);
   numberfileSize--;  
   mult = mult * 10;
 }
  displayNumber();
}
// -------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------- loop ---------
void loop() {
buttoncheck();
scroll();
if (lastCangeMillis + interval < millis()) switch_off(); 
}
// ------------------------------------------------------------------------- SR -----------------
// ------------------------------------------------------------------------- button check --------
void buttoncheck(){
  if (digitalRead(button_r_pin) == LOW || digitalRead(button_l_pin) == LOW){
     lastCangeMillis = millis();       
  while (digitalRead(button_r_pin) == LOW || digitalRead(button_l_pin) == LOW){
  if (digitalRead(button_r_pin) == LOW) buttonState = 1;
  if (digitalRead(button_l_pin) == LOW) buttonState = -1;
// -------------------------------------------------- checking for shut down   
  if (digitalRead(button_r_pin) == LOW && digitalRead(button_l_pin) == LOW){    
   switch_off();
  }  
 }
 patchNumber = patchNumber + buttonState;
 if ( patchNumber > 99) patchNumber = 99;
 if ( patchNumber < 1) patchNumber = 1;
 displayNumber();
 read_name();
 send_patch(patchNumber); 
 }
}
// ------------------------------------------------------------------------- SR -----------------
// ------------------------------------------------------------------------ scroll 1 step -------
 void scroll(){
   int i;
cursorPointer --;  
 //  Serial.println(cursorPointer);  
if (cursorPointer < 1) {
 
cursorPointer = 6;
if (stringPointer > 29) stringPointer = 0;
stringName = "";
 for (i = stringPointer; i < 30; i++){                    // arrey to string patr 1
 stringName = stringName + char(patchName[i]);
}
 for ( i = 0 ; i < stringPointer; i++){                    // arrey to string patr 2
 stringName = stringName + char(patchName[i]);
 }
 // Serial.println(stringPointer);  
 stringPointer++;
}
 //   Serial.println(stringName);  
 oled.setCursor(cursorPointer, 7);
     oled.setFont(System5x7);  
     oled.set1X();
     oled.println(stringName);
     oled.println();
     delay(10);  
}
// ------------------------------------------------------------------------- SR -----------------
// ------------------------------------------------------------------------- send_patch --------- by Mathis Rosenhauer
void send_patch(uint8_t patch_id)
{
    size_t i, j;
    uint8_t prefix[] = {
        0xf0, 0x43, 0x7d, 0x00, 0x02, 0x0c, 0x44, 0x54,
        0x41, 0x31, 0x41, 0x6c, 0x6c, 0x50, 0x00, 0x00,
        0x7f, 0x7f
    };
    uint8_t cs = 0x71; // prefix checksum

    const size_t patch_size = 0x100;
    size_t offset = 13 + (patch_id - 1) * (patch_size + 5);
    uint8_t chunk[16];
    uint8_t suffix[2];
    File patchfile = SD.open(PATCH_FILE);
    if (patchfile) {
          //Serial.println(" SD open ");
        patchfile.seek(offset);

       Midi.SendSysEx(prefix, sizeof(prefix));
//       Serial.println("prefix sended ");
        for (j = 0; j < patch_size; j += sizeof(chunk)) {
            for (i = 0; i < sizeof(chunk); i++) {
                if (j + i == patch_size - 1) {
                    // Last byte of patch has to be zero.
                    chunk[i] = 0;
                } else {
                    chunk[i] = patchfile.read();
                }
                cs += chunk[i];
            }
            // Split SysEx. THR doesn't seem to care even though
            // USBH_MIDI ends each fragment with a 0x6 or 0x7 Code
            // Index Number (CIN).
            Midi.SendSysEx(chunk, sizeof(chunk));
  //          Serial.println("chunk sended ");
        }
        // Calculate check sum.
        //Serial.println("----> ");
        //Serial.println(cs,HEX);
        suffix[0] = (~cs + 1) & 0x7f;
        //Serial.println(suffix[0],HEX);
        // end SysEx
        suffix[1] = 0xf7;
        Midi.SendSysEx(suffix, sizeof(suffix));
 //       Serial.println("suffix sended ");
        patchfile.close();
    }
}

// ------------------------------------------------------------------------- SR -----------------
// ------------------------------------------------------------------------- displayNumber ------
void displayNumber(){
      oled.clear(); 
      oled.setCursor(40,1);
      oled.setFont(Cooper50); 
      oled.set1X(); 
      oled.println(patchNumber);
}
// ------------------------------------------------------------------------- SR ----------------
// ------------------------------------------------------------------------- read_name ---------
void read_name(){
 const size_t patch_size = 0x100;
 int i;
     for (i = 0; i < 30; i++) {
       patchName[i] = 0 ;
    }
 size_t offset = 13 + (patchNumber - 1) * (patch_size + 5);
  File patchfile = SD.open(PATCH_FILE);
  if (patchfile) {
    patchfile.seek(offset);
    for (i = 0; i < 30; i++) {
      patchName[i] = patchfile.read();
     // if (patchName[i] = 0) patchName[i] = 65;
     //Serial.println(patchName[i]);
    }
    patchName[i] = 0;           // Last byte of patch has to be zero.
    patchfile.close();
  }  
       for (i = 0; i < 30; i++) {
        // Serial.println(patchName[i]);
         if(patchName[i] == 0) patchName[i] = 32;
         // Serial.println(patchName[i]);
         }
}
// ------------------------------------------------------------------------- SR -----------------
// ------------------------------------------------------------------------- switch_off ---------
void switch_off(){
    SD.remove("NUMBER.txt");     
  numberfile = SD.open("NUMBER.txt", FILE_WRITE);
  if (numberfile) {
    numberfile.print(patchNumber);
    numberfile.close();
  } else {
  }
    ledState = LOW;
  digitalWrite(ledPin, ledState);
  delay(2000);
  while(1);
}
