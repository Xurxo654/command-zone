// Pin Definitions
const int CLK_PIN = 3;
const int DT_PIN = 4;
const int SW_PIN = 5;
const int BTN1_PIN = 6;
const int BTN2_PIN = 7;

// Rotary encoder variables
volatile int encoderPos = 0;
volatile int lastCLK = HIGH;
unsigned long lastEncoderMove = 0;
const unsigned long ENCODER_SETTLE_TIME = 1000; // 1 second

// Button handling
unsigned long btn1PressStart = 0;
unsigned long btn2PressStart = 0;
bool btn1WasPressed = false;
bool btn2WasPressed = false;
const unsigned long HOLD_THRESHOLD = 1000; // 1 second for hold

// State machine
enum State {
  LISTENING_ENCODER,
  BUTTON_INPUT_MODE
};
State currentState = LISTENING_ENCODER;

// Data
const int ARRAY_SIZE = 6;
string titles[ARRAY_SIZE] = {
  "Health",
  "Tax",
  "Commander Damage 1",
  "Commander Damage 2",
  "Commander Damage 3",
  "Poison"
};
int values[ARRAY_SIZE] = { 40, 0, 0, 0, 0, 0};
int currentIndex = 0;
int counter = 0;

void setup() {
  Serial.begin(9600);
  
  // Configure pins
  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  
  // Attach interrupt for rotary encoder
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, CHANGE);
  
  lastCLK = digitalRead(CLK_PIN);
  
  Serial.println("Rotary Encoder System Ready");
  Serial.print("Current Index: ");
  Serial.println(currentIndex);
}

void loop() {
  switch (currentState) {
    case LISTENING_ENCODER:
      handleEncoderListening();
      break;
      
    case BUTTON_INPUT_MODE:
      handleButtonInput();
      break;
  }
}

void encoderISR() {
  int clkState = digitalRead(CLK_PIN);
  int dtState = digitalRead(DT_PIN);
  
  // Check if CLK state changed
  if (clkState != lastCLK) {
    // Determine direction
    if (dtState != clkState) {
      encoderPos++;
    } else {
      encoderPos--;
    }
    lastEncoderMove = millis();
  }
  
  lastCLK = clkState;
}

void handleEncoderListening() {
  static int lastEncoderPos = 0;
  
  // Check if encoder has moved
  if (encoderPos != lastEncoderPos) {
    int delta = encoderPos - lastEncoderPos;
    lastEncoderPos = encoderPos;
    
    // Update index
    currentIndex += delta;
    
    // Handle wrapping
    if (currentIndex >= ARRAY_SIZE) {
      currentIndex = 0;
    } else if (currentIndex < 0) {
      currentIndex = ARRAY_SIZE - 1;
    }
    
    Serial.print("Index: ");
    Serial.println(currentIndex);
  }
  
  // Check if encoder has been idle for settle time
  if (millis() - lastEncoderMove >= ENCODER_SETTLE_TIME && lastEncoderMove > 0) {
    if (encoderPos != 0 || lastEncoderMove > 1000) { // Make sure we've actually moved
      Serial.println("Encoder settled. Entering button input mode...");
      Serial.print("Selected Index: ");
      Serial.println(currentIndex);
      currentState = BUTTON_INPUT_MODE;
      lastEncoderMove = millis(); // Reuse for timeout tracking
    }
  }
}

void handleButtonInput() {
  static unsigned long modeStartTime = millis();
  const unsigned long TIMEOUT = 3000; // 3 seconds
  
  bool btn1State = digitalRead(BTN1_PIN) == LOW;
  bool btn2State = digitalRead(BTN2_PIN) == LOW;
  
  // Handle Button 1 (Increment)
  if (btn1State && !btn1WasPressed) {
    // Button just pressed
    btn1PressStart = millis();
    btn1WasPressed = true;
  } else if (btn1State && btn1WasPressed) {
    // Button is being held
    if (millis() - btn1PressStart >= HOLD_THRESHOLD) {
      counter += 5;
      Serial.print("Button 1 HOLD - Counter: ");
      Serial.println(counter);
      btn1PressStart = millis(); // Reset to allow continuous hold increments
    }
  } else if (!btn1State && btn1WasPressed) {
    // Button released
    if (millis() - btn1PressStart < HOLD_THRESHOLD) {
      counter++;
      Serial.print("Button 1 PRESS - Counter: ");
      Serial.println(counter);
    }
    btn1WasPressed = false;
    modeStartTime = millis(); // Reset timeout on activity
  }
  
  // Handle Button 2 (Decrement)
  if (btn2State && !btn2WasPressed) {
    // Button just pressed
    btn2PressStart = millis();
    btn2WasPressed = true;
  } else if (btn2State && btn2WasPressed) {
    // Button is being held
    if (millis() - btn2PressStart >= HOLD_THRESHOLD) {
      counter -= 5;
      Serial.print("Button 2 HOLD - Counter: ");
      Serial.println(counter);
      btn2PressStart = millis(); // Reset to allow continuous hold decrements
    }
  } else if (!btn2State && btn2WasPressed) {
    // Button released
    if (millis() - btn2PressStart < HOLD_THRESHOLD) {
      counter--;
      Serial.print("Button 2 PRESS - Counter: ");
      Serial.println(counter);
    }
    btn2WasPressed = false;
    modeStartTime = millis(); // Reset timeout on activity
  }
  
  // Check for timeout
  if (millis() - modeStartTime >= TIMEOUT) {
    Serial.println("Timeout - Returning to encoder listening mode");
    Serial.print("Final Counter Value: ");
    Serial.println(counter);
    currentState = LISTENING_ENCODER;
    modeStartTime = millis();

    // apply counter to current value
    value[currentIndex] = value[currentIndex] + counter;
    
    // Reset counter
    counter = 0;
  }
}