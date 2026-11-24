
// Quick Pin Reference

// Pin A0: Mechanical 8-button resistor ladder
// Pin D3: IR Signal out (via 1N4148 diode, stripe toward Arduino) to TV IR OUT
// Pins D4 & D5: Volume rotary encoder (CLK & DT)
// Pins D6 & D7: Program/Channel rotary encoder (CLK & DT)
// Pin D8: Menu button (connects to GND when pressed)


#include <Arduino.h>

#define DISABLE_CODE_FOR_RECEIVER
#define USE_NO_SEND_PWM           // Direct-to-board active-low mode (remove if using an IR LED)
#define IR_SEND_PIN 3
#include <IRremote.hpp>

// --- Analog Mechanical Buttons ---
const int buttonPin = A0;
int buttonA = 0;
int buttonAprev = 0;

// --- Rotary Encoder 1: Channels ---
const int chDtPin  = 4;
const int chClkPin = 5;
int chLastClk = HIGH;

// --- Rotary Encoder 2: Program / Volume ---
const int volDtPin  = 7;
const int volClkPin = 8;
int volLastClk = HIGH;


// --- Menu Button (Standalone button or Encoder SW pin) ---
const int menuBtnPin = 2;
int menuLastState = HIGH;

// --- AV Button (Standalone button or Encoder SW pin) ---
const int avBtnPin = 6;
int avLastState = HIGH;

// --- Captured IR Codes: Mechanical Buttons 1 - 8 ---
const uint32_t tRawData[8][2] = {
    {0x7DC611C, 0x3F8},   // Button 1 (~37-55)
    {0xBDC611C, 0x3F4},   // Button 2 (~201)
    {0xFDC611C, 0x3F0},   // Button 3 (~299)
    {0x13DC611C, 0x3EC},  // Button 4 (~382-403)
    {0x17DC611C, 0x3E8},  // Button 5 (~495-509)
    {0x1BDC611C, 0x3E4},  // Button 6 (~597-616)
    {0x1FDC611C, 0x3E0},  // Button 7 (~672-702)
    {0x23DC611C, 0x3DC}   // Button 8 (~780-827)
};

// --- Captured IR Codes: Encoders & Menu ---
const uint32_t tRawVolUp   [2] = {0x43DC611C, 0x3BC}; // Volume Up
const uint32_t tRawVolDown [2] = {0x53DC611C, 0x3AC}; // Volume Down
const uint32_t tRawChUp    [2] = {0x4FDC611C, 0x3B0}; // Program Up
const uint32_t tRawChDown  [2] = {0x5FDC611C, 0x3A0}; // Program Down
const uint32_t tRawMenu    [2] = {0x3BDC611C, 0x3C4}; // Menu
const uint32_t tRawAV      [2] = {0x2FDC611C, 0x3D0}; // AV

/* --- Additional Reference Commands ---
   AV:    uint32_t tRawAV[2]    = {0x2FDC611C, 0x3D0};
   Power: uint32_t tRawPower[2] = {0x4BDC611C, 0x3B4};
*/

// Helper function to transmit any 42-bit raw code array
void sendTVCode(const uint32_t* codeArray) {
    IrSender.sendPulseDistanceWidthFromArray(
        38, 8950, 4500, 550, 1700, 550, 550,
        codeArray, 42, PROTOCOL_IS_LSB_FIRST, 0, 0
    );
}

int getButtonFromRaw(int raw) {
    if (raw >= 740) return 8;      // Button 8 (~780-827)
    if (raw >= 635) return 7;      // Button 7 (~672-702)
    if (raw >= 550) return 6;      // Button 6 (~597-616)
    if (raw >= 450) return 5;      // Button 5 (~495-509)
    if (raw >= 350) return 4;      // Button 4 (~382-403)
    if (raw >= 250) return 3;      // Button 3 (~295-299)
    if (raw >= 150) return 2;      // Button 2 (~201-202)
    if (raw >= 20)  return 1;      // Button 1 (~37-55)
    return 0;                      // Idle / Nothing pressed (~0-6)
}

void setup() {
//    Serial.begin(9600);
    IrSender.begin(DISABLE_LED_FEEDBACK);

    pinMode(volClkPin, INPUT_PULLUP);
    pinMode(volDtPin, INPUT_PULLUP);
    pinMode(chClkPin, INPUT_PULLUP);
    pinMode(chDtPin, INPUT_PULLUP);
    pinMode(menuBtnPin, INPUT_PULLUP);
    pinMode(avBtnPin, INPUT_PULLUP);

    volLastClk = digitalRead(volClkPin);
    chLastClk  = digitalRead(chClkPin);

//    Serial.println("Starting Mechanical Buttons + Dual Encoders for Adrijan TV");
}

void loop() {
    // 1. CHECK VOLUME ENCODER
    int volClk = digitalRead(volClkPin);
    if (volClk != volLastClk && volClk == LOW) {
        if (digitalRead(volDtPin) != volClk) {
//            Serial.println("Volume UP");
            sendTVCode(tRawVolUp);
        } else {
//            Serial.println("Volume DOWN");
            sendTVCode(tRawVolDown);
        }
    }
    volLastClk = volClk;

    // 2. CHECK PROGRAM / CHANNEL ENCODER
    int chClk = digitalRead(chClkPin);
    if (chClk != chLastClk && chClk == LOW) {
        if (digitalRead(chDtPin) != chClk) {
//            Serial.println("Program UP");
            sendTVCode(tRawChUp);
        } else {
//            Serial.println("Program DOWN");
            sendTVCode(tRawChDown);
        }
    }
    chLastClk = chClk;

    // 3. CHECK MENU BUTTON
    int menuState = digitalRead(menuBtnPin);
    if (menuState == LOW && menuLastState == HIGH) {
        delay(25);
        if (digitalRead(menuBtnPin) == LOW) {
//            Serial.println("Menu Pressed");
            sendTVCode(tRawMenu);
        }
    }
    menuLastState = menuState;

    
    // 4. CHECK AV BUTTON
    int avState = digitalRead(avBtnPin);
    if (avState == LOW && avLastState == HIGH) {
        delay(25);
        if (digitalRead(avBtnPin) == LOW) {
//            Serial.println("AV Pressed");
            sendTVCode(tRawAV);
        }
    }
    menuLastState = menuState;

    // 5. CHECK MECHANICAL CHANNEL BUTTONS (A0)
    int firstRead = getButtonFromRaw(analogRead(buttonPin));
    if (firstRead != 0 && firstRead != buttonAprev) {
        delay(60); // Mechanical switch debounce
        int rawSettled = analogRead(buttonPin);
        buttonA = getButtonFromRaw(rawSettled);

        if (buttonA != 0 && buttonA != buttonAprev) {
            buttonAprev = buttonA;

  //          Serial.print("Latched Button: ");
  //          Serial.print(buttonA);
  //          Serial.print(" | Raw: ");
  //          Serial.println(rawSettled);

            sendTVCode(tRawData[buttonA - 1]);
        }
    }

    delay(1);
}
