#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int LEFT_BTN = 2;
const int RIGHT_BTN = 3;
const int RESET_BTN = 4;

const int LEFT_LED = 5;
const int RIGHT_LED = 6;

const int BUZZER = 9;

const unsigned long HIDE_TIME = 3000;
const int MAX_ROUNDS = 1;

int leftScore = 0;
int rightScore = 0;
int roundNumber = 0;

unsigned long targetTime = 0;

unsigned long startTime = 0;
unsigned long stateTimer = 0;

unsigned long leftTime = 0;
unsigned long rightTime = 0;

bool leftStopped = false;
bool rightStopped = false;
bool leftReady = false;
bool rightReady = false;
bool bothReady = false;

unsigned long lastLedBlink = 0;
bool ledState = false;

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
  leftStopped = false;
  rightStopped = false;
  leftReady = false;
  rightReady = false;
  bothReady = false;

  leftTime = 0;
  rightTime = 0;

  digitalWrite(LEFT_LED, LOW);
  digitalWrite(RIGHT_LED, LOW);
}

String formatTime(unsigned long ms) {
  unsigned long sec = ms / 1000;
  unsigned long centi = (ms % 1000) / 10;

  char buf[8];
  sprintf(buf, "%02lu.%02lu", sec, centi);

  return String(buf);
}

void showIdleScreen() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("L:");
  if (leftScore < 10) {
    lcd.print(String(" ") + leftScore);
  } else {
    lcd.print(leftScore);
  }

  lcd.setCursor(12, 0);
  lcd.print("R:");

  if (rightScore < 10) {
    lcd.print(String(" ") + rightScore);
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

void generateTarget() {
  targetTime = random(7000, 16001);
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
    lcd.print("DRAW!");
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
          generateTarget();
          roundNumber++;

          lcd.clear();
          lcd.setCursor(0, 0);
          String msg = "Round " + String(roundNumber) + "/" + String(MAX_ROUNDS) + "ready";
          lcd.setCursor(0, 1);

          stateTimer = millis();
          state = READY_CHECK;
        }
        break;
      }

    case READY_CHECK:
      {
        if (!leftReady && digitalRead(LEFT_BTN) == LOW) {
          leftReady = true;
          digitalWrite(LEFT_LED, HIGH);
          beep(1600, 80);
          delay(120);
        }

        if (!rightReady && digitalRead(RIGHT_BTN) == LOW) {
          rightReady = true;
          digitalWrite(RIGHT_LED, HIGH);
          beep(1900, 80);
          delay(120);
        }

        lcd.setCursor(0, 0);
        lcd.print("Round ");
        lcd.print(roundNumber);
        lcd.print("/");
        lcd.print(MAX_ROUNDS);
        lcd.print("  READY");

        lcd.setCursor(0, 1);
        if (leftReady && rightReady) {
          lcd.print("   BOTH READY!   ");
        } else {
          lcd.print("L:");
          lcd.print(leftReady ? "OK " : "-- ");
          lcd.setCursor(12, 1);
          lcd.print("R:");
          lcd.print(rightReady ? "OK " : "-- ");
          lcd.print("   ");
        }

        if (leftReady && rightReady && !bothReady) {
          bothReady = true;
          delay(500);
          stateTimer = millis();
          state = COUNTDOWN;

          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print("Aim: ");
          lcd.print(formatTime(targetTime) + "s");
        }
        break;
      }

    case COUNTDOWN:
      {
        digitalWrite(LEFT_LED, LOW);
        digitalWrite(RIGHT_LED, LOW);
        
        unsigned long elapsed = millis() - stateTimer;

        lcd.setCursor(3, 0);
        lcd.print("Aim: ");
        lcd.print(formatTime(targetTime));
        lcd.print("s   ");

        lcd.setCursor(0, 1);

        if (elapsed < 2000) {
          lcd.print("    Get ready   ");
        } else if (elapsed < 3000) {
          lcd.print("        3       ");
        } else if (elapsed < 4000) {
          lcd.print("        2       ");
        } else if (elapsed < 5000) {
          lcd.print("        1       ");
        } else {
          beep(1500, 150);
          lcd.clear();
          startTime = millis();
          state = RUNNING;
        }

        break;
      }

    case RUNNING:
      {
        unsigned long elapsed = millis() - startTime;
        static unsigned long lastDisplayUpdate = 0;
        static String lastLeftDisplay = "";
        static String lastRightDisplay = "";

        if (!leftStopped && digitalRead(LEFT_BTN) == LOW) {
          leftStopped = true;
          leftTime = elapsed;
          digitalWrite(LEFT_LED, HIGH);
          beep(1600, 80);
          delay(120);
        }

        if (!rightStopped && digitalRead(RIGHT_BTN) == LOW) {
          rightStopped = true;
          rightTime = elapsed;
          digitalWrite(RIGHT_LED, HIGH);
          beep(1900, 80);
          delay(120);
        }

        unsigned long currentTime = millis();
        if (currentTime - lastDisplayUpdate < 50) {
          break;
        }
        lastDisplayUpdate = currentTime;

        String leftDisplay;
        String rightDisplay;

        if (elapsed < HIDE_TIME) {
          if (leftStopped)
            leftDisplay = formatTime(leftTime);
          else
            leftDisplay = formatTime(elapsed);

          if (rightStopped)
            rightDisplay = formatTime(rightTime);
          else
            rightDisplay = formatTime(elapsed);
        } else {
          if (leftStopped)
            leftDisplay = formatTime(leftTime);
          else
            leftDisplay = "??.??";

          if (rightStopped)
            rightDisplay = formatTime(rightTime);
          else
            rightDisplay = "??.??";
        }

        if (leftDisplay != lastLeftDisplay || rightDisplay != lastRightDisplay) {
          lcd.setCursor(0, 0);
          lcd.print("                ");

          lcd.setCursor(0, 0);
          lcd.print(leftDisplay.substring(0, 5));

          lcd.setCursor(11, 0);
          lcd.print(rightDisplay.substring(0, 5));

          lcd.setCursor(3, 1);
          lcd.print("Aim: ");
          lcd.print(formatTime(targetTime));
          lcd.print("s   ");

          lastLeftDisplay = leftDisplay;
          lastRightDisplay = rightDisplay;
        }

        if (leftStopped && rightStopped) {
          unsigned long leftError =
            abs((long)leftTime - (long)targetTime);

          unsigned long rightError =
            abs((long)rightTime - (long)targetTime);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("");
          lcd.print(formatTime(leftTime));
          lcd.setCursor(11, 0);
          lcd.print(formatTime(rightTime));

          lcd.setCursor(0, 1);

          if (leftError < rightError) {
            leftScore++;
            digitalWrite(LEFT_LED, HIGH);
            digitalWrite(RIGHT_LED, LOW);
            lcd.print("   LEFT wins!");
            beep(2200, 250);
          } else if (rightError < leftError) {
            rightScore++;
            digitalWrite(RIGHT_LED, HIGH);
            digitalWrite(LEFT_LED, LOW);
            lcd.print("   RIGHT wins!");
            beep(2200, 250);
          } else {
            lcd.print("DRAW!");
            digitalWrite(LEFT_LED, LOW);
            digitalWrite(RIGHT_LED, LOW);
            beep(1200, 350);
          }

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