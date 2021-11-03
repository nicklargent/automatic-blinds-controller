#include <Stepper.h>

// change this to the number of steps on your motor
#define STEPS_PER_REV 2038 

const int sensorMin = 370;
const int sensorMax = 1020;
const int PHOTOCELL_PIN = A2;
const int BUTTON1_PIN = 2;
const int BUTTON2_PIN = 3;
const int LED_PIN = LED_BUILTIN; //6;

#define TRANSITION_LEVEL 10


#define Sprint(a) //(Serial.print(a))
#define Sprintln(a) //(Serial.println(a))

// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS_PER_REV, 8, 10, 9, 11);
bool blindsOpen = false;
bool blindOverride = false;
int trendCount = 0;

long stepCount = 0;  // number of steps the motor has taken
bool stepCountSet = false;

int mode = 1;


void setup() {  
  Serial.begin(9600);

  stepper.setSpeed(15);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // test
  //stepCount = 47320;
  //blindsOpen = true;
  //mode = 2;
}

void motorOff() {
  digitalWrite(8,LOW);
  digitalWrite(9,LOW);
  digitalWrite(10,LOW);
  digitalWrite(11,LOW);
}

void openBlinds() {
  Sprintln("Opening Blinds...");
  long stepsRemaining = stepCount;
  while (stepsRemaining > 0) {
    int steps = STEPS_PER_REV / 100;
    stepper.step(steps);
    stepsRemaining -= steps;
  }
  motorOff();
  blindsOpen = true;
  trendCount = 0;
}

void closeBlinds() {
  Sprintln("Closing Blinds...");
  long stepsRemaining = stepCount;
  while (stepsRemaining > 0) {
    int steps = STEPS_PER_REV / 100;
    stepper.step(-steps);
    stepsRemaining -= steps;
  }  
  motorOff();
  blindsOpen = false;
  trendCount = 0;
}


// direct control of blinds with the two buttons.
// Move blinds to the open position and press both buttons to reset step count.
// Move blinds to the closed position and press both buttons to set step count.
// Moves into mode2 after second button..
void mode1_loop() {
  // read the button states:
  int button1 = digitalRead(BUTTON1_PIN);
  int button2 = digitalRead(BUTTON2_PIN);

  Sprint("MODE 1:  B1: ");
  Sprint(button1); Sprint("   B2: ");
  Sprint(button2); Sprint("   STEP_CT: ");
  Sprintln(stepCount);

  if (stepCountSet)
    digitalWrite(LED_PIN, (millis() / 500) % 2);
  else
    digitalWrite(LED_PIN, (millis() / 100) % 2);

  // set the motor speed:
  if (button1 == LOW && button2 == HIGH) {
    stepCount += (STEPS_PER_REV / 100);
    stepper.step(STEPS_PER_REV / 100);
  } else if (button1 == HIGH && button2 == LOW) {
    stepCount += (STEPS_PER_REV / -100);
    stepper.step(STEPS_PER_REV / -100);
  } else {
    motorOff();

    if (button1 == LOW && button2 == LOW) {
      if (!stepCountSet) {
        Sprintln("Step count reset");
        stepCount = 0;
        stepCountSet = true;
        delay(2000);
      } else {
        stepCount = abs(stepCount);
        Sprintln("Calibration complete.");
        mode = 2;
        digitalWrite(LED_PIN, 0);
        delay(500);
      }
    }
    
    delay(10);
  }
}


void mode2_loop() {
  int button1 = digitalRead(BUTTON1_PIN);
  int button2 = digitalRead(BUTTON2_PIN);
  
  int analogValue = analogRead(PHOTOCELL_PIN);
  int value = map(analogValue, sensorMin, sensorMax, 0, 100);
  Sprint("MODE 2:  ");
  if (blindsOpen)
    Sprint("OPEN  ");
  else
    Sprint("CLOSED  ");
  if (blindOverride)
    Sprint(" (OVERRIDE) ");
  Sprint("  Light Level: "); Sprint(analogValue);  Sprint("  Adjusted: ");
  Sprint(value);  Sprint("   Trend: ");
  Sprintln(trendCount);

  if (button1 == LOW || button2 == LOW) {
    blindOverride = !blindOverride;
    digitalWrite(LED_PIN, blindOverride);
    
    if (blindsOpen)
      closeBlinds();
    else
      openBlinds();
  }
  
  if (!blindOverride) {
    if (blindsOpen) {
      if (value < TRANSITION_LEVEL)
        trendCount ++;
      else
        trendCount = 0;
  
      if (trendCount >= 5) {
        closeBlinds();
      }
    } else {
      if (value > TRANSITION_LEVEL)
        trendCount ++;
      else
        trendCount = 0;
  
      if (trendCount >= 5) {
        openBlinds();
      }
    }
  }
  
  delay (1000);
}


void loop() {
  switch (mode) {
    case 1:
      mode1_loop();
      break;
    case 2:
      mode2_loop();
      break;
  }
}
