#include <Stepper.h>

// change this to the number of steps on your motor
#define STEPS_PER_REV 2038 

const int sensorMin = 370;
const int sensorMax = 1020;
const int PHOTOCELL_PIN = A2;
const int BUTTON1_PIN = 2;
const int BUTTON2_PIN = 3;

#define TRANSITION_LEVEL 10

// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS_PER_REV, 8, 10, 9, 11);
bool blindsOpen = false;
bool blindOverride = false;
int trendCount = 0;

int stepCount = 0;  // number of steps the motor has taken
bool stepCountSet = false;

int mode = 1;

void setup() {  
  Serial.begin(9600);

  stepper.setSpeed(10);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  
  blink_ok();
}


void blink_ok() {
  for (int i = 0; i<2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5);
    digitalWrite(LED_BUILTIN, LOW);
    delay(5);    
    digitalWrite(LED_BUILTIN, HIGH);
    delay(5);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void motorOff() {
  digitalWrite(8,LOW);
  digitalWrite(9,LOW);
  digitalWrite(10,LOW);
  digitalWrite(11,LOW);
}

void openBlinds() {
  Serial.println("Opening Blinds...");
  stepper.step(-stepCount);
  motorOff();
  blindsOpen = true;
  trendCount = 0;
}

void closeBlinds() {
  Serial.println("Closing Blinds...");
  stepper.step(stepCount);
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

  Serial.print("MODE 1:  B1: ");
  Serial.print(button1); Serial.print("   B2: ");
  Serial.print(button2); Serial.print("   STEP_CT: ");
  Serial.println(stepCount);

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
      blink_ok();
      if (!stepCountSet) {
        Serial.println("Step count reset");
        stepCount = 0;
        stepCountSet = true;
        delay(2000);
        blink_ok();
      } else {
        Serial.println("Calibration complete.");
        mode = 2;
        delay(500);
        blink_ok();
      }
    }
    
    delay(20);
  }
}


void mode2_loop() {
  int button1 = digitalRead(BUTTON1_PIN);
  int button2 = digitalRead(BUTTON2_PIN);
  
  int analogValue = analogRead(PHOTOCELL_PIN);
  int value = map(analogValue, sensorMin, sensorMax, 0, 100);
  Serial.print("MODE 2:  ");
  if (blindsOpen)
    Serial.print("OPEN  ");
  else
    Serial.print("CLOSED  ");
  if (blindOverride)
    Serial.print(" (OVERRIDE) ");
  Serial.print("  Light Level: "); Serial.print(analogValue);  Serial.print("  Adjusted: ");
  Serial.print(value);  Serial.print("   Trend: ");
  Serial.println(trendCount);

  if (button1 == LOW || button2 == LOW) {
    blindOverride = !blindOverride;
    blink_ok();
    delay(1000);
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
