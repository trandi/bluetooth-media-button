#include <Arduino.h>
#include <BleKeyboard.h>
#include <ESP32Encoder.h>
#include <Streaming.h> // use Arduino Serial with "<<" and ">>" operators

const gpio_num_t ROTARY_ENC_PIN_A{GPIO_NUM_35};
const gpio_num_t ROTARY_ENC_PIN_B{GPIO_NUM_34};
const gpio_num_t PUSH_BUTTON_PIN{GPIO_NUM_33};

BleKeyboard bleKeyboard("Volume Button");
ESP32Encoder enc;
volatile boolean pushButtonPressed{false};
volatile unsigned long pushButtonPressed_Timestamp{millis()};
unsigned long lastAction_Timestamp{millis()};

/*
* The interrupt handling routine should have the IRAM_ATTR attribute, in order for the compiler to place the code in IRAM. 
* Also, interrupt handling routines should only call functions also placed in IRAM, as can be seen in the IDF documentation
*/
void IRAM_ATTR pushButtonCallback() {
  // DEBOUNCE, require a minimum duration between presses
  unsigned long time = millis();
  if (time - pushButtonPressed_Timestamp > 200) {
    pushButtonPressed = true;
    pushButtonPressed_Timestamp = time;
  }
}

void setup() {
  Serial.begin(115200);

  // Initialise push button/switch
  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(PUSH_BUTTON_PIN, pushButtonCallback, RISING);

  // Initialize Rotary Encoder
  ESP32Encoder::useInternalWeakPullResistors = UP;
  // set the pins such that the COUNT increases when rotating Clockwise
  enc.attachSingleEdge(ROTARY_ENC_PIN_A, ROTARY_ENC_PIN_B);
  enc.clearCount();

  

  // Initialize BLE Keyboard
  bleKeyboard.begin();
}



int i{0};
boolean up{true};
long count{0};
void loop() {
  unsigned long currentTime = millis();

  if(pushButtonPressed) {
    Serial << "Play/Pause" << endl;
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    pushButtonPressed = false;
    lastAction_Timestamp = currentTime;
  }

  long newCount = enc.getCount();
  if(newCount - count > 10) {
    Serial << newCount << endl;
    count = newCount;
    if(bleKeyboard.isConnected()) {
      bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
    }
    lastAction_Timestamp = currentTime;
  } else if(count - newCount > 10) {
    Serial << newCount << endl;
    count = newCount;
    if(bleKeyboard.isConnected()) {
      bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
    }
    lastAction_Timestamp = currentTime;
  }
  
  // is there any activity or should we go to sleep ?
  if(currentTime - lastAction_Timestamp > 30000) {
    Serial << "Going to a well deserved SLEEP " << endl;
    //Configure GPIO34 (connected to the rotary encoder) as ext0 wake up source for LOW logic level
    // can be one of RTC GPIOs 0,2,4,12-15,25-27,32-39 ONLY
    esp_sleep_enable_ext0_wakeup(PUSH_BUTTON_PIN, LOW);
    esp_deep_sleep_start();
  }

  // Make sure we don't overwhelm the Bluetooth link 
  // Otherwise it breaks the connection, at least under Linux
  delay(100); 
}



