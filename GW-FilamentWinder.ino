/***
 * Project: Filamentwinder (Pastamatic) to rewind filament spools.
 *          In Cooperation with Gregor Trierscheid and Werner Riemann.
 * File   : GW-FilamentWinder.ino
 * Author : Werner Riemann 
 * Created: 03.11.2025
 * Board: Arduino Nano
 * 
 * Description: This is the software part of the filament rewinder.
 *              The main task is to control the side movement after each rotation
 *              of the filament spool. The movement has to be 1.75mm for each
 *              rotation. At the end of a layer, the direction of the rail movement
 *              is inverted and a new layer starts. The rail is driven with a Nema stepper
 *              at 800 micro steps per revolution.
 * 
 * Pins:
 * A0     - Digital IN endswitch left side
 * A1     - Digital IN endswitch right side
 * A2     - 
 * A4,A5  - I2C Display control (A4 - SDA, A5 - SCL) - not used
 * A6     - 
 * A7     - not used, A6+A7 only as analog pins available on Nano, Pro Mini and Mini's
 * 
 * D1     - Digital IN push button start/stop
 * D2     - Digital IN (ISR0) Hall sensor
 * D3     - OUT red LED (stoped / manuell move)
 * D4     - OUT green LED (auto mode, movement triggert by Hall sensor)
 *
 * D5     - Digital IN push button  - dirLeftButtonPin       - Direction Left, Start, Stop
 * D6     - Digital IN push button  - dirRightButtonPin      - Direction Right, Start, Stop
 * D7     - 
 * D8     - 
 * D9     - OUT Direction Nema Stepper Motor Rail
 * D10    - OUT Pulse for Nema Stepper Motor Rail
 * D11    - OUT Enable Stepper Motor rail
 * D12    - 
 * D13    - 
 * 
 */

#include <Arduino.h>
#include "WRKeyStateDef.h"
#include "NemaStepperLi.h"

#define VERSION "1.00"

// inputs
#define LEFT_ENDSWITCH_PIN A0
#define RIGHT_ENDSWITCH_PIN A1

#define STARTSTOP_BUTTON_PIN 1
#define SENSOR_PIN 2
#define DIR_LEFT_BUTTON_PIN 5
#define DIR_RIGHT_BUTTON_PIN 6

// outputs
#define RED_LED_PIN 3
#define GREEN_LED_PIN 4

// Nema stepper
#define NEMA_DIRECTION_PIN 9
#define NEMA_PULSE_PIN 10
#define NEMA_SLEEP_PIN 11  // sleep and reset pin of A4988 has to be connected

#define TO_LEFTSIDE 0
#define TO_RIGHTSIDE 1

#define TWISTWIDTH_STEPS 70     // 70 x 0.025mm = 1.75mm
#define SETUP_SPEED 6           // in mm/s
#define RUN_SPEED 9             // in mm/s

// application states
#define APPSTATE_STARTUP 1
#define APPSTATE_SETMODE 10
#define APPSTATE_INIT_RUN 11
#define APPSTATE_WAIT_START_POINT 12
#define APPSTATE_RUNMODE 20
#define APPSTATE_TWISTMOVE 22
#define APPSTATE_SKIPPULSE 24


//-- global accessable data ---------------------
uint8_t B100HzToggle = 0;  // 100 Hertz Signal
uint8_t ui10MilliSekCount = 0;

uint8_t leftEndswitchState = 0;
uint8_t rightEndswitchState = 0;
uint8_t startStopButtonState = 0;
uint8_t dirLeftButtonState = 0;
uint8_t dirRightButtonState = 0;

uint8_t railDirection = TO_LEFTSIDE;

volatile unsigned long pulseCount = 0;

// GT2 pulley circumference = number of teeth * division
// using 20 teeth pulle: 20 * 2mm = 40mm
// driver set to eight step = 1600 steps for one revolution.
// for 1,75mm we need 70 steps. Each step moves 0.025mm
NemaStepper railStepper(NEMA_DIRECTION_PIN, NEMA_PULSE_PIN, NEMA_SLEEP_PIN, 1600, 40);

