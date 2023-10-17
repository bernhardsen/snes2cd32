#define CONFIG_BUTTON   B00000100
#define START_BUTTON    B00001000
#define SHOULDER_L      B00000100
#define SHOULDER_R      B00001000
#define DPAD_UP         B00010000
#define DPAD_DOWN       B00100000
#define BUTTON_B        B00000001
#define BUTTON_Y        B00000010
#define BUTTON_A        B00000001
#define BUTTON_X        B00000010

#define SWAP_ENABLED    B01000000

#define MACRO_FRAME_DELAY  19900 // old "ok" = 16200

#define TURBO_INTERVAL    4
#define PULL_DELAY        2450

#define delaySec(N)    (B11000000 | (0x1F & N))
#define delayFrm(N)    (B10000000 | (0x1F & N))

#define STATUS_LED_ON   PORTB |= B00000001
#define STATUS_LED_OFF  PORTB &= B11111110

int cState1 = 0x00;
int cState2 = 0x00;
int mState1 = 0xFF;
int mState2 = 0xFF;

byte mQx = B11001100;
byte mQy = B11001100;
byte posX = 0;
byte posY = 0;

int swapConfigUp = 0x00; // Format: RE XX XB BB - R = cState1 or cState2, BB = which bit to replace, X = Unused, E = enabled
int swapConfigDown = 0x00;

byte turboConfig = 0x00;
byte turboCount = 0;

int autoFireConfig = 0x00; // bits: ES RL AB XY
int autoFireTimer = 0x00;

byte macroMem[256];

void setup() {
  DDRB =  B00101000; // PB3 and PB5 as output, the rest as input // + PB0 (status LED)
  DDRC |= B00001111; // Set PC0 - PC3 as outputs
  DDRD |= B11111111; // Set all of PORTD to output

  PORTB |= B00001000; // Set clock high
  PORTC = B00001111;
  PORTD =  B11111111;

  macroMem[0] = 0xFF; // set 1st byte to "End macro"

  delayMicroseconds(1000); // 1ms
  readController();
  readController();
}

void loop() {
  readController();
  checkConfig();
  processButtons();
  writeOutput();
}

void readController() {
  int i;
  
  PORTB |= B00100000; // write 1 to PB5
  delayMicroseconds(12); // This delay could maybe be used to do other things, maybe do some logic and wait less
  PORTB &= B11011111; // write 0 to PB5
  delayMicroseconds(6);

  cState1 = 0x00;
  cState2 = 0x00;
  
  for(i = 0; i < 8; i++) {
    if (PINB & 0x10) {
      cState1 |= 1 << i;
    }
    PORTB &= B11110111; // write 0 to PB3
    delayMicroseconds(6);
    PORTB |= B00001000; // write 1 to PB3
    delayMicroseconds(6);
  }
  
  for(i = 0; i < 8; i++) {
    if (PINB & 0x10) {
      cState2 |= 1 << (i);
    }
    PORTB &= B11110111; // write 0 to PB3
    delayMicroseconds(6);
    PORTB |= B00001000; // write 1 to PB3
    delayMicroseconds(6);
  }
}

#define rotL(v)     ((v << 1) | (v >> 7))
#define rotR(v)     ((v >> 1) | (v << 7))

void writeOutput() {
  PORTD = cState1; // Write first 8 bits to PORTB (pin 0-7)
  PORTC = cState2 & 0x0F; // Write next 4 bits to PORTD (pin 10-13)
}

void writeBlank() {
  // No buttons pressed
  PORTD = 0xFF;
  PORTC = 0xFF;
}

int getButtonSwapConfigValue() {
  do {
    readController();
  } while (cState1 == 0xFF && cState2 == 0xFF); // Wait for a button to be pressed

  if (!(cState1 & B00010000) || !(cState1 & B00100000)) { // Up or down button
    return B00000000; // Disable button swap
  } else if (!(cState1 & B00000010)) { // Y button
    return B01000001;
  } else if (!(cState1 & B00000001)) { // B button
    return B01000000;
  } else if (!(cState2 & B00000001)) { // A button
    return B11000000;
  } else if (!(cState2 & B00000010)) { // X button
    return B11000001;
  } else if (!(cState2 & B00000100)) { // L button
    return B11000010;
  } else if (!(cState2 & B00001000)) { // R button
    return B11000011;
  }
}

