#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>

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
struct Stat {
  const char* title;
  int value;
  int max;
  int min;
  bool impact;
};
const int ARRAY_SIZE = 7;
Stat stats[ARRAY_SIZE] = {
  {"Health", 40, 256, 0, false},
  {"Tax", 0, 20, 0, false},
  {"DMG 1", 0, 21, 0, true},
  {"DMG 2", 0, 21, 0, true},
  {"DMG 3", 0, 21, 0, true},
  {"Poison", 0, 11, 0, false},
  {"Energy", 0, 256, 0, false}
};

int currentIndex = 0;
int counter = 0;

// Display
Adafruit_SSD1306 display(128, 32, &Wire, -1);


void setup() {
  Serial.begin(9600);
  
  // Configure pins
  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP);
  
  // Attach interrupt for rotary encoder
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, CHANGE);
  
  lastCLK = digitalRead(CLK_PIN);

  // set up display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Try 0x3D if this fails
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(2,2);
  display.println("Command Zone");
  display.display();
}

void loop() {
  bool buttonPressed = checkButtonPress();

  switch (currentState) {
    case DISPLAY_STATE:
      handleDisplayState(buttonPressed);
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

void setDisplayDisplay() {
  display.clearDisplay();
  display.display();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(2,2);
  display.print(stats[currentIndex].title);
  display.print(": ");
  display.println(stats[currentIndex].value);
  display.display();
}

void setEditDisplay(int delta) {
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(2,2);
  display.print(stats[currentIndex].title);
  display.print(": ");
  display.println(stats[currentIndex].value + delta);
  display.setCursor(2, 17);
  display.print("Delta: ");
  display.println(delta);
  display.display();
}

void handleDisplayState(bool buttonPressed) {
  static bool displayStateInitialized = false;
  static int lastEncoderPos = 0;

  if (!displayStateInitialized) {
    Serial.println("Display State Initialized");
    Serial.print(stats[currentIndex].title);
    Serial.print(": ");
    Serial.println(stats[currentIndex].value);
    displayStateInitialized = true;
    setDisplayDisplay();
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
    // Serial
    Serial.print(stats[currentIndex].title);
    Serial.print(": ");
    Serial.println(stats[currentIndex].value);

    // Display
    setDisplayDisplay();

  }
  

  // In display state, just wait for button press to enter selection
  if (buttonPressed) {
    Serial.println("\n--- Entering EDIT mode ---");
    Serial.print("Current Index: ");
    Serial.println(currentIndex);
    Serial.println("Rotate encoder to change selection");
    currentState = EDIT_STATE;
    encoderPos = 0; // Reset encoder position for relative movement
    lastEncoderPos = 0;
    displayStateInitialized = false;
  }
}

void handleEditState(bool buttonPressed) {
  static int lastEncoderPos = 0;
  static bool editInitialized = false;

  if (!editInitialized) {
    Serial.println("Edit State Initialized.");
    editInitialized = true;
    setEditDisplay(counter);
  }
  
  // Check if encoder has moved
  if (encoderPos != lastEncoderPos) {
    int delta = encoderPos - lastEncoderPos;
    lastEncoderPos = encoderPos;
    
    // Update counter
    counter += delta;
    counter = constrain(counter, 0-stats[currentIndex].value, stats[currentIndex].max - stats[currentIndex].value);
    
    Serial.print("Counter: ");
    Serial.println(counter);
    setEditDisplay(counter);
  }

  
  
  // Button press confirms edit and returns to display
  if (buttonPressed) {
    Serial.println("\n--- Returning to DISPLAY mode ---");
    Serial.print("Final Counter Value: ");
    Serial.println(counter);
    Serial.print("Selected Index: ");
    Serial.println(currentIndex);
    Serial.println("Press button to enter SELECTION mode");

    // Change value
    stats[currentIndex].value += counter;
    if (stats[currentIndex].impact) {
      stats[0].value -= counter;
    }

    currentState = DISPLAY_STATE;
    encoderPos = 0; // Reset encoder position
    lastEncoderPos = 0;
    counter = 0;
    editInitialized = false;
  }
}

