#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int LEFT_BTN = 2;
const int RIGHT_BTN = 3;
const int RESET_BTN = 4;

const int LEFT_LED = 5;
const int RIGHT_LED = 6;

const int BUZZER = 9;

const int MAX_ROUNDS = 3;

int leftScore = 0;
int rightScore = 0;
int roundNumber = 0;

int tugPosition = 7;
const int LEFT_EDGE = 0;
const int RIGHT_EDGE = 15;

unsigned long stateTimer = 0;
unsigned long lastLedBlink = 0;
bool ledState = false;

unsigned long lastLeftPress = 0;
unsigned long lastRightPress = 0;
const unsigned long PRESS_COOLDOWN = 60;

bool leftBtnPressed = false;
bool rightBtnPressed = false;

bool roundActive = false;

enum GameState {
  WAIT_START,
  READY_CHECK,
  COUNTDOWN,
  RUNNING,
  RESULT,
  GAME_OVER
};

GameState state = WAIT_START;

void beep(int freq, int dur) {
  tone(BUZZER, freq, dur);
}

void clearRoundData() {
  tugPosition = 7;
  roundActive = false;
  lastLeftPress = 0;
  lastRightPress = 0;
  leftBtnPressed = false;
  rightBtnPressed = false;

  digitalWrite(LEFT_LED, LOW);
  digitalWrite(RIGHT_LED, LOW);
}

void showIdleScreen() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("L:");
  if (leftScore < 10) {
    lcd.print(" ");
    lcd.print(leftScore);
  } else {
    lcd.print(leftScore);
  }

  lcd.setCursor(12, 0);
  lcd.print("R:");
  if (rightScore < 10) {
    lcd.print(" ");
    lcd.print(rightScore);
  } else {
    lcd.print(rightScore);
  }

  lcd.setCursor(0, 1);
  if (roundNumber >= MAX_ROUNDS) {
    lcd.print("   GAME OVER!");
  } else {
    lcd.print("  Press CENTER");
  }
}

void showGameOver() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" ! GAME OVER !");
  lcd.setCursor(0, 1);

  if (leftScore > rightScore) {
    lcd.print("   LEFT WINS!");
    digitalWrite(LEFT_LED, HIGH);
    digitalWrite(RIGHT_LED, LOW);
    beep(2200, 500);
  } else if (rightScore > leftScore) {
    lcd.print("  RIGHT WINS!");
    digitalWrite(RIGHT_LED, HIGH);
    digitalWrite(LEFT_LED, LOW);
    beep(2200, 500);
  } else {
    lcd.print("     DRAW!");
    digitalWrite(LEFT_LED, LOW);
    digitalWrite(RIGHT_LED, LOW);
    beep(1200, 500);
  }

  delay(4000);
  lcd.setCursor(0, 0);
  lcd.print("   Thank you  ");
  lcd.setCursor(0, 1);
  lcd.print("  for playing!");
  delay(3000);
}

void drawTugDisplay() {
  lcd.setCursor(0, 0);

  for (int i = 0; i < 16; i++) {
    if (i == LEFT_EDGE) {
      lcd.print("x");
    } else if (i == RIGHT_EDGE) {
      lcd.print("x");
    } else if (i == tugPosition) {
      lcd.print("0");
    } else {
      lcd.print("-");
    }
  }

  lcd.setCursor(0, 1);
  lcd.print("L:");
  lcd.print(leftScore);
  lcd.setCursor(11, 1);
  lcd.print("R:");
  lcd.print(rightScore);
}