void configTurbo() {
  do {
    readController();
  } while (cState1 & CONFIG_BUTTON); // Wait until select is pressed
  turboConfig = 0x00;
  
  // read what face buttons are pressed and write to turbo config
  if (!(cState1 & BUTTON_B)) {
    turboConfig |= B00000001;
  }
  if (!(cState1 & BUTTON_Y)) {
    turboConfig |= B00000010;
  }
  if (!(cState2 & BUTTON_A)) {
    turboConfig |= B00000100;
  }
  if (!(cState2 & BUTTON_X)) {
    turboConfig |= B00001000;
  }
  if (!(cState2 & SHOULDER_L)) {
    turboConfig |= B00010000;
  }
  if (!(cState2 & SHOULDER_R)) {
    turboConfig |= B00100000;
  }
}

void applyTurbo() {
  if (!turboConfig) {
    return;
  }
  turboCount++;
  if (turboCount < TURBO_INTERVAL) {
    return;
  }
  turboCount = 0;
  
  if (turboConfig & 0x80) {
    if (turboConfig & 0x01) {
      cState1 |= BUTTON_B;
    }
    if (turboConfig & 0x02) {
      cState1 |= BUTTON_Y;
    }
    if (turboConfig & 0x04) {
      cState2 |= BUTTON_A;
    }
    if (turboConfig & 0x08) {
      cState2 |= BUTTON_X;
    }
    if (turboConfig & 0x10) {
      cState2 |= SHOULDER_L;
    }
    if (turboConfig & 0x20) {
      cState2 |= SHOULDER_R;
    }
    
    turboConfig &= 0x7F;
  } else {
    turboConfig |= 0x80;
  }
}


void buttonSwap() {
  if (!(swapConfigUp & SWAP_ENABLED)) {
    return;
  }
  boolean upValue = (cState1 & B00010000) == B00010000;
  boolean swapValue = (((swapConfigUp & 0x80) ? cState2 : cState1) >> (swapConfigUp & 0x07)) & 0x01 == 0x01;
  
  if (swapValue) {
    // Set the up bit
    cState1 |= B00010000;
  } else {
    // Clear up bit
    cState1 &= B11101111;
  }
  
  if (upValue) {
    // Set bit
    if (swapConfigUp & 0x80) { // cState2
      cState2 |= 1 << (swapConfigUp & 0x07);
    } else { // cState1
      cState1 |= 1 << (swapConfigUp & 0x07);
    }
  } else {
    // Clear bit
    if (swapConfigUp & 0x80) { // cState2
      cState2 &= 0xFF ^ (1 << (swapConfigUp & 0x07));
    } else { // cState1
      cState1 &= 0xFF ^ (1 << (swapConfigUp & 0x07));
    }
  }
}

void buttonSwapDown() {
  if (!(swapConfigDown & SWAP_ENABLED)) {
    return;
  }
  boolean downValue = (cState1 & B00100000) == B00100000;
  boolean swapValue = (((swapConfigDown & 0x80) ? cState2 : cState1) >> (swapConfigDown & 0x07)) & 0x01 == 0x01;
  
  if (swapValue) {
    // Set the up bit
    cState1 |= B00100000;
  } else {
    // Clear up bit
    cState1 &= B11011111;
  }
  
  if (downValue) {
    // Set bit
    if (swapConfigUp & 0x80) { // cState2
      cState2 |= 1 << (swapConfigDown & 0x07);
    } else { // cState1
      cState1 |= 1 << (swapConfigDown & 0x07);
    }
  } else {
    // Clear bit
    if (swapConfigUp & 0x80) { // cState2
      cState2 &= 0xFF ^ (1 << (swapConfigDown & 0x07));
    } else { // cState1
      cState1 &= 0xFF ^ (1 << (swapConfigDown & 0x07));
    }
  }
}

void processButtons() {
  applyTurbo();
  buttonSwap();
  buttonSwapDown();
}

void checkConfig() {
  if (cState1 & CONFIG_BUTTON) {
    return; // Select button not pressed, so we do nothing
  }
  PORTB |= B00000010;
  debounce();
  
  writeBlank(); // Write that no buttons are pressed to output
  // wait for next button press
  while (cState1 == 0xFF && cState2 == 0xFF) {
    delayMicroseconds(MACRO_FRAME_DELAY); // wait ~16ms
    readController();
  }
  if (!(cState1 & START_BUTTON)) {
    // Start button pressed
    debounce();
    configTurbo();
  } else if (!(cState1 & DPAD_UP)) {
    // Up pressed, config Up button swap
    debounce();
    swapConfigUp = getButtonSwapConfigValue();
  } else if (!(cState1 & DPAD_DOWN)) {
    debounce();
    swapConfigDown = getButtonSwapConfigValue();
  } else if (!(cState2 & SHOULDER_L)) {
    debounce();
    recordMacro();
  } else if (!(cState2 & SHOULDER_R)) {
    // debounce(); Don't debounce here because we want the macro to start immediatly.
    playBackMacro();
  }
  debounce();
  PORTB &= B11111101;
}

