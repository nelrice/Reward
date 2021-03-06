/*
  Behavior Control with blinking LED for video synchronization
 
 General template:
 Collects input from digital inputs (e.g., beam breaks over food wells),
 and controls digital outputs (e.g., syringe pumps or valves) to provide
 reward as appropriate.
 
 This file:
 Runs the logic for a linear (i.e., back and forth) maze.
 Syringe pumps are active LOW
 
 by Caleb Kemere
 modified by Nicola Park, 2013
 modified by Etienne Ackermann, 01/21/2014 to fix bugs (startup state was previously undefined), and to add web server (subsequently removed)
 modified by Etienne Ackermann, 08/12/2016 to add LED sync
 modified by Etienne Ackermann, 09/15/2016 to change from digitalread to analogread for interfacing with SpikeGadgets ECU
 */

// Arduino input pins corresponding to food well beam breaks
const int wellA = A2;
const int wellB = A3;
const int buttonPin = 2;    // the number of the pushbutton pin

// Arduino output pins corresponding to syringe pump output controls
const int pumpA = 6;
const int pumpB = 7;
const int ledPin = 13;      // IR sync LED
const int ledMonPin = 12;   // toggled with IR sync LED, used as DIO to Trodes
const int ledRedPin = 11;   // visible LED toggled at start and end of sync, for visual feedback to user

const int dispenseLength = 500; // duration (in ms) that pump remains active per reward (1000 for 10 rpm; 500 for 20 rpm)
const long blinkInterval = 5000; // duration (in ms) between LED blinks
const long blinkLength = 30;     // duration (in ms) of LED blink

int ledState = LOW;
int syncState = true; // don't blink-n-sync until button is pressed
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// State information for behvaioral logic
int lastWell = 0; // during operation will alternate between 1 (=A) and 2 (=B)

// State information for output control
int dispense = -1; // signals to output service to activate a syringe pump
//  if nonzero, corresponds to output pin 
unsigned long int dispenseTime = 0; // will be the time dispensing should end
unsigned long previousMillis = 0;   // will store last time LED was updated

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// Reward history for webserver output
//int wellACount = 0;
//int wellBCount = 0;

void setup() {

  // initalize the input and output pins:
  pinMode(wellA, INPUT);
  pinMode(wellB, INPUT);

  pinMode(pumpA, OUTPUT);
  pinMode(pumpB, OUTPUT);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ledMonPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);

  digitalWrite(pumpA, HIGH); // initially turn syringe pumps off
  digitalWrite(pumpB, HIGH); //   (they're active LOW)
  digitalWrite(ledPin, LOW); 
  digitalWrite(ledMonPin, LOW); 
  digitalWrite(ledRedPin, LOW); 

  // give the sensor and Ethernet shield time to set up:
  delay(1000);  // 1000 ms delay
}

void loop() {
  unsigned long currentMillis = millis();

  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = currentMillis;
  }

  if ((currentMillis - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the sync state if the new button state is HIGH
      if (buttonState == HIGH) {
        syncState = !syncState;
        if (syncState == true) {
          // light up red LED to indicate that syncState is not true
          digitalWrite(ledRedPin, HIGH); 
          delay(2000);
          digitalWrite(ledRedPin, LOW); 
        } else {
          // flash red LED to indicate that syncState is not true
          digitalWrite(ledRedPin, HIGH); 
          delay(500);
          digitalWrite(ledRedPin, LOW); 
          delay(500);
          digitalWrite(ledRedPin, HIGH); 
          delay(500);
          digitalWrite(ledRedPin, LOW); 
        }
      }
    }
  }

  lastButtonState = reading;

  if (currentMillis - previousMillis >= blinkInterval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    ledState = LOW;

    if (syncState == false){
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
    digitalWrite(ledMonPin, ledState);

  } else if (currentMillis - previousMillis >= (blinkInterval - blinkLength)) {
    ledState = HIGH;

    if (syncState == false){
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
    digitalWrite(ledMonPin, ledState);
  }
  

  // Check whether foodwells have been triggered, and reward if necessary
  if ((analogRead(wellA) > 200) && (lastWell != 1)) {
    digitalWrite(pumpB,HIGH);  // as a safety, make sure that pump B is deactivated
    digitalWrite(pumpA,LOW);
    dispenseTime = currentMillis + dispenseLength;
    //wellACount++;
    lastWell = 1;
  }
  if ((analogRead(wellB) > 200) && (lastWell != 2)) {
    digitalWrite(pumpA,HIGH);  // as a safety, make sure that pump A is deactivated
    digitalWrite(pumpB,LOW);
    dispenseTime = currentMillis + dispenseLength;
    //wellBCount++;
    lastWell = 2;
  }

  doDispense();
}

void doDispense() {
  if ((millis() > dispenseTime) && (dispenseTime != 0)) { // done dispensing
    digitalWrite(pumpA,HIGH); // (these are active low outputs)
    digitalWrite(pumpB,HIGH); // (these are active low outputs)
    dispenseTime = 0;
  }
}
 
