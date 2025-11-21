#include "Arduino.h"
/***
 * Project: NemaStepperLi
 * File   : NemaStepperLi.cpp
 * Author : Werner Riemann 
 * Created: 05.11.2025
 * Board: Arduino Nano
 * 
 * Description: implements the class
 * 
 * Pins : DirectionPin, StepPin and SleepPin
 * 
 */

#include "NemaStepperLi.h"

NemaStepper::NemaStepper(uint8_t dirPin, uint8_t stepPin, uint8_t sleepPin, int stepsPerRevolution, uint8_t pulleyCircumference)
{
    motorSignal = 0;

    this->directionPin = dirPin;
    this->steppingPin = stepPin;
    this->sleepingPin = sleepPin;
    this->stepsPerRevolution = stepsPerRevolution;
    this->pulleyCircumference = pulleyCircumference;
    this->curDirection = STEPDIR_CLOCKWISE;
    this->last_wavepart_time = 0;

    pinMode(this->steppingPin, OUTPUT);
    pinMode(this->directionPin, OUTPUT);
    pinMode(this->sleepingPin, OUTPUT);
    digitalWrite(this->directionPin, STEPDIR_CLOCKWISE);
    digitalWrite(this->sleepingPin, LOW);  // default motor off    
}

NemaStepper::~NemaStepper()
{
}


void NemaStepper::setSpeedRPM(long speedInRPM) {
  // time in micro seconds for each step depending of the result speed RAMP_ADDSTEPS
  // to enable slow speeds, the param speedInRPM is interpreted as tenth rotations per minute.
  // so we need to 10 tenth for one rotation per minute
  this->step_waveTime = 60L * 1000L * 1000L / this->stepsPerRevolution / ( speedInRPM / 10.0);
  this->step_halfWaveTime = this->step_waveTime / 2;
}

void NemaStepper::setSpeedMMPS(uint8_t speedInMMPS) {
  float circumf = this->pulleyCircumference;
  long speedTenthRPM = speedInMMPS / circumf * 60 * 10;
  setSpeedRPM(speedTenthRPM);

}


// Turns off the motor. It can then rotate freely.
// 1, HIGH or True enables the MOSFet output of the driver
void NemaStepper::enableMotor(uint8_t enableState) {
  digitalWrite(this->sleepingPin, enableState);
}

void NemaStepper::setDirection(uint8_t direction) {
  this->curDirection = direction;
  digitalWrite(this->directionPin, this->curDirection);
}

uint8_t NemaStepper::getRunState() {
  return this->motorRun;
}

unsigned long NemaStepper::getCurrentStepCount() {
  return this->currentStepCount;
}

void NemaStepper::resetCurrentStepCount() {
  this->currentStepCount = 0;
}

void NemaStepper::setPositionSteps(unsigned long steps) {
  this->curStepPosition = steps;
}

unsigned long NemaStepper::getPositionSteps() {
  return this->curStepPosition;
}

void NemaStepper::setRailSizeSteps(unsigned long totalSteps) {
  this->railSizeSteps = totalSteps;
}

unsigned long NemaStepper::getRailSizeSteps() {
  return this->railSizeSteps;
}


void NemaStepper::stopMotor()
{
    this->motorRun = 0;
    this->motorSignal = 0;
}


void NemaStepper::runMotor()
{
  this->motorRun = 1;
  this->motorSignal = 1;
  
  digitalWrite(this->steppingPin, this->motorSignal);
}

void NemaStepper::motorStep()
{
  static uint8_t rdelay=0;
  uint8_t doStepper = 0;
  unsigned long now = micros();

  if(now - this->last_wavepart_time >= this->step_halfWaveTime)
    doStepper = 1;


  // toggle the motorSignal if half of the full wave is passed
  if(doStepper == 1 && this->motorRun == 1)
  {
    this->motorSignal ^= 1;
    digitalWrite(this->steppingPin, this->motorSignal);
    this->last_wavepart_time = now;

    if(this->motorSignal) {
      this->currentStepCount ++;
      
      if(this->curDirection == STEPDIR_CLOCKWISE) {
        this->curStepPosition ++;    
      } 
      else {
        this->curStepPosition --;
      }
   
    }
      
  }


}