// Interrupt is called once a millisecond,
SIGNAL(TIMER0_COMPA_vect) {
  unsigned long currentMillis = millis();
  ui10MilliSekCount++;

  if (ui10MilliSekCount >= 10) {
    ui10MilliSekCount = 0;
    B100HzToggle ^= 1;
  }
}

// Interrupt Service Routine: Jedes Mal wenn ein Puls registriert wird
void countPulse() {
  pulseCount++;
}

void setup() {
  // inputs
  pinMode(LEFT_ENDSWITCH_PIN, INPUT_PULLUP);
  pinMode(RIGHT_ENDSWITCH_PIN, INPUT_PULLUP);
  pinMode(STARTSTOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DIR_LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DIR_RIGHT_BUTTON_PIN, INPUT_PULLUP);

  // input Hall sensor
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  // ISR0 at pin D2 at falling flank
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), countPulse, FALLING);

  // outputs
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Timer setup --------------------------
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function below
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);

  // zum testen des ISR0 triggers
  Serial.begin(115200);
  Serial.println("Setup ISR0 on PIN2");
}


void CheckDirectionButtons() {
  if (dirLeftButtonState == 1)  // move rail to left
  {
    if (railStepper.getRunState() == 1 && railDirection == TO_LEFTSIDE) {
      railStepper.stopMotor();
    } else {
      railDirection = TO_LEFTSIDE;
      railStepper.setDirection(railDirection);
      railStepper.runMotor();
    }
  }

  if (dirRightButtonState == 1)  // move rail to right
  {
    if (railStepper.getRunState() == 1 && railDirection == TO_RIGHTSIDE) {
      railStepper.stopMotor();
    } else {
      railDirection = TO_RIGHTSIDE;
      railStepper.setDirection(railDirection);
      railStepper.runMotor();
    }
  }
}

void CheckDirectionAction() {
  if (dirLeftButtonState == 2 || dirRightButtonState == 2) {
    if (dirLeftButtonState == 2) {
      railDirection = TO_LEFTSIDE;
      railStepper.setDirection(railDirection);
      railStepper.runMotor();
    }

    if (dirRightButtonState == 2) {
      railDirection = TO_RIGHTSIDE;
      railStepper.setDirection(railDirection);
      railStepper.runMotor();
    }

  } else {
    railStepper.stopMotor();
  }
}

void resetPulseCount() {
  noInterrupts();  // pause interrupts
  pulseCount = 0;  // reset counter 
  interrupts();    // resume interrupts
}

