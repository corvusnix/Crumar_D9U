////////////////////////////////////////////////////////////////////
// Crumar Drawbar Controller D9U
// by Guido Scognamiglio
// Runs on Atmel ATmega32U4 Arduino Leonardo (with MIDI USB Library)
// Reads 9 analog inputs from internal ADCs
// Sends MIDI CC numbers 12-20 or 21-29 according to selected mode
// Last update: February 2020
//

#define DRAWBAR_COUNT 9

////////////////////////////////////////////////////////////////////
// This is where you can define your CC numbers for the Bank 0 or 1
uint8_t CCMap[2][DRAWBAR_COUNT] =
{
  { 12, 13, 14, 15, 16, 17, 18, 19, 20 }, // Upper drawbars
  { 21, 22, 23, 24, 25, 26, 27, 28, 29 }  // Lower drawbars
};

////////////////////////////////////////////////////////////////////
// You should not modify anything else below this line
// unless you know what you're doing.
////////////////////////////////////////////////////////////////////

// Define I/O pins
#define LED_RED     15
#define LED_GREEN   16
#define BUTTON      5

// Define global modes
#define DEBOUNCE_TIME 150
#define DEADBAND    8

// Include libraries
#include <EEPROM.h>
#include <MIDIUSB.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// Make sure mode is either 0 or 1
bool mode;

// ADC reference map
int ADCmap[DRAWBAR_COUNT] = { A0, A1, A2, A3, A6, A7, A8, A9, A10 };

// Called then the pushbutton is depressed
void set_leds()
{
  digitalWrite(LED_RED, !mode);
  digitalWrite(LED_GREEN, mode);
}

// Called to generate the MIDI CC message
void SendMidiCC(uint8_t num, uint8_t value)
{
  midiEventPacket_t CC = {0x0B, 0xB0 | mode, num, value};
  MidiUSB.sendMIDI(CC);
  MidiUSB.flush();

  // Midi lib wants channels 1~16
  MIDI.sendControlChange(num, value, mode + 1);
}

// Called to check whether a drawbar has been moved
void checkDrawbar() {  
  // Read analog inputs (do the round robin)
  static uint8_t drawbar = 0;
  int value = analogRead(ADCmap[drawbar]);
  if (++drawbar >= DRAWBAR_COUNT) drawbar = 0;

  // Get difference from current and previous value and
  // Exit this function if the new value is within the deadband
  static int prev_val[DRAWBAR_COUNT] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
  if (abs(value - prev_val[drawbar]) <= DEADBAND) return;

  // Store new value
  prev_val[drawbar] = value;

  // Send Midi with the 7 bit value
  SendMidiCC(CCMap[mode][drawbar], value >> 3);
}

void checkButton() {
  static uint8_t debounce_timer = DEBOUNCE_TIME;
  // Read Button
  if (digitalRead(BUTTON) == LOW) {
    if (debounce_timer > 0) {
      --debounce_timer;
      if (debounce_timer == 1) {
        mode = !mode; // Reverse
        set_leds(); // and set LEDs
        EEPROM.update(0x01, mode); //save mode for reboot
      }
    }
  } else {
    debounce_timer = DEBOUNCE_TIME;
  }
}

// The setup routine runs once when you press reset:
void setup()
{
  // Initialize serial MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Set up digital I/Os
  pinMode(BUTTON, INPUT_PULLUP); // Button
  pinMode(LED_RED, OUTPUT);      // Led 1
  pinMode(LED_GREEN, OUTPUT);    // Led 2

  // Recall mode from memory and set LEDs
  mode = EEPROM.read(0x01);
  set_leds();
}

// The loop routine runs over and over again forever:
void loop()
{
  // Check next Drawbar for changed position
  checkDrawbar();
  // Check if Button is pressed
  checkButton();
}