void setup() {
  pinMode(LEFT_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN, INPUT_PULLUP);
  pinMode(RESET_BTN, INPUT_PULLUP);

  pinMode(LEFT_LED, OUTPUT);
  pinMode(RIGHT_LED, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  lcd.init();
  lcd.backlight();

  randomSeed(analogRead(A0));

  Serial.begin(9600);
  showIdleScreen();
  lastLedBlink = millis();
}

void loop() {
  switch (state) {
    case WAIT_START:
      {
        if (roundNumber >= MAX_ROUNDS) {
          state = GAME_OVER;
          break;
        }

        if (millis() - lastLedBlink >= 500) {
          ledState = !ledState;
          lastLedBlink = millis();
        }

        if (ledState) {
          digitalWrite(LEFT_LED, HIGH);
          digitalWrite(RIGHT_LED, LOW);
        } else {
          digitalWrite(LEFT_LED, LOW);
          digitalWrite(RIGHT_LED, HIGH);
        }

        if (digitalRead(RESET_BTN) == LOW) {
          delay(200);

          clearRoundData();
          roundNumber++;

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Round ");
          lcd.print(roundNumber);
          lcd.print("/");
          lcd.print(MAX_ROUNDS);
          lcd.setCursor(0, 1);
          lcd.print("  Get ready...  ");

          stateTimer = millis();
          state = READY_CHECK;
        }
        break;
      }

    case READY_CHECK:
      {
        if (digitalRead(LEFT_BTN) == LOW) {
          digitalWrite(LEFT_LED, HIGH);
          beep(1600, 80);
          delay(120);
          digitalWrite(LEFT_LED, LOW);
          stateTimer = millis();
          state = COUNTDOWN;
        }

        if (digitalRead(RIGHT_BTN) == LOW) {
          digitalWrite(RIGHT_LED, HIGH);
          beep(1900, 80);
          delay(120);
          digitalWrite(RIGHT_LED, LOW);
          stateTimer = millis();
          state = COUNTDOWN;
        }

        lcd.setCursor(0, 0);
        lcd.print("Round ");
        lcd.print(roundNumber);
        lcd.print("/");
        lcd.print(MAX_ROUNDS);
        lcd.setCursor(0, 1);
        lcd.print("Press any button");
        break;
      }

    case COUNTDOWN:
      {
        digitalWrite(LEFT_LED, LOW);
        digitalWrite(RIGHT_LED, LOW);

        unsigned long elapsed = millis() - stateTimer;

        lcd.setCursor(0, 0);
        lcd.print("   TUG OF WAR!  ");
        lcd.setCursor(0, 1);

        if (elapsed < 1000) {
          lcd.print("    Get ready    ");
        } else if (elapsed < 2000) {
          lcd.print("       3       ");
        } else if (elapsed < 3000) {
          lcd.print("       2       ");
        } else if (elapsed < 4000) {
          lcd.print("       1       ");
        } else {
          beep(1500, 150);
          lcd.clear();
          drawTugDisplay();
          roundActive = true;
          state = RUNNING;
        }

        break;
      }

    case RUNNING:
      {
        if (!roundActive) break;

        unsigned long currentMillis = millis();
        bool leftBtnCurrentlyPressed = (digitalRead(LEFT_BTN) == LOW);
        bool rightBtnCurrentlyPressed = (digitalRead(RIGHT_BTN) == LOW);

        if (leftBtnCurrentlyPressed && !leftBtnPressed && (currentMillis - lastLeftPress) > PRESS_COOLDOWN) {
          lastLeftPress = currentMillis;
          leftBtnPressed = true;
          if (tugPosition > LEFT_EDGE + 1) {
            tugPosition--;
            drawTugDisplay();
            beep(1000, 30);
          }
        }
        if (!leftBtnCurrentlyPressed) {
          leftBtnPressed = false;
        }

        if (rightBtnCurrentlyPressed && !rightBtnPressed && (currentMillis - lastRightPress) > PRESS_COOLDOWN) {
          lastRightPress = currentMillis;
          rightBtnPressed = true;
          if (tugPosition < RIGHT_EDGE - 1) {
            tugPosition++;
            drawTugDisplay();
            beep(1200, 30);
          }
        }
        if (!rightBtnCurrentlyPressed) {
          rightBtnPressed = false;
        }

        if (tugPosition <= LEFT_EDGE + 1) {
          roundActive = false;
          leftScore++;
          digitalWrite(LEFT_LED, HIGH);
          digitalWrite(RIGHT_LED, LOW);

          lcd.setCursor(0, 0);
          lcd.print("0--------------x");
          lcd.setCursor(0, 1);
          lcd.print("   LEFT WINS!   ");
          beep(2200, 500);

          stateTimer = millis();
          state = RESULT;
        }

        if (tugPosition >= RIGHT_EDGE - 1) {
          roundActive = false;
          rightScore++;
          digitalWrite(RIGHT_LED, HIGH);
          digitalWrite(LEFT_LED, LOW);

          lcd.setCursor(0, 0);
          lcd.print("x--------------0");
          lcd.setCursor(0, 1);
          lcd.print("   RIGHT WINS! ");
          beep(2200, 500);

          stateTimer = millis();
          state = RESULT;
        }

        break;
      }

    case RESULT:
      {
        if (millis() - stateTimer > 4000) {
          showIdleScreen();
          digitalWrite(LEFT_LED, LOW);
          digitalWrite(RIGHT_LED, LOW);
          state = WAIT_START;
        }
        break;
      }

    case GAME_OVER:
      {
        showGameOver();
        delay(5000);
        leftScore = 0;
        rightScore = 0;
        roundNumber = 0;
        digitalWrite(LEFT_LED, LOW);
        digitalWrite(RIGHT_LED, LOW);
        state = WAIT_START;
        showIdleScreen();
        break;
      }
  }
}