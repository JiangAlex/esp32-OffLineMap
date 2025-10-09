//#include "Common/HAL/HAL.h"
#include "HAL.h"
// #include "ButtonEvent/ButtonEvent.h"
// #include "lvgl/lvgl.h"
#include "ButtonEvent.h"
#include "lvgl.h"
#include "Arduino.h"

static ButtonEvent EncoderPush(1000, 200, 200);  // 1s long press, 200ms repeat, 200ms double click

static bool EncoderEnable = true;
static volatile int32_t EncoderDiff = 0;
static bool EncoderDiffDisable = false;
static int PreviousCLK = 0;  // Track previous CLK state for encoder
static int PreviousDT = 0;   // Track previous DT state for encoder
static volatile int encoderValue = 0;  // Current encoder value
static int lastEncoderValue = 0;  // Last encoder value for change detection
static unsigned long TimeOfLastDebounce = 0;  // For debouncing encoder
static unsigned long DelayofDebounce = 50;    // Debounce delay in milliseconds

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#if 1
static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static bool lastState;
    static int readCount = 0;
    static unsigned long lastDiffTime = 0;
    
    // Get encoder difference - this also resets the internal counter
    int32_t diff = HAL::Encoder_GetDiff();
    data->enc_diff = diff;

    bool isPush = HAL::Encoder_GetIsPush();
    data->state = isPush ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    // Simple debug output only for significant changes
    if(diff != 0) {
        unsigned long currentTime = millis();
        unsigned long timeSinceLastDiff = currentTime - lastDiffTime;
        lastDiffTime = currentTime;
        
        printf("LVGL encoder diff=%d, readCount=%d, timeSince=%lu ms\r\n", diff, readCount, timeSinceLastDiff);
        
        // Check if there's a focused object in the group
        lv_group_t * group = lv_group_get_default();
        if(group) {
            lv_obj_t * focused = lv_group_get_focused(group);
            uint32_t obj_count = lv_group_get_obj_count(group);
            printf("LVGL encoder: focused object=%p, group has %d objects\r\n", focused, obj_count);
            
            // If group is empty or no focus, this indicates a problem
            if(obj_count == 0 || focused == NULL) {
                printf("LVGL encoder: WARNING - Group is empty or no focus! This may cause delays.\r\n");
            }
        } else {
            printf("LVGL encoder: No default group!\r\n");
        }
    }
    if(isPush != lastState) {
        printf("LVGL encoder push=%d\r\n", isPush);
    }
    
    readCount++;

    if(isPush != lastState)
    {
        HAL::Buzz_Tone(isPush ? 500 : 700, 20);
        lastState = isPush;
    }
}

static void lv_port_encoder() {
    /*Register a encoder input device*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_read;
    
    lv_indev_t * indev = lv_indev_drv_register(&indev_drv);
    
    // Create a group for encoder navigation
    lv_group_t * group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(indev, group);
    
    // Configure group behavior for scrolling
    lv_group_set_wrap(group, false);  // Don't wrap around when reaching end
    
    printf("LVGL encoder registered with group\r\n");
}

void HAL::Encoder_InitLVGL()
{
    printf("Encoder_InitLVGL: Starting LVGL encoder initialization\r\n");
    lv_port_encoder();
    printf("Encoder_InitLVGL: LVGL encoder initialization completed\r\n");
}
//-----------------------
void IRAM_ATTR handleEncoderChange() {
  int currentCLK = digitalRead(CONFIG_ENCODER_A_PIN);
  int currentDT = digitalRead(CONFIG_ENCODER_B_PIN);

  if (PreviousCLK == 0 && currentCLK == 1) {
    if (currentDT == 0) {
      encoderValue++;  // Clockwise
    } else {
      encoderValue--;  // Counter-Clockwise
    }
  } else if (PreviousCLK == 1 && currentCLK == 0) {
    if (currentDT == 1) {
      encoderValue++;  // Clockwise
    } else {
      encoderValue--;  // Counter-Clockwise
    }
  }

  PreviousCLK = currentCLK;
  PreviousDT = currentDT;
  Serial.println(encoderValue,DEC);
}

void IRAM_ATTR handleButtonPress() {
  unsigned long currentTime = millis();
  if (currentTime - TimeOfLastDebounce > DelayofDebounce) {
    TimeOfLastDebounce = currentTime;
    Serial.println("SW Button Pressed!");
  }
}

void readEncoderTask(void * pvParameters) {
  static int printCounter = 0;
  static int lastDiff = 0;
  static bool lastButtonState = false;
  static int buttonStateCounter = 0;
  
  for (;;) {
    if (lastEncoderValue != encoderValue) {
      // Handle encoder value changes
      Serial.print("Encoder Value Changed: ");
      Serial.print(lastEncoderValue);
      Serial.print(" -> ");
      Serial.println(encoderValue);
      lastEncoderValue = encoderValue;
    }
    
    // Check if EncoderDiff changed
    if(EncoderDiff != lastDiff) {
      Serial.print("EncoderDiff changed: ");
      Serial.print(lastDiff);
      Serial.print(" -> ");
      Serial.println(EncoderDiff);
      lastDiff = EncoderDiff;
    }
    
    // Monitor button state changes more frequently for debugging
    bool currentButtonState = (digitalRead(CONFIG_ENCODER_PUSH_PIN) == LOW);
    if(currentButtonState != lastButtonState) {
      Serial.print("RAW Button State: ");
      Serial.print(currentButtonState ? "LOW(Pressed)" : "HIGH(Released)");
      Serial.print(" Counter: ");
      Serial.println(buttonStateCounter);
      lastButtonState = currentButtonState;
      buttonStateCounter = 0;
    } else {
      buttonStateCounter++;
    }
    
    // Print pin states every 1000ms for debugging (less frequent)
    printCounter++;
    if(printCounter >= 1000) {
      printCounter = 0;
      int pinA = digitalRead(CONFIG_ENCODER_A_PIN);
      int pinB = digitalRead(CONFIG_ENCODER_B_PIN);
      int pinPush = digitalRead(CONFIG_ENCODER_PUSH_PIN);
      Serial.print("DEBUG - A:");
      Serial.print(pinA);
      Serial.print(" B:");
      Serial.print(pinB);
      Serial.print(" PUSH:");
      Serial.print(pinPush);
      Serial.print(" Diff:");
      Serial.println(EncoderDiff);
    }
    
    vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 ms
  }
}
#endif

static void Buzz_Handler(int dir)
{
    printf("Buzz_Handler \r\n");
    static const uint16_t freqStart = 2000;
    static uint16_t freq = freqStart;
    static uint32_t lastRotateTime;

    if(millis() - lastRotateTime > 1000)
    {
        freq = freqStart;
    }
    else
    {
        if(dir > 0)
        {
            freq += 100;
        }

        if(dir < 0)
        {
            freq -= 100;
        }

        freq = constrain(freq, 100, 20 * 1000);
    }

    lastRotateTime = millis();
    HAL::Buzz_Tone(freq, 5);
}


static void IRAM_ATTR Encoder_EventHandler()
{
    //printf("Encoder_EventHandler \r\n");  // Don't use printf in ISR
    portENTER_CRITICAL_ISR(&timerMux);
    if(!EncoderEnable || EncoderDiffDisable)
    {
        //Serial.println("Encoder_EventHandler return"); // REMOVED: Serial not safe in ISR
        portEXIT_CRITICAL_ISR(&timerMux);
        return;
    }

    // Use proper encoder direction detection based on A and B phase relationship
    int dir = 0;
    if (digitalRead(CONFIG_ENCODER_A_PIN) == LOW)
    {
        dir = (digitalRead(CONFIG_ENCODER_B_PIN) == LOW ? -1 : +1);
    }
    else 
    {
        dir = (digitalRead(CONFIG_ENCODER_B_PIN) == LOW ? +1 : -1);
    }

    EncoderDiff += dir;
    //Serial.println(EncoderDiff,DEC); // REMOVED: Serial not safe in ISR
    portEXIT_CRITICAL_ISR(&timerMux);
    //Buzz_Handler(dir);  // Call outside ISR would be better
}


static void Encoder_PushHandler(ButtonEvent* btn, int event)
{
    printf("=== Encoder_PushHandler TRIGGERED ===\r\n");
    printf("Event Code: %d\r\n", event);
    
    if(event == ButtonEvent::EVENT_PRESSED)          // 1
    {
        printf("✓ Button PRESSED - Disabling encoder rotation\r\n");
        EncoderDiffDisable = true;
    }
    else if(event == ButtonEvent::EVENT_PRESSING)    // 2
    {
        printf("✓ Button PRESSING (holding)\r\n");
        // Don't change EncoderDiffDisable state during pressing
    }
    else if(event == ButtonEvent::EVENT_LONG_PRESSED) // 3
    {
        printf("✓ Button LONG_PRESSED - Initiating shutdown\r\n");
        HAL::Audio_PlayMusic("Shutdown");
        HAL::Power_Shutdown();
    }
    else if(event == ButtonEvent::EVENT_RELEASED)    // 6
    {
        printf("✓ Button RELEASED - Enabling encoder rotation\r\n");
        EncoderDiffDisable = false;
    }
    else if(event == ButtonEvent::EVENT_CLICKED)     // 8
    {
        printf("✓ Button CLICKED\r\n");
    }
    else if(event == ButtonEvent::EVENT_SHORT_CLICKED) // 9
    {
        printf("✓ Button SHORT_CLICKED\r\n");
    }
    else if(event == ButtonEvent::EVENT_DOUBLE_CLICKED) // 10
    {
        printf("✓ Button DOUBLE_CLICKED\r\n");
    }
    else if(event == ButtonEvent::EVENT_CHANGED)     // 7
    {
        printf("✓ Button state CHANGED\r\n");
    }
    else
    {
        printf("? Unknown event: %d\r\n", event);
    }
    printf("=====================================\r\n");
}

void HAL::Encoder_Init()
{
    printf("Encoder_Init \r\n");
    pinMode(CONFIG_ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(CONFIG_ENCODER_B_PIN, INPUT_PULLUP);
    pinMode(CONFIG_ENCODER_PUSH_PIN, INPUT_PULLUP);

    attachInterrupt(CONFIG_ENCODER_A_PIN, Encoder_EventHandler, FALLING);
    printf("attachInterrupt Encoder_EventHandler end \r\n");
    //attachInterrupt(digitalPinToInterrupt(CONFIG_ENCODER_A_PIN), handleEncoderChange, CHANGE);
    //attachInterrupt(digitalPinToInterrupt(CONFIG_ENCODER_PUSH_PIN), handleButtonPress, FALLING);

    PreviousCLK = digitalRead(CONFIG_ENCODER_A_PIN);
    PreviousDT = digitalRead(CONFIG_ENCODER_B_PIN);

    EncoderPush.EventAttach(Encoder_PushHandler);
    printf("EncoderPush.EventAttach end \r\n");

    // Create encoder debugging task with larger stack
    xTaskCreatePinnedToCore(
        readEncoderTask,    // Function to implement the task
        "readEncoderTask",  // Name of the task
        4096,               // Stack size in bytes (reduced from 10000)
        NULL,               // Task input parameter
        1,                  // Priority of the task
        NULL,               // Task handle
        0                   // Core where the task should run
    );
    printf("Encoder debug task created \r\n");
    
    // Don't call lv_port_encoder() here - will be called after LVGL is fully ready
    printf("Encoder_Init completed - LVGL encoder will be initialized later\r\n");
}

void HAL::Encoder_Update()
{
    // Simplified button monitoring - let ButtonEvent handle debouncing
    static int updateCounter = 0;
    static int debugCounter = 0;
    
    updateCounter++;
    debugCounter++;
    
    // Debug print every 500 calls to show update is working
    if(debugCounter >= 500) {
        bool buttonState = Encoder_GetIsPush();
        printf("Encoder_Update: ButtonState=%d, DebugCount=%d, UpdateCount=%d\r\n", 
               buttonState, debugCounter, updateCounter);
        debugCounter = 0;
    }
    
    if(updateCounter >= 10) {  // Check button every 10 calls
        updateCounter = 0;
        bool buttonState = Encoder_GetIsPush();
        EncoderPush.EventMonitor(buttonState);
    }
}

int32_t HAL::Encoder_GetDiff()
{
    int32_t diff = EncoderDiff;
    EncoderDiff = 0;
    return diff;
}

bool HAL::Encoder_GetIsPush()
{
    if(!EncoderEnable)
    {
        return false;
    }
    
    // Simple single reading - let ButtonEvent handle debouncing
    return (digitalRead(CONFIG_ENCODER_PUSH_PIN) == LOW);
}

void HAL::Encoder_SetEnable(bool en)
{
    EncoderEnable = en;
}

// Debug function to test encoder pins
void HAL::Encoder_PrintPinStates()
{
    int pinA = digitalRead(CONFIG_ENCODER_A_PIN);
    int pinB = digitalRead(CONFIG_ENCODER_B_PIN);
    int pinPush = digitalRead(CONFIG_ENCODER_PUSH_PIN);
    
    Serial.print("Encoder Pins - A: ");
    Serial.print(pinA);
    Serial.print(", B: ");
    Serial.print(pinB);
    Serial.print(", Push: ");
    Serial.print(pinPush);
    Serial.print(", Diff: ");
    Serial.print(EncoderDiff);
    Serial.print(", Value: ");
    Serial.println(encoderValue);
}