void loop() {
  static uint8_t AppState = APPSTATE_STARTUP;
  static uint8_t StateTrigger = 0;
  uint8_t aktStateTrigger;
  static uint8_t sekCount = 0;
  static uint8_t runBlinkToogle = 0;
  static uint8_t runToogleCount = 0;
  static boolean skipPulse = false;


  aktStateTrigger = B100HzToggle;

  // >--- the code within the aktStateTrigger will be executed only 100 times per second.
  //      So you don't need further debounce for input buttons and switches
  if (aktStateTrigger != StateTrigger) {
    StateTrigger = aktStateTrigger;

    // check the state of all buttons
    CheckKeyState(&leftEndswitchState, LEFT_ENDSWITCH_PIN);
    CheckKeyState(&rightEndswitchState, RIGHT_ENDSWITCH_PIN);
    CheckKeyState(&startStopButtonState, STARTSTOP_BUTTON_PIN);
    CheckKeyState(&dirLeftButtonState, DIR_LEFT_BUTTON_PIN);
    CheckKeyState(&dirRightButtonState, DIR_RIGHT_BUTTON_PIN);

    if (runToogleCount >= 50) {
      runBlinkToogle ^= 1;

      runToogleCount = 0;
    } else {
      runToogleCount++;
    }

    // process application state machine --------------
    switch (AppState) {
      case APPSTATE_STARTUP:
        digitalWrite(RED_LED_PIN, HIGH);
        railStepper.enableMotor(HIGH);
        railStepper.setSpeedMMPS(SETUP_SPEED);
        AppState = APPSTATE_SETMODE;
        break;

      case APPSTATE_SETMODE:
        if (startStopButtonState == 1) {
          digitalWrite(GREEN_LED_PIN, HIGH);
          digitalWrite(RED_LED_PIN, LOW);
          AppState = APPSTATE_INIT_RUN;
        } else {
          //CheckDirectionButtons();
          CheckDirectionAction();
        }
        break;

      case APPSTATE_INIT_RUN:
        railStepper.setSpeedMMPS(50);
        railDirection = TO_LEFTSIDE;
        railStepper.setDirection(railDirection);
        railStepper.runMotor();
        AppState = APPSTATE_WAIT_START_POINT;
        break;

      case APPSTATE_WAIT_START_POINT:
        if (leftEndswitchState == 2) {
          railStepper.stopMotor();
          resetPulseCount();
          railStepper.setSpeedMMPS(RUN_SPEED);
          railDirection = TO_RIGHTSIDE;         // directiion from start position
          AppState = APPSTATE_RUNMODE;
        }
        break;

      case APPSTATE_RUNMODE:

        digitalWrite(GREEN_LED_PIN, runBlinkToogle);

        noInterrupts();  // Interrupts temporär deaktivieren
        
        if(pulseCount >= 1) {
            railStepper.resetCurrentStepCount();
            pulseCount = 0;  // Zähler zurücksetzen    
            
            if(skipPulse == false) {
              railStepper.setDirection(railDirection);     
              AppState = APPSTATE_TWISTMOVE;
              railStepper.runMotor();
            } else {
              skipPulse = false;
            }
          
        }
        
        interrupts();    // Interrupts wieder aktivieren
                         //Serial.print("Pulses counted: ");
                         //Serial.println(pulses);

        if (startStopButtonState == 1) {
          digitalWrite(GREEN_LED_PIN, LOW);
          digitalWrite(RED_LED_PIN, HIGH);
          railStepper.setSpeedMMPS(6);
          AppState = APPSTATE_SETMODE;
        }
        break;

      case APPSTATE_TWISTMOVE:
        digitalWrite(GREEN_LED_PIN, runBlinkToogle);
        if (railStepper.getCurrentStepCount() >= TWISTWIDTH_STEPS) {
          railStepper.stopMotor();
          AppState = APPSTATE_RUNMODE;
        }

        // change direction on right endswitch hit - layer change
        if(railDirection == TO_RIGHTSIDE && rightEndswitchState == 2) {
          railStepper.stopMotor();
          railDirection = TO_LEFTSIDE;
          AppState = APPSTATE_RUNMODE;
          skipPulse = true; // skip one revolution at layer change
        }

        // change direction on left endswitch hit - layer change
        if(railDirection == TO_LEFTSIDE && leftEndswitchState == 2) {
          railDirection = TO_RIGHTSIDE;
          AppState = APPSTATE_RUNMODE;
          skipPulse = true;
        }

        // Abbruch
        if (startStopButtonState == 1) {
          digitalWrite(GREEN_LED_PIN, LOW);
          digitalWrite(RED_LED_PIN, HIGH);
          railStepper.setSpeedMMPS(6);
          AppState = APPSTATE_SETMODE;
        }
        break;
    }

  }  // <--- State trigger end

  if(AppState == APPSTATE_SETMODE || AppState == APPSTATE_INIT_RUN || AppState == APPSTATE_WAIT_START_POINT) {
    if (railDirection == TO_LEFTSIDE && leftEndswitchState != 2 || railDirection == TO_RIGHTSIDE && rightEndswitchState != 2) {
      railStepper.motorStep();
    }
  }

  if(AppState == APPSTATE_TWISTMOVE) {
      if(railStepper.getCurrentStepCount() < TWISTWIDTH_STEPS) {
        if (railDirection == TO_LEFTSIDE && leftEndswitchState != 2 || railDirection == TO_RIGHTSIDE && rightEndswitchState != 2) {
        railStepper.motorStep();
      }
    }
  }

}
