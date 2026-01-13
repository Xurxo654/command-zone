// Pin Definitions
const int CLK_PIN = 3;
const int DT_PIN = 4;
const int SW_PIN = 5;

// Rotary encoder variables
volatile int encoderPos = 0;
volatile int lastCLK = HIGH;

// Button debouncing
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 250;

// State machine
enum State {
  DISPLAY_STATE,
  SELECTION_STATE,
  EDIT_STATE
};
State currentState = DISPLAY_STATE;

// Data
const int ARRAY_SIZE = 6;
String titles[ARRAY_SIZE] = {
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
  
  // Attach interrupt for rotary encoder
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, CHANGE);
  
  lastCLK = digitalRead(CLK_PIN);
  
  Serial.println("Rotary Encoder System Ready");
  Serial.print("Current Index: ");
  Serial.println(currentIndex);
}

void loop() {
  bool buttonPressed = checkButtonPress();

  switch (currentState) {
    case DISPLAY_STATE:
      handleDisplayState(buttonPressed);
      break;
      
    case SELECTION_STATE:
      handleSelectionState(buttonPressed);
      break;
    
    case EDIT_STATE:
      handleEditState(buttonPressed);
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
      encoderPos--;
    } else {
      encoderPos++;
    }
  }
  
  lastCLK = clkState;
}

bool checkButtonPress() {
  bool buttonState = digitalRead(SW_PIN) == LOW;
  
  if (buttonState && (millis() - lastButtonPress > DEBOUNCE_DELAY)) {
    lastButtonPress = millis();
    return true;
  }
  
  return false;
}

void handleDisplayState(bool buttonPressed) {
  static bool displayStateInitialized = false;
  static int lastEncoderPos = 0;

  if (!displayStateInitialized) {
    Serial.print(titles[currentIndex] + " ");
    Serial.println(values[currentIndex]);
    displayStateInitialized = true;
  }

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
    
    Serial.print(titles[currentIndex] + " ");
    Serial.println(values[currentIndex]);
  }
  

  // In display state, just wait for button press to enter selection
  if (buttonPressed) {
    Serial.println("\n--- Entering SELECTION mode ---");
    Serial.print("Current Index: ");
    Serial.println(currentIndex);
    Serial.println("Rotate encoder to change selection");
    currentState = SELECTION_STATE;
    encoderPos = 0; // Reset encoder position for relative movement
    displayStateInitialized = false;
  }
}

void handleSelectionState(bool buttonPressed) {
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
    
    Serial.println(titles[currentIndex]);
  }
  
  // Button press confirms selection and enters edit mode
  if (buttonPressed) {
    Serial.println("\n--- Entering EDIT mode ---");
    Serial.print("Editing: ");
    Serial.println(titles[currentIndex]);
    Serial.print("Current value: ");
    Serial.println(values[currentIndex]);
    Serial.println("Rotate encoder to adjust counter");
    currentState = EDIT_STATE;
    encoderPos = 0; // Reset encoder position for counter adjustment
    lastEncoderPos = 0;
  }
}

void handleEditState(bool buttonPressed) {
  static int lastEncoderPos = 0;
  
  // Check if encoder has moved
  if (encoderPos != lastEncoderPos) {
    int delta = encoderPos - lastEncoderPos;
    lastEncoderPos = encoderPos;
    
    // Update counter
    counter += delta;
    
    Serial.print("Counter: ");
    Serial.println(counter);
  }
  
  // Button press confirms edit and returns to display
  if (buttonPressed) {
    Serial.println("\n--- Returning to DISPLAY mode ---");
    Serial.print("Final Counter Value: ");
    Serial.println(counter);
    Serial.print("Selected Index: ");
    Serial.println(currentIndex);
    Serial.println("Press button to enter SELECTION mode");
    currentState = DISPLAY_STATE;
    encoderPos = 0; // Reset encoder position
    lastEncoderPos = 0;
    values[currentIndex] = values[currentIndex] + counter;
    counter = 0;
  }
}

