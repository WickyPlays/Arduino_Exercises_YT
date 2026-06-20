#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int LEFT_BTN  = 2;
const int RIGHT_BTN = 3;
const int RESET_BTN = 4;

const int LEFT_LED  = 5;
const int RIGHT_LED = 6;

const int BUZZER    = 9;

const unsigned long HIDE_TIME = 5000;

int leftScore = 0;
int rightScore = 0;

unsigned long targetTime = 0;

unsigned long startTime = 0;
unsigned long stateTimer = 0;

unsigned long leftTime = 0;
unsigned long rightTime = 0;

bool leftStopped = false;
bool rightStopped = false;

enum GameState
{
  WAIT_START,
  COUNTDOWN,
  RUNNING,
  RESULT
};

GameState state = WAIT_START;

void beep(int freq, int dur)
{
  tone(BUZZER, freq, dur);
}

void clearRoundData()
{
  leftStopped = false;
  rightStopped = false;

  leftTime = 0;
  rightTime = 0;

  digitalWrite(LEFT_LED, LOW);
  digitalWrite(RIGHT_LED, LOW);
}

String formatTime(unsigned long ms)
{
  unsigned long sec = ms / 1000;
  unsigned long centi = (ms % 1000) / 10;

  char buf[8];
  sprintf(buf, "%02lu.%02lu", sec, centi);

  return String(buf);
}

void showIdleScreen()
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("L:");
  lcd.print(leftScore);

  lcd.setCursor(12, 0);
  lcd.print("R:");
  lcd.print(rightScore);

  lcd.setCursor(0, 1);
  lcd.print("Press CENTER");
}

void generateTarget()
{
  targetTime = random(1000, 3001) * 10;
}

void setup()
{
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
}

void loop()
{
  switch (state)
  {
    case WAIT_START:
    {
      if (digitalRead(RESET_BTN) == LOW)
      {
        delay(200);

        clearRoundData();
        generateTarget();

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("Target ");
        lcd.print(formatTime(targetTime));

        stateTimer = millis();
        state = COUNTDOWN;
      }

      break;
    }

    case COUNTDOWN:
    {
      unsigned long elapsed = millis() - stateTimer;

      lcd.setCursor(0, 1);

      if (elapsed < 1000)
      {
        lcd.print("      3       ");
      }
      else if (elapsed < 2000)
      {
        lcd.print("      2       ");
      }
      else if (elapsed < 3000)
      {
        lcd.print("      1       ");
      }
      else
      {
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

      if (!leftStopped && digitalRead(LEFT_BTN) == LOW)
      {
        leftStopped = true;
        leftTime = elapsed;

        beep(1600, 80);
        delay(120);
      }

      if (!rightStopped && digitalRead(RIGHT_BTN) == LOW)
      {
        rightStopped = true;
        rightTime = elapsed;

        beep(1900, 80);
        delay(120);
      }

      lcd.setCursor(0, 0);

      String leftDisplay;
      String rightDisplay;

      if (elapsed < HIDE_TIME)
      {
        if (leftStopped)
          leftDisplay = formatTime(leftTime);
        else
          leftDisplay = formatTime(elapsed);

        if (rightStopped)
          rightDisplay = formatTime(rightTime);
        else
          rightDisplay = formatTime(elapsed);
      }
      else
      {
        if (leftStopped)
          leftDisplay = formatTime(leftTime);
        else
          leftDisplay = "??.??";

        if (rightStopped)
          rightDisplay = formatTime(rightTime);
        else
          rightDisplay = "??.??";
      }

      char row[17];
      snprintf(row, sizeof(row),
               "%5s %5s",
               leftDisplay.c_str(),
               rightDisplay.c_str());

      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print(row);

      lcd.setCursor(0, 1);
      lcd.print("Target:");
      lcd.print(formatTime(targetTime));
      lcd.print(" ");

      if (leftStopped && rightStopped)
      {
        unsigned long leftError =
          abs((long)leftTime - (long)targetTime);

        unsigned long rightError =
          abs((long)rightTime - (long)targetTime);

        lcd.clear();
        lcd.setCursor(0, 0);

        char resultLine[17];
        snprintf(resultLine,
                 sizeof(resultLine),
                 "L%s R%s",
                 formatTime(leftTime).c_str(),
                 formatTime(rightTime).c_str());

        lcd.print(resultLine);
        lcd.setCursor(0, 1);

        if (leftError < rightError)
        {
          leftScore++;
          digitalWrite(LEFT_LED, HIGH);
          lcd.print("LEFT WINS");
          beep(2200, 250);
        }
        else if (rightError < leftError)
        {
          rightScore++;
          digitalWrite(RIGHT_LED, HIGH);
          lcd.print("RIGHT WINS");
          beep(2200, 250);
        }
        else
        {
          lcd.print("DRAW");
          beep(1200, 350);
        }

        stateTimer = millis();
        state = RESULT;
      }
      break;
    }

    case RESULT:
    {
      if (millis() - stateTimer > 5000)
      {
        showIdleScreen();
        state = WAIT_START;
      }
      break;
    }
  }
}