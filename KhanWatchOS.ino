// KhanWatchOs 1.0
// KhanWatchOs is a combination and improvement of several arduino programs for
// the TinyDuino TinyScreen
// Main code developed by TinyScreen
// https://codebender.cc/sketch:115936#TinyScreen_SmartWatch_iOS.ino
// Flappy Bird design developed by TinyScreen
// https://codebender.cc/sketch:77043#TinyScreen_FlappyBirds.ino
// Thanks to all for the code, I hope you like my improvements and new features

#define menu_debug_print true

#include <Wire.h>
#include <SPI.h>
#include <TinyScreen.h>
#include <EEPROM.h>
#include <Time.h>
TinyScreen display = TinyScreen(0);

#define BLACK 0x00
#define BLUE 0xE0
#define RED 0x03
#define GREEN 0x1C
#define DGREEN 0x0C
#define YELLOW 0x1F
#define WHITE 0xFF
#define ALPHA 0xFE
#define BROWN 0x32

uint8_t defaultFontColor = WHITE;
uint8_t defaultFontBG = BLACK;
uint8_t inactiveFontColor = RED;
uint8_t inactiveFontBG = BLACK;

uint8_t topBarHeight = 10;
uint8_t timeY = 14;
uint8_t menuTextY[4] = {12, 25, 38, 51};

uint8_t ble_connection_state = 0;
uint8_t ble_connection_displayed_state = 1;

unsigned long batteryUpdateInterval = 10000;
unsigned long lastBatteryUpdate = 0;

unsigned long sleepTimer = 0;
int sleepTimeout = 5000;

uint8_t lastSecondDisplayed = -1;
uint8_t lastDisplayedDay = -1;

uint8_t displayOn = 0;
uint8_t buttonReleased = 1;
uint8_t amtNotifications = 0;

int brightness = 0;
uint8_t lastSetBrightness = 100;

const FONT_INFO& font8pt = liberationSansNarrow_8ptFontInfo;
const FONT_INFO& font10pt = liberationSansNarrow_10ptFontInfo;
const FONT_INFO& font22pt = liberationSansNarrow_22ptFontInfo;

void setup(void) {
  setTime(0, 0, 0, 19, 12, 2015);
  Serial.begin(115200);
  Wire.begin();
  display.begin();
  display.setFlip(1);
  initHomeScreen();

  // If things get really crazy, uncomment this line. It wipes the saved EEPROM
  // information for the Nordic chip. Good to do this if the services.h file
  // gets updated.
  // After it is wiped, comment and reupload.
  // eepromWrite(0, 0xFF);
  requestScreenOn();
}

void loop() {
  if (displayOn) {
    updateDateDisplay();
    updateTimeDisplay();
    updateMainDisplay();
  }
  checkButtons();
  if (millis() > sleepTimer + (unsigned long)sleepTimeout) {
    if (displayOn) {
      displayOn = 0;
      display.off();
    }
  }
}

int requestScreenOn() {
  sleepTimer = millis();
  if (!displayOn) {
    displayOn = 1;
    displayBattery();
    display.on();
    return 1;
  }
  return 0;
}

void checkButtons() {
  byte buttons = display.getButtons();
  if (buttonReleased && buttons) {
    if (displayOn) buttonPress(buttons);
    requestScreenOn();
    buttonReleased = 0;
  }
  if (!buttonReleased && !(buttons & 0x0F)) {
    buttonReleased = 1;
  }
}

const uint8_t displayStateHome = 0x01;
const uint8_t displayStateMenu = 0x02;
const uint8_t displayStateEditor = 0x03;

uint8_t currentDisplayState = displayStateHome;

void (*menuHandler)(uint8_t) = NULL;
uint8_t (*editorHandler)(uint8_t, int*, PGM_P, void (*)()) = NULL;

typedef struct {
  const uint8_t amtLines;
  const char* const* strings;
  void (*selectionHandler)(uint8_t);
} menu_info;

const uint8_t upButton = 0x01;
const uint8_t flappybird = 0x01;
const uint8_t downButton = 0x02;
const uint8_t selectButton = 0x04;
const uint8_t backButton = 0x08;
const uint8_t menuButton = 0x04;
const uint8_t viewButton = 0x02;
const uint8_t clearButton = 0x02;

void buttonPress(uint8_t buttons) {
  if (currentDisplayState == displayStateHome) {
    if (buttons == viewButton) {
      menuHandler = viewNotifications;
      menuHandler(0);
    } else if (buttons == menuButton) {
      menuHandler = viewMenu;
      menuHandler(0);
    } else if (buttons == flappybird) {
      display.setBrightness(1);
      while (true) {
        menuHandler = flappybirds;
        menuHandler(0);
        delay(5);
        if (menuHandler == viewMenu) {
          initHomeScreen();
          break;
        }
      }
    } else if (buttons == backButton) {
      menuHandler = phraseBook;
      menuHandler(0);
    }
  } else if (currentDisplayState == displayStateMenu) {
    if (menuHandler) {
      menuHandler(buttons);
    }
  } else if (currentDisplayState == displayStateEditor) {
    if (editorHandler) {
      editorHandler(buttons, 0, 0, NULL);
    }
  }
}

void phraseBook(uint8_t button) {
  if (!button) {
    currentDisplayState = displayStateMenu;
    display.clearWindow(0, 12, 96, 64);
    display.setCursor(1, menuTextY[0]);
    display.setCursor(0, menuTextY[1]);
    display.setCursor(56, menuTextY[3]);
    display.print("Back >");
    display.setCursor(0, menuTextY[0]);
    display.print("Important #");
    display.setCursor(40, menuTextY[1]);
    display.print("123-456-7890");
  }
}

void viewNotifications(uint8_t button) {
  if (!button) {
    currentDisplayState = displayStateMenu;
    display.clearWindow(0, 12, 96, 64);
    display.setCursor(1, menuTextY[0]);
    display.setCursor(0, menuTextY[1]);
    display.setCursor(56, menuTextY[3]);
    display.print("Back >");
    display.setCursor(0, menuTextY[0]);
    if (weekday() == 1) {
      display.print("Sun Act.");
    }
    if (weekday() == 2) {
      display.print("Mon Act.");
    }
    if (weekday() == 3) {
      display.print("Tue Act.");
    }
    if (weekday() == 4) {
      display.print("Wen Act.");
    }
    if (weekday() == 5) {
      display.print("Thur Act.");
    }
    if (weekday() == 6) {
      display.print("Fri Act.");
    }
    if (weekday() == 7) {
      display.print("Sat Act.");
    }
  }
  if (button == clearButton) {
    amtNotifications = 0;
    currentDisplayState = displayStateHome;
    initHomeScreen();
  }
}

uint8_t menuHistory[5];
uint8_t menuHistoryIndex = 0;
uint8_t currentMenu = 0;
uint8_t currentMenuLine = 0;
uint8_t lastMenuLine = -1;
uint8_t currentSelectionLine = 0;
uint8_t lastSelectionLine = -1;

void newMenu(char newIndex) {
  currentMenuLine = 0;
  lastMenuLine = -1;
  currentSelectionLine = 0;
  lastSelectionLine = -1;
  if (newIndex >= 0) {
    menuHistory[menuHistoryIndex++] = currentMenu;
    currentMenu = newIndex;
  } else {
    if (currentDisplayState == displayStateMenu) {
      menuHistoryIndex--;
      currentMenu = menuHistory[menuHistoryIndex];
    }
  }
  if (menuHistoryIndex) {
    currentDisplayState = displayStateMenu;
    if (menu_debug_print) Serial.print("New menu index ");
    if (menu_debug_print) Serial.println(currentMenu);
  } else {
    if (menu_debug_print) Serial.print("New menu index ");
    if (menu_debug_print) Serial.println("home");
    currentDisplayState = displayStateHome;
    initHomeScreen();
  }
}

static const char PROGMEM mainMenuStrings0[] = "Set date/time";
static const char PROGMEM mainMenuStrings1[] = "Set auto off";
static const char PROGMEM mainMenuStrings2[] = "Set brightness";

static const char* const PROGMEM mainMenuStrings[] = {
    mainMenuStrings0, mainMenuStrings1, mainMenuStrings2,
};

const menu_info mainMenuInfo = {
    3, mainMenuStrings, mainMenu,
};

static const char PROGMEM dateTimeMenuStrings0[] = "Set Year";
static const char PROGMEM dateTimeMenuStrings1[] = "Set Month";
static const char PROGMEM dateTimeMenuStrings2[] = "Set Day";
static const char PROGMEM dateTimeMenuStrings3[] = "Set Hour";
static const char PROGMEM dateTimeMenuStrings4[] = "Set Minute";
static const char PROGMEM dateTimeMenuStrings5[] = "Set Second";

static const char* const PROGMEM dateTimeMenuStrings[] = {
    dateTimeMenuStrings0, dateTimeMenuStrings1, dateTimeMenuStrings2,
    dateTimeMenuStrings3, dateTimeMenuStrings4, dateTimeMenuStrings5,
};

const menu_info dateTimeMenuInfo = {
    6, dateTimeMenuStrings, dateTimeMenu,
};

const menu_info menuList[] = {mainMenuInfo, dateTimeMenuInfo};
#define mainMenuIndex 0
#define dateTimeMenuIndex 1

int currentVal = 0;
int digits[4];
int currentDigit = 0;
int maxDigit = 4;
int* originalVal;
void (*editIntCallBack)() = NULL;

uint8_t editInt(uint8_t button, int* inVal, PGM_P intName, void (*cb)()) {
  if (menu_debug_print) Serial.println("editInt");
  if (!button) {
    if (menu_debug_print) Serial.println("editIntInit");
    editIntCallBack = cb;
    currentDisplayState = displayStateEditor;
    editorHandler = editInt;
    currentDigit = 0;
    originalVal = inVal;
    currentVal = *originalVal;
    digits[3] = currentVal % 10;
    currentVal /= 10;
    digits[2] = currentVal % 10;
    currentVal /= 10;
    digits[1] = currentVal % 10;
    currentVal /= 10;
    digits[0] = currentVal % 10;
    currentVal = *originalVal;
    display.clearWindow(0, 12, 96, 64);
    display.setCursor(0, menuTextY[0]);
    display.print("< back/undo");
    display.setCursor(90, menuTextY[0]);
    display.print('^');
    display.setCursor(10, menuTextY[1]);
    displayPrint_P(intName);
    display.setCursor(0, menuTextY[3]);
    display.print("< next/save");
    display.setCursor(90, menuTextY[3]);
    display.print('v');
  } else if (button == upButton) {
    if (digits[currentDigit] < 9) digits[currentDigit]++;
  } else if (button == downButton) {
    if (digits[currentDigit] > 0) digits[currentDigit]--;
  } else if (button == selectButton) {
    if (currentDigit < maxDigit - 1) {
      currentDigit++;
    } else {
      // save
      int newValue = (digits[3]) + (digits[2] * 10) + (digits[1] * 100) +
                     (digits[0] * 1000);
      *originalVal = newValue;
      viewMenu(backButton);
      if (editIntCallBack) {
        editIntCallBack();
        editIntCallBack = NULL;
      }
      return 1;
    }
  } else if (button == backButton) {
    if (currentDigit > 0) {
      currentDigit--;
    } else {
      if (menu_debug_print) Serial.println("<back");
      viewMenu(backButton);
      return 0;
    }
  }
  display.setCursor(10, menuTextY[2]);
  for (uint8_t i = 0; i < 4; i++) {
    if (i != currentDigit) display.fontColor(BLUE, BLACK);
    display.print(digits[i]);
    if (i != currentDigit) display.fontColor(WHITE, BLACK);
  }
  display.print("   ");
  return 0;
}

void mainMenu(uint8_t selection) {
  if (menu_debug_print) Serial.println("mainMenuHandler");
  if (selection == 0) {
    newMenu(dateTimeMenuIndex);
  }
  if (selection == 1) {
    editInt(0, &sleepTimeout, menuList[currentMenu].strings[selection], NULL);
  }
  if (selection == 2) {
    editInt(0, &brightness, menuList[currentMenu].strings[selection], NULL);
  }
}

uint8_t dateTimeSelection = 0;
int dateTimeVariable = 0;

void saveChangeCallback() {
  int timeData[] = {year(), month(), day(), hour(), minute(), second()};
  timeData[dateTimeSelection] = dateTimeVariable;
  setTime(timeData[3], timeData[4], timeData[5], timeData[2], timeData[1],
          timeData[0]);
  if (menu_debug_print) Serial.print("set time ");
  if (menu_debug_print) Serial.println(dateTimeVariable);
}

void dateTimeMenu(uint8_t selection) {
  if (menu_debug_print) Serial.print("dateTimeMenu ");
  if (menu_debug_print) Serial.println(selection);
  if (selection >= 0 && selection < 6) {
    int timeData[] = {year(), month(), day(), hour(), minute(), second()};
    dateTimeVariable = timeData[selection];
    dateTimeSelection = selection;
    editInt(0, &dateTimeVariable, menuList[currentMenu].strings[selection],
            saveChangeCallback);
  }
}

void displayPrint_P(PGM_P str) {
  for (uint8_t c; (c = pgm_read_byte(str)); str++) display.write(c);
}

void viewMenu(uint8_t button) {
  if (menu_debug_print) Serial.print("viewMenu ");
  if (menu_debug_print) Serial.println(button);
  if (!button) {
    newMenu(mainMenuIndex);
    display.clearWindow(0, 12, 96, 64);
  } else {
    if (button == upButton) {
      if (currentSelectionLine > 0) {
        currentSelectionLine--;
      } else if (currentMenuLine > 0) {
        currentMenuLine--;
      }
    } else if (button == downButton) {
      if (currentSelectionLine < menuList[currentMenu].amtLines - 1 &&
          currentSelectionLine < 3) {
        currentSelectionLine++;
      } else if (currentSelectionLine + currentMenuLine <
                 menuList[currentMenu].amtLines - 1) {
        currentMenuLine++;
      }
    } else if (button == selectButton) {
      if (menu_debug_print) Serial.print("select ");
      if (menu_debug_print)
        Serial.println(currentMenuLine + currentSelectionLine);
      menuList[currentMenu].selectionHandler(currentMenuLine +
                                             currentSelectionLine);
    } else if (button == backButton) {
      newMenu(-1);
      if (!menuHistoryIndex) return;
    }
  }
  display.setFont(font10pt);
  if (lastMenuLine != currentMenuLine ||
      lastSelectionLine != currentSelectionLine) {
    if (menu_debug_print) Serial.println("drawing menu ");
    if (menu_debug_print) Serial.println(currentMenu);
    for (int i = 0; i < 4; i++) {
      display.setCursor(7, menuTextY[i]);
      if (i == currentSelectionLine) {
        display.fontColor(defaultFontColor, inactiveFontBG);
      } else {
        display.fontColor(inactiveFontColor, inactiveFontBG);
      }
      if (currentMenuLine + i < menuList[currentMenu].amtLines)
        displayPrint_P(menuList[currentMenu].strings[currentMenuLine + i]);
      for (uint8_t i = 0; i < 25; i++) display.write(' ');
      if (i == 0) {
        display.fontColor(defaultFontColor, inactiveFontBG);
        display.setCursor(0, menuTextY[0]);
        display.print('<');
        display.setCursor(90, menuTextY[0]);
        display.print('^');
      }
      if (i == 3) {
        display.fontColor(defaultFontColor, inactiveFontBG);
        display.setCursor(0, menuTextY[3]);
        display.print('>');
        display.setCursor(90, menuTextY[3]);
        display.print('v');
      }
    }
    display.fontColor(defaultFontColor, inactiveFontBG);
    lastMenuLine = currentMenuLine;
    lastSelectionLine = currentSelectionLine;
  }
}

void initHomeScreen() {
  display.clearWindow(0, 0, 0, 0);
  updateMainDisplay();
}

void updateDateDisplay() {
  if (lastDisplayedDay == day()) return;
  lastDisplayedDay = day();
  display.setFont(font8pt);
  display.setCursor(2, 1);
  display.print(dayShortStr(weekday()));
  display.print(' ');
  display.print(month());
  display.print('/');
  display.print(day());
  display.print("  ");
}

void updateMainDisplay() {
  if (lastSetBrightness != brightness) {
    display.setBrightness(brightness);
    lastSetBrightness = brightness;
  }
  if (currentDisplayState == displayStateHome) {
    updateTimeDisplay();
    lastDisplayedDay = day();
    display.setFont(font8pt);
    display.setCursor(2, 1);
    display.print(dayShortStr(weekday()));
    display.print(' ');
    display.print(month());
    display.print('/');
    display.print(day());
    display.print("  ");
    displayBattery();
    display.setCursor(0, menuTextY[3]);
    display.print(F("< Menu    View >"));
  }
}

