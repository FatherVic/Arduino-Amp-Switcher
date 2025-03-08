// 2024 by L S. Olsen
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>  // Library for watchdog timer to reset Arduino

// Define pin numbers
const int buttonPin4 = 1;  // Button for turning off all connections
const int buttonPin1 = 2;  // Button for cycling relays 4-8 (Amps) Pin 2
const int buttonPin2 = 3;  // Button for toggling relays 9 and 10 (FX Chain) Pin 3
const int buttonPin3 = 13; // Button for toggling relays 11 and 12 (FX Loop from AMP) Pin 13

const int relayPins[] = {4, 5, 6, 7, 8}; // Relays for cycling Amps (pins 4, 5, 6, 7, 8)
const int relay9 = 9;    // Relay 9 (pin 9) - FX Chain In
const int relay10 = 10;  // Relay 10 (pin 10) - FX Chain Out
const int relay11 = 11;  // Relay 11 (pin 11) - Katana Loop Send
const int relay12 = 12;  // Relay 12 (pin 12) - Katana Loop Return

int currentRelay = 0;    // Start with the first relay
bool lastButtonState1 = HIGH; // Previous button state for button 1
bool lastButtonState2 = HIGH; // Previous button state for button 2
bool lastButtonState3 = HIGH; // Previous button state for button 3
bool lastButtonState4 = HIGH; // Previous button state for button 4

// Variables to track button press durations
unsigned long button2PressTime = 0;
unsigned long button3PressTime = 0;

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD address 0x27, 16 chars, 2 lines

// Define string variables for each amp name in order.
String relayNames[] = {
    "Katana MkII Head",
    "Line6 Spider III",
    "Fender Sidekick",
    "Bugera Vintage 5",
    "Other Amp"
};

// Define string variables for line 2 messages
const String FX_CHAIN = "FX Chain";
const String KATANA_LOOP = "Katana Loop";
const String FX_BYPASS = "FX Bypass";

void setup() {
    // Initialize pin modes - use pullup resistors for the debounce
    pinMode(buttonPin1, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);
    pinMode(buttonPin3, INPUT_PULLUP);
    pinMode(buttonPin4, INPUT_PULLUP);

    for (int i = 0; i < 5; i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], LOW);
    }

    pinMode(relay9, OUTPUT);
    pinMode(relay10, OUTPUT);
    pinMode(relay11, OUTPUT);
    pinMode(relay12, OUTPUT);

    digitalWrite(relay9, LOW);
    digitalWrite(relay10, LOW);
    digitalWrite(relay11, LOW);
    digitalWrite(relay12, LOW);

    lcd.init();
    lcd.backlight();

    // Initial text
    updateLCD(centerText("No Amp"), centerText("FX Bypass"));

    Serial.begin(9600);
}

void loop() {
    bool buttonState1 = digitalRead(buttonPin1);
    bool buttonState2 = digitalRead(buttonPin2);
    bool buttonState3 = digitalRead(buttonPin3);
    bool buttonState4 = digitalRead(buttonPin4);

    // Check if button 1 is pressed
    if (buttonState1 == LOW && lastButtonState1 == HIGH) {
        // Set all relay pins to LOW first
        for (int i = 0; i < 5; i++) {
            digitalWrite(relayPins[i], LOW);
        }

        // Set the current relay pin to HIGH
        digitalWrite(relayPins[currentRelay], HIGH);

        // Print relay number and LCD display name to Serial
        Serial.print("Relay Pin: ");
        Serial.print(relayPins[currentRelay]);
        Serial.print(" | Display: ");
        Serial.println(relayNames[currentRelay]);

        // Update the LCD with the current relay name before incrementing
        updateLCD(centerText(relayNames[currentRelay]), centerText(determineLine2()));

        // Move to the next relay (after updating the LCD)
        currentRelay = (currentRelay + 1) % 5;

        delay(500); // Debounce delay
    }

    // Engage or bypass the FX Chain when button 2 is pressed (no change to displayName)
    if (buttonState2 == LOW && lastButtonState2 == HIGH) {
        int relay9State = digitalRead(relay9);
        int relay10State = digitalRead(relay10);
        digitalWrite(relay9, !relay9State);
        digitalWrite(relay10, !relay10State);
        digitalWrite(relay11, LOW);
        digitalWrite(relay12, LOW);

        // Update only line 2 without changing displayName (line 1)
        updateLCD(centerText(relayNames[(currentRelay - 1 + 5) % 5]), centerText(determineLine2()));
        
        delay(500); // Debounce delay
    }

    // Engage the Katana send/return with the FX Loop. Engage the Katana
    if (buttonState3 == LOW && lastButtonState3 == HIGH) {
        // Set relayPins 1-4 to LOW
        for (int i = 1; i < 5; i++) {
            digitalWrite(relayPins[i], LOW);
        }

        // Set relayPin 0 to HIGH
        digitalWrite(relayPins[0], HIGH);

        int relay11State = digitalRead(relay11);
        int relay12State = digitalRead(relay12);
        digitalWrite(relay11, !relay11State);
        digitalWrite(relay12, !relay12State);
        digitalWrite(relay9, LOW);
        digitalWrite(relay10, LOW);

        // Update the LCD with the first relay name and line 2 status
        updateLCD(centerText(relayNames[0]), centerText(determineLine2()));
        delay(500); // Debounce delay
    }

    // Check if button 4 (reset button) is pressed to reset relays to the startup state
    if (buttonState4 == LOW && lastButtonState4 == HIGH) {
        // Set all relays to LOW (default startup state)
        for (int i = 0; i < 5; i++) {
            digitalWrite(relayPins[i], LOW);
        }
        digitalWrite(relay9, LOW);
        digitalWrite(relay10, LOW);
        digitalWrite(relay11, LOW);
        digitalWrite(relay12, LOW);

        // Optionally, update the LCD to show the reset state
        updateLCD(centerText("No Amp"), centerText("FX Bypass"));

        // Provide feedback in Serial Monitor
        Serial.println("All relays reset to default startup state");

        delay(500);  // Debounce delay
    }

    // Check for button 2 long press (more than 5 seconds)
    if (buttonState2 == LOW) {
        if (button2PressTime == 0) button2PressTime = millis(); // Start counting time

        // If held for more than 5 seconds, reset Arduino
        if (millis() - button2PressTime > 5000) {
            wdt_enable(WDTO_15MS); // Trigger watchdog reset
            while (1); // Infinite loop to force reset
        }
    } else {
        button2PressTime = 0; // Reset time if button is released
    }

    // Check for button 3 long press (more than 5 seconds)
    if (buttonState3 == LOW) {
        if (button3PressTime == 0) button3PressTime = millis(); // Start counting time

        // If held for more than 5 seconds, update LCD display
        if (millis() - button3PressTime > 5000) {
            updateLCD(centerText("Amp Switcher"), centerText("2024 L S Olsen"));
        }
    } else {
        button3PressTime = 0; // Reset time if button is released
    }

    lastButtonState1 = buttonState1;
    lastButtonState2 = buttonState2;
    lastButtonState3 = buttonState3;
    lastButtonState4 = buttonState4;

    delay(10);
}

// Set line 1 - which amp are we?
String lcdLine1() {
    if (currentRelay >= 0 && currentRelay < sizeof(relayNames) / sizeof(relayNames[0])) {
        return relayNames[currentRelay];
    } else {
        return "ERROR";
    }
}

// Set line 2 - FX Chain, Katana Loop or Bypass?
String determineLine2() {
    int relay9State = digitalRead(relay9);
    int relay10State = digitalRead(relay10);
    int relay11State = digitalRead(relay11);
    int relay12State = digitalRead(relay12);

    // All are LOW
    if (relay9State == LOW && relay10State == LOW && relay11State == LOW && relay12State == LOW) {
        return FX_BYPASS;
    }

    // FX chain is engaged
    if (relay9State == HIGH && relay10State == HIGH && relay11State == LOW && relay12State == LOW) {
        return FX_CHAIN;
    }

    // FX chain is engaged
    if (relay9State == LOW && relay10State == LOW && relay11State == HIGH && relay12State == HIGH) {
        return KATANA_LOOP;
    }
    
    // Default display state
    return FX_BYPASS;
}

// Center text by adding leading/trailing spaces
String centerText(String text) {
    int spaces = (16 - text.length()) / 2; // Calculate leading spaces
    String centeredText = "";
    for (int i = 0; i < spaces; i++) {
        centeredText += " "; // Add leading spaces
    }
    centeredText += text; // Add the text itself
    return centeredText;
}

// Helper function to update LCD lines with provided strings
void updateLCD(String line1, String line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}