// Wait for all buttons to be released
void debounce() {
  while (cState1 != 0xFF || cState2 != 0xFF) {
    delayMicroseconds(16000); // wait 16ms
    readController();
  }
}

/**

Format: CCCPPPPP


CCC
100 = wait Frame          4
110 = wait half seconds   6
000 = button down     0
001 = button up         1
111 = End macro           7

1000 = wait P = 
110p = wait seconds - (max 30 seconds)
000x = button down
001x = button up
1111 = end of routine
*/


void recordMacro() {
  byte macroPointer = 0;
  byte frameDelay = 0;
  byte secondDelay = 0;
  
  byte lastState1 = 0xFF;
  byte lastState2 = 0xFF;

  do {
    delayMicroseconds(16000);
    readController();
  } while (cState1 == 0xFF && cState2 == 0xFF);
  // wait for first button input
  
  while ((cState1 & CONFIG_BUTTON) && macroPointer < 255) {
    if (cState1 != lastState1 || cState2 != lastState2) {
      if (secondDelay > 0) {
        macroMem[macroPointer++] = delaySec(secondDelay);
        secondDelay = 0;
      }
      if (frameDelay > 0) {
        macroMem[macroPointer++] = delayFrm(frameDelay);
        frameDelay = 0;
      }
      
      // Write button change
      byte diff1 = lastState1 ^ cState1;
      byte diff2 = lastState2 ^ cState2;

      for (int i = 0; i < 8; i++) {
        if ((diff1 >> i) & 0x01) { // bit i in cState1 changed
          macroMem[macroPointer++] = (((cState1 >> i) & 0x01) << 5) + i;
        }
        if ((diff2 >> i) & 0x01) { // bit i in cState2 changed
          macroMem[macroPointer++] = (((cState2 >> i) & 0x01) << 5) + i + 8;
        }
      }
      
      lastState1 = cState1;
      lastState2 = cState2;
    }
    
    // Write the button output, so that we can see what we are recording
    processButtons();
    writeOutput();
    
    delayMicroseconds(MACRO_FRAME_DELAY); // Ajust this bad boy to make it more accurate
    frameDelay++;
    if (frameDelay == 30) {
      frameDelay = 0;
      secondDelay++;
    }
    if (secondDelay == 30) {
      macroMem[macroPointer++] = 0xDE; // Wait 30 seconds
      secondDelay = 0;
    }
    readController();
  }
  
  macroMem[macroPointer] = 0xFF; // Write end of macro command;
}

void playBackMacro() {
  byte macroPointer = 0;
  byte macroState1 = 0xFF;
  byte macroState2 = 0xFF;
  
  while (macroMem[macroPointer] != 0xFF && macroPointer < 255) {
    readController();
    if (!(cState1 & CONFIG_BUTTON)) {
      return; // Config button = cancel playback
    }
    
    boolean buttonsChanged = false;
    byte button;
  
    switch ((macroMem[macroPointer] >> 5) & 0x07) {
      case 0x04: // Wait P frames;
        for (int i = (macroMem[macroPointer] & 0x1F); i > 0; i--) {
          delayMicroseconds(MACRO_FRAME_DELAY);
        }
        break;
      case 0x06: // Wait P seconds / 2 (30 * frames)
        for (int i = (macroMem[macroPointer] & 0x1F); i > 0; i--) {
          for (int j = 0; j < 30; j++) {
            delayMicroseconds(MACRO_FRAME_DELAY);
          }
        }
        break;
      case 0x00: // Button pressed
        button = macroMem[macroPointer] & 0x1F;
        if (button < 8) {
          macroState1 &= ~(1 << button);
        } else {
          macroState2 &= ~(1 << (button - 8));
        }
        buttonsChanged = true;
        break;
      case 0x01: // Button released
        button = macroMem[macroPointer] & 0x1F;
        if (button < 8) {
          macroState1 |= 1 << button;
        } else {
          macroState2 |= 1 << (button - 8);
        }
        buttonsChanged = true;
        break;
    }
    
    if (buttonsChanged) {
      cState1 = macroState1;
      cState2 = macroState2;
      processButtons(); // We still want to apply button swap and auto-fire, since the macro is recorded before this.
      writeOutput();
    }
    
    macroPointer++;
  }
}