void updateTimeDisplay() {
  if (currentDisplayState != displayStateHome ||
      lastSecondDisplayed == second())
    return;
  lastSecondDisplayed = second();
  display.setFont(font22pt);
  char displayX = 0;
  int PM = 0;
  int h = hour(), m = minute(), s = second();
  if (h > 12) {
    PM = 1;
    h -= 12;
  }
  display.setCursor(displayX, timeY);
  if (h < 10) display.print('0');
  display.print(h);
  display.print(':');
  if (m < 10) display.print('0');
  display.print(m);
  display.print(':');
  if (s < 10) display.print('0');
  display.print(s);
  display.print(' ');
  if (PM) display.fontColor(inactiveFontColor, inactiveFontBG);
  display.setFont(font10pt);
  display.setCursor(displayX + 79, timeY - 1);
  display.print("AM");
  if (PM) {
    display.fontColor(defaultFontColor, defaultFontBG);
  } else {
    display.fontColor(inactiveFontColor, inactiveFontBG);
  }
  display.setCursor(displayX + 79, timeY + 11);
  display.print("PM");
  display.fontColor(defaultFontColor, defaultFontBG);
}

void displayBattery() {
  // http://forum.arduino.cc/index.php?topic=133907.0
  const long InternalReferenceVoltage = 1100L;
  ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << ADLAR) | (1 << MUX3) |
          (1 << MUX2) | (1 << MUX1) | (0 << MUX0);
  delay(10);
  ADCSRA |= _BV(ADSC);
  while (((ADCSRA & (1 << ADSC)) != 0))
    ;
  int result = (((InternalReferenceVoltage * 1024L) / ADC) + 5L) / 10L;
  // Serial.println(result);
  // if(result>440){//probably charging
  uint8_t charging = false;
  if (result > 450) {
    charging = true;
  }
  result = constrain(result - 300, 0, 120);
  uint8_t x = 70;
  uint8_t y = 3;
  uint8_t height = 5;
  uint8_t length = 20;
  uint8_t amtActive = (result * length) / 120;
  uint8_t red, green, blue;
  display.drawLine(x - 1, y, x - 1, y + height, 0xFF);      // left boarder
  display.drawLine(x - 1, y - 1, x + length, y - 1, 0xFF);  // top border
  display.drawLine(x - 1, y + height + 1, x + length, y + height + 1,
                   0xFF);  // bottom border
  display.drawLine(x + length, y - 1, x + length, y + height + 1,
                   0xFF);  // right border
  display.drawLine(x + length + 1, y + 2, x + length + 1, y + height - 2,
                   0xFF);  // right border
  for (uint8_t i = 0; i < length; i++) {
    if (i < amtActive) {
      red = 63 - ((63 / length) * i);
      green = ((63 / length) * i);
      blue = 0;
    } else {
      red = 32;
      green = 32;
      blue = 32;
    }
    display.drawLine(x + i, y, x + i, y + height, red, green, blue);
  }
}

/*
void ram (void) {
  extern int __heap_start, *__brkval;
  int v;
  Serial.print(F("[free SRAM] "));
  Serial.print((int) &v - (__brkval == 0 ? (int) &__heap_start : (int)
__brkval));
  Serial.println(F(" bytes"));
}
*/

unsigned char flappyBird[] = {
    ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  BLACK,  BLACK,  BLACK,
    BLACK,  BLACK,  BLACK,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA,
    ALPHA,  ALPHA,  ALPHA,  BLACK,  BLACK,  WHITE,  WHITE,  WHITE,  BLACK,
    WHITE,  WHITE,  BLACK,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA,
    ALPHA,  BLACK,  WHITE,  WHITE,  YELLOW, YELLOW, BLACK,  WHITE,  WHITE,
    WHITE,  WHITE,  BLACK,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  BLACK,
    WHITE,  YELLOW, YELLOW, YELLOW, YELLOW, BLACK,  WHITE,  WHITE,  WHITE,
    BLACK,  WHITE,  BLACK,  ALPHA,  ALPHA,  ALPHA,  BLACK,  YELLOW, YELLOW,
    YELLOW, YELLOW, YELLOW, YELLOW, BLACK,  WHITE,  WHITE,  WHITE,  BLACK,
    WHITE,  BLACK,  ALPHA,  ALPHA,  ALPHA,  BLACK,  YELLOW, YELLOW, YELLOW,
    YELLOW, YELLOW, YELLOW, YELLOW, BLACK,  WHITE,  WHITE,  WHITE,  WHITE,
    BLACK,  ALPHA,  ALPHA,  ALPHA,  BLACK,  YELLOW, YELLOW, YELLOW, YELLOW,
    YELLOW, YELLOW, YELLOW, YELLOW, BLACK,  BLACK,  BLACK,  BLACK,  BLACK,
    BLACK,  ALPHA,  ALPHA,  BLACK,  YELLOW, YELLOW, YELLOW, YELLOW, YELLOW,
    YELLOW, YELLOW, BLACK,  RED,    RED,    RED,    RED,    RED,    RED,
    BLACK,  ALPHA,  ALPHA,  BLACK,  YELLOW, YELLOW, YELLOW, YELLOW, YELLOW,
    YELLOW, BLACK,  RED,    BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  BLACK,
    ALPHA,  ALPHA,  BLACK,  YELLOW, YELLOW, YELLOW, YELLOW, YELLOW, YELLOW,
    BLACK,  RED,    RED,    RED,    RED,    RED,    BLACK,  ALPHA,  ALPHA,
    ALPHA,  ALPHA,  BLACK,  BLACK,  YELLOW, YELLOW, YELLOW, YELLOW, YELLOW,
    BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  ALPHA,  ALPHA,  ALPHA,  ALPHA,
    ALPHA,  ALPHA,  ALPHA,  BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  ALPHA,
    ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA,  ALPHA};
// 7x8
unsigned char wingUp[] = {
    ALPHA,  BLACK,  BLACK,  BLACK, BLACK, ALPHA, ALPHA,  BLACK, WHITE, WHITE,
    WHITE,  WHITE,  BLACK,  ALPHA, BLACK, WHITE, WHITE,  WHITE, WHITE, WHITE,
    BLACK,  BLACK,  YELLOW, WHITE, WHITE, WHITE, YELLOW, BLACK, ALPHA, BLACK,
    YELLOW, YELLOW, YELLOW, BLACK, ALPHA, ALPHA, ALPHA,  BLACK, BLACK, BLACK,
    ALPHA,  ALPHA,  ALPHA,  ALPHA, ALPHA, ALPHA, ALPHA,  ALPHA, ALPHA, ALPHA,
    ALPHA,  ALPHA,  ALPHA,  ALPHA, ALPHA, ALPHA,
};
unsigned char wingMid[] = {
    ALPHA, ALPHA, ALPHA, ALPHA,  ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA,
    ALPHA, ALPHA, ALPHA, ALPHA,  ALPHA, BLACK, BLACK, BLACK, BLACK, BLACK,
    ALPHA, BLACK, WHITE, WHITE,  WHITE, WHITE, WHITE, BLACK, BLACK, YELLOW,
    WHITE, WHITE, WHITE, YELLOW, BLACK, ALPHA, BLACK, BLACK, BLACK, BLACK,
    BLACK, ALPHA, ALPHA, ALPHA,  ALPHA, ALPHA, ALPHA, ALPHA, ALPHA, ALPHA,
    ALPHA, ALPHA, ALPHA, ALPHA,  ALPHA, ALPHA,
};
unsigned char wingDown[] = {
    ALPHA, ALPHA, ALPHA, ALPHA,  ALPHA, ALPHA,  ALPHA, ALPHA, ALPHA, ALPHA,
    ALPHA, ALPHA, ALPHA, ALPHA,  ALPHA, ALPHA,  ALPHA, ALPHA, ALPHA, ALPHA,
    ALPHA, ALPHA, BLACK, BLACK,  BLACK, BLACK,  BLACK, ALPHA, BLACK, YELLOW,
    WHITE, WHITE, WHITE, YELLOW, BLACK, BLACK,  WHITE, WHITE, WHITE, WHITE,
    BLACK, ALPHA, BLACK, WHITE,  WHITE, YELLOW, BLACK, ALPHA, ALPHA, ALPHA,
    BLACK, BLACK, BLACK, BLACK,  ALPHA, ALPHA,
};
char wingHeight = 8;
char wingWidth = 7;
uint8_t highScore = 0;
char flappyBirdX = 25;
char flappyBirdY = 30;
char flappyBirdHeight = 12;
char flappyBirdWidth = 17;
char amtPipes = 4;
char PipeHeight[] = {10, 15, 20, 25, 25, 10, 10};
char pipeOffsetX = 0;
const char pipeWidth = 10;
const char pipeSpacingX = 26;
const char pipeSpacingY = 26;
unsigned char lineBuffer[16 + 96 + (pipeSpacingX + pipeWidth)];
unsigned char BGcolor = BLUE;
unsigned char pipeColor = GREEN;
char wingPos = 1;
char cloudY[] = {5, 15, 30, 25, 5, 0};
char cloudOffsetX = 0;
const char cloudWidth = 15;
const char cloudSpacingX = 30;
unsigned char cloudColor = WHITE;
char groundY = 52;
char groundOffsetX = 0;
char birdYmod = 1;
unsigned long framecount = 0;
char brightnessChanged = 0;
char movePipe = 4;
char movePipeMod = 1;
uint8_t score = 0;

void setBuffer(char i, char amt, unsigned char color) {
  char endbyte = i + amt;
  while (i < endbyte) lineBuffer[i++] = color;
}

void updateDisplay() {
  display.clearWindow(0, 12, 96, 64);
  display.goTo(0, 0);
  display.startData();
  for (unsigned char y = 0; y < groundY; y++) {
    for (unsigned char i = 16; i < 112; i++) lineBuffer[i] = BGcolor;

    int x = cloudOffsetX;
    char cloud = 0;
    while (x < 16 + 96) {
      if (y > cloudY[cloud] && y < cloudY[cloud] + 8) {
        if (y < cloudY[cloud] + 2 || y > cloudY[cloud] + 6)
          setBuffer(x + 1, cloudWidth - 2, cloudColor);
        else
          setBuffer(x, cloudWidth, cloudColor);
      }
      cloud++;
      x += (cloudSpacingX + cloudWidth);
    }

    x = pipeOffsetX;
    char pipe = 0;
    while (x < 16 + 96) {
      if (y < PipeHeight[pipe] || y > PipeHeight[pipe] + pipeSpacingY) {
        if (y < PipeHeight[pipe] - 4 || y > PipeHeight[pipe] + pipeSpacingY + 4)
          setBuffer(x, pipeWidth, pipeColor);
        else
          setBuffer(x - 1, pipeWidth + 2, pipeColor);
      }
      pipe++;
      x += (pipeSpacingX + pipeWidth);
    }

    if (y >= flappyBirdY && y < flappyBirdY + flappyBirdHeight) {
      int offset = (y - flappyBirdY) * flappyBirdWidth;
      for (int i = 0; i < flappyBirdWidth; i++) {
        unsigned char color = flappyBird[offset + i];
        if (color != ALPHA) lineBuffer[16 + flappyBirdX + i] = color;
      }
    }
    char wingY = flappyBirdY + 3;
    if (y >= wingY && y < wingY + wingHeight) {
      int offset = (y - wingY) * wingWidth;
      for (int i = 0; i < wingWidth; i++) {
        unsigned char color;
        if (wingPos == 0) color = wingUp[offset + i];
        if (wingPos == 1) color = wingMid[offset + i];
        if (wingPos == 2) color = wingDown[offset + i];
        if (color != ALPHA) lineBuffer[16 + flappyBirdX + i] = color;
      }
    }

    char highScoreString[3];
    highScoreString[0] = (highScore / 10) + '0';
    highScoreString[1] = (highScore % 10) + '0';
    highScoreString[2] = '\0';

    putString(y, 80, 1, highScoreString, liberationSans_10ptFontInfo);

    char scoreString[3];
    scoreString[0] = (score / 10) + '0';
    scoreString[1] = (score % 10) + '0';
    scoreString[2] = '\0';

    putString(y, 60, 15, scoreString, liberationSans_16ptFontInfo);

    display.writeBuffer(lineBuffer + 16, 96);
  }
  for (int z = 0; z < 21; z++) {
    lineBuffer[16 + (z * 5)] = GREEN;
    lineBuffer[17 + (z * 5)] = GREEN;
    lineBuffer[18 + (z * 5)] = GREEN;
    lineBuffer[19 + (z * 5)] = DGREEN;
    lineBuffer[20 + (z * 5)] = DGREEN;
  }
  display.writeBuffer(lineBuffer + 16 + groundOffsetX, 96);
  display.writeBuffer(lineBuffer + 17 + groundOffsetX, 96);
  display.writeBuffer(lineBuffer + 18 + groundOffsetX, 96);
  display.writeBuffer(lineBuffer + 19 + groundOffsetX, 96);
  display.writeBuffer(lineBuffer + 20 + groundOffsetX, 96);

  for (unsigned char i = 16; i < 112; i++) lineBuffer[i] = BROWN;
  for (unsigned char i = 0; i < 7; i++)
    display.writeBuffer(lineBuffer + 16, 96);

  display.endTransfer();
}

void putString(uint8_t y, uint8_t fontX, uint8_t fontY, char* string,
               const FONT_INFO& fontInfo) {
  const FONT_CHAR_INFO* fontDescriptor = fontInfo.charDesc;
  uint8_t fontHeight = fontInfo.height;
  if (y >= fontY && y < fontY + fontHeight) {
    const unsigned char* fontBitmap = fontInfo.bitmap;
    uint8_t fontFirstCh = fontInfo.startCh;
    uint8_t fontLastCh = fontInfo.endCh;
    // if(!_fontFirstCh)return 1;
    // if(ch<_fontFirstCh || ch>_fontLastCh)return 1;
    // if(_cursorX>xMax || _cursorY>yMax)return 1;
    uint8_t stringChar = 0;
    uint8_t ch = string[stringChar++];
    while (ch) {
      uint8_t chWidth = pgm_read_byte(&fontDescriptor[ch - fontFirstCh].width);
      uint8_t bytesPerRow = chWidth / 8;
      if (chWidth > bytesPerRow * 8) bytesPerRow++;
      uint16_t offset =
          pgm_read_word(&fontDescriptor[ch - fontFirstCh].offset) +
          (bytesPerRow * fontHeight) - 1;

      for (uint8_t byte = 0; byte < bytesPerRow; byte++) {
        uint8_t data = pgm_read_byte(fontBitmap + offset - (y - fontY) -
                                     ((bytesPerRow - byte - 1) * fontHeight));
        uint8_t bits = byte * 8;
        for (uint8_t i = 0; i < 8 && (bits + i) < chWidth; i++) {
          if (data & (0x80 >> i)) {
            lineBuffer[16 + fontX] = 0;
          } else {
            // SPDR=_fontBGcolor;
          }
          fontX++;
        }
      }
      fontX += 2;
      ch = string[stringChar++];
    }
  }
}

byte flip = 0;
void flappybirds(uint8_t button) {
  unsigned long timer = micros();
  updateDisplay();
  timer = micros() - timer;
  Serial.println(timer);

  if (pipeOffsetX < 18 && pipeOffsetX > 6) {
    if (flappyBirdY < (PipeHeight[1]) ||
        flappyBirdY > (PipeHeight[1] + pipeSpacingY - 13)) {
      if (score == 0) {
        pipeColor = GREEN;
        if (pipeOffsetX == 7) score++;
      } else if (score > 0) {
        pipeColor = RED;
        delay(1000);
        menuHandler = viewMenu;
        if (score > highScore) highScore = score;
        score = 0;
      }
    } else {
      pipeColor = GREEN;
      if (pipeOffsetX == 7) score++;
    }
  } else {
    pipeColor = GREEN;
  }

  framecount++;

  if (display.getButtons()) {
    wingPos = (framecount) % 3;
    flappyBirdY -= (birdYmod * 2);
    if (flappyBirdY < 0) flappyBirdY = 0;
  } else {
    wingPos = (framecount >> 2) % 3;
    flappyBirdY += birdYmod;
    if (flappyBirdY > 40) flappyBirdY = 40;
  }

  pipeOffsetX -= 1;

  if (movePipe && movePipe < 5) {
    PipeHeight[movePipe - 1] += movePipeMod;
    if (PipeHeight[movePipe - 1] < 1) movePipeMod = abs(movePipeMod);
    if (PipeHeight[movePipe - 1] > groundY - 3 - pipeSpacingY)
      movePipeMod = -1 * abs(movePipeMod);
  }
  if (pipeOffsetX <= 0) {
    pipeOffsetX = (pipeSpacingX + pipeWidth);
    PipeHeight[0] = PipeHeight[1];
    PipeHeight[1] = PipeHeight[2];
    PipeHeight[2] = PipeHeight[3];
    PipeHeight[3] = PipeHeight[4];
    PipeHeight[4] = 3 + micros() % (groundY - 8 - pipeSpacingY);
    if (movePipe)
      movePipe -= 1;
    else
      movePipe = 3 + micros() % 5;
  }

  groundOffsetX += 1;
  if (groundOffsetX >= 5) groundOffsetX = 0;

  if (!(framecount % 2)) cloudOffsetX--;
  if (cloudOffsetX <= 0) {
    cloudOffsetX = cloudSpacingX + cloudWidth;
    cloudY[0] = cloudY[1];
    cloudY[1] = cloudY[2];
    cloudY[2] = cloudY[3];
    cloudY[3] = cloudY[4];
    cloudY[4] = cloudY[5];
    cloudY[5] = micros() % (30);
  }
}
