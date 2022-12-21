#include <LiquidCrystal.h>
#include "LedControl.h"
#include <EEPROM.h>

const byte RIGHT = 1;
const byte DOWN = 2;
const byte LEFT = 3;
const byte UP = 0;

const byte dinPin = 12;
const byte clockPin = 11;
const byte loadPin = 10;
const byte matrixSize = 8;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);
byte matrixBrightness = 2;

const byte RS = 4;
const byte enable = 3;
const byte d4 = 7;
const byte d5 = 8;
const byte d6 = 9;
const byte d7 = 13;
const byte backlight = 6;

const byte pinSW = 2;
const byte pinX = A0;
const byte pinY = A1;

const byte pinBuzzer = 5;
const int eatTone = 300;

int programState = 0;
bool recentStateChange = 0;

int menuCursor = 0;
int settingsCursor = 0;

long lastDebounceTime = 0;
int debounceDelay = 50;

byte lastReadState = 1;
byte buttonState = 1;

int refreshDelay = 100;
long lastRefresh = 0;

int minThreshold = 412;
int maxThreshold = 612;
bool joyMoved = false;

byte arrowDown[] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
  B00000
};

byte arrowUp[] = {
  B00000,
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00000
};

byte block[] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
};

LiquidCrystal lcd(RS, enable, d4, d5, d6, d7);

byte orientation = RIGHT;
byte lastOrientation = RIGHT;
byte snakeBody[64][2];
int snakeLength = 2;
float snakeSpeed = 1;
long lastMoved = 0;
bool alive = true;
byte foodLocation[2];
bool ate = false;

long lastBlink = 0;
int blinkDelay = 200;
bool foodOn = false;
int foodLife = 5000;
long foodSpawn = 0;
bool isFood = false;

bool gameOn = false;

int highscore[5];
char names[5][3];
byte difficulty = 1;

int scoreIndex = 0;
int scoreDelay = 3000;
long lastScoreMove = 0;

int lcdBrightness = 8;

int charIndex = 0;
int charAdd = 0;
bool charOn = true;
int charBlinkDelay = 400;

char currentName[3] = {'A', 'A', 'A'};

bool hasSound = true;

byte aboutPage = 0;

int congrats = 0;

void setup() { 
  lcdBrightness = EEPROM.read(10);  
  analogWrite(backlight, (lcdBrightness * 16));

  hasSound = EEPROM.read(11);

  lcd.createChar(1, arrowDown);
  lcd.createChar(2, arrowUp);
  lcd.createChar(3, block);
  lcd.begin(16, 2);
  lcd.print(" --- Snake! --- ");
  lcd.setCursor(0, 1);
  lcd.print("     ^^^^^^     ");
  delay(3000);

  matrixBrightness = EEPROM.read(12);  
  lc.shutdown(0, false);
  lc.setIntensity(0, matrixBrightness);
  lc.clearDisplay(0);

  pinMode(pinSW, INPUT_PULLUP);

  for (int i = 0; i < 5; ++i){
    highscore[i] = EEPROM.read(i);
    int nameEncoded = EEPROM.read(i+5);
    char decode[3];
    decode[2] = (nameEncoded % 10) + 64;
    decode[1] = (nameEncoded / 10 % 10) + 64;
    decode[0] = (nameEncoded / 100 % 10) + 64;    
    names[i][0] = decode[0];
    names[i][1] = decode[1];
    names[i][2] = decode[2];
  }

  randomSeed(micros());
  Serial.begin(9600);
}

void loop() {
    //Serial.println(programState);
    

    if(programState == 0){
      printMenu();
      selectMenu();
      navigateMenu();
      lastScoreMove = millis();
    }
    else if(programState == 1){
      snake();
    }
    else if(programState == 2){
      showScores();
    }
    else if(programState == 3){
      printSettings();
      selectSettings();
      navigateSettings();
    }
    else if(programState == 4){
      about();
    }
    else if(programState == 5){
      howTo();
    }       
    else if(programState == 10){
      selectName();     
    }
    else if(programState == 20){
      selectDifficulty();        
    }
    else if(programState == 30){
      selectBrightness();
    }
    else if(programState == 40){
      setSound();
    }
    else if(programState == 50){
      setMatrix();
    }
    else if(programState == 100){
      congratulation();
    }
}

void printMenu() {
  if(millis() - lastRefresh > refreshDelay)
  {
    if(menuCursor < 2) {      
      lcd.setCursor(0, 0);
      lcd.print(" Start Game    ");
      lcd.write(" ");
      lcd.setCursor(0, 1);
      lcd.print(" Highscore     ");
      lcd.write(1);
    }
    else if(menuCursor < 4) {
      lcd.setCursor(0, 0);
      lcd.print(" Settings      ");
      lcd.write(2);
      lcd.setCursor(0, 1);
      lcd.print(" About         ");
      lcd.write(1);
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print(" How to play   ");
      lcd.write(2);    
      lcd.setCursor(0, 1);
      lcd.print("               ");
      lcd.write(" ");
    }

    if(menuCursor % 2 == 0){
      lcd.setCursor(0, 0);
      lcd.write(">");
      lcd.setCursor(0, 1);
      lcd.write(" ");
    }
    else{
      lcd.setCursor(0, 1);
      lcd.write(">");
      lcd.setCursor(0, 0);
      lcd.write(" ");
    }    
    
    lastRefresh = millis();
  }
}

void navigateMenu() {
  int yValue = analogRead(pinY);

  if(yValue > maxThreshold && menuCursor < 4 && joyMoved == false){
    menuCursor++;
    joyMoved = true;
  }
  else if(yValue < minThreshold && menuCursor > 0 && joyMoved == false){
    menuCursor--;
    joyMoved = true;
  }
  else if(yValue > minThreshold && yValue < maxThreshold){
    joyMoved = false;
  }
}

void selectMenu(){
  int reading = digitalRead(pinSW);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        //Serial.println(programState);
        programState = menuCursor + 1;
        menuCursor = 0;   
        lcd.clear();          
      }      
    }
  }

  lastReadState = reading;
}

void printSettings() {
  if(millis() - lastRefresh > refreshDelay)
  {
    if(settingsCursor < 2) {      
      lcd.setCursor(0, 0);
      lcd.print(" Set Name      ");
      lcd.write(" ");
      lcd.setCursor(0, 1);
      lcd.print(" Difficulty    ");
      lcd.write(1);
    }
    else if(settingsCursor < 4) {
      lcd.setCursor(0, 0);
      lcd.print(" LCD Brightness");
      lcd.write(2);
      lcd.setCursor(0, 1);
      lcd.print(" Sound         ");
      lcd.write(1);
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print(" Matrix Light  ");
      lcd.write(2);    
      lcd.setCursor(0, 1);
      lcd.print(" Back          ");
      lcd.write(" ");
    }

    if(settingsCursor % 2 == 0){
      lcd.setCursor(0, 0);
      lcd.write(">");
      lcd.setCursor(0, 1);
      lcd.write(" ");
    }
    else{
      lcd.setCursor(0, 1);
      lcd.write(">");
      lcd.setCursor(0, 0);
      lcd.write(" ");
    }

    lastRefresh = millis();
  }    
}

void navigateSettings() {
  int yValue = analogRead(pinY);

  if(yValue > maxThreshold && settingsCursor < 5 && joyMoved == false){
    settingsCursor++;
    joyMoved = true;
  }
  else if(yValue < minThreshold && settingsCursor > 0 && joyMoved == false){
    settingsCursor--;
    joyMoved = true;
  }
  else if(yValue > minThreshold && yValue < maxThreshold){
    joyMoved = false;
  }
}

void selectSettings(){
  int reading = digitalRead(pinSW);
  if(reading != lastReadState){
      lastDebounceTime = millis();
  }
  
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState){
      buttonState = reading;

      if(buttonState == 1)
      {
        recentStateChange = 1;
        if(settingsCursor == 5){
          programState = 0;
        }
        else{
          programState = (settingsCursor + 1) * 10;      
        }  
        settingsCursor = 0;  
        lcd.clear();            
      }     
    }   
  }

  lastReadState = reading;
  charIndex = 0;
}

void snake(){
  lcd.setCursor(0, 0);
  lcd.write(currentName[0]);
  lcd.write(currentName[1]);
  lcd.write(currentName[2]);

  lcd.setCursor(7, 0);
  lcd.print("Score:");
  char convertChar[3];
  int score = difficulty * (snakeLength+1);   
  lcd.write(itoa(score, convertChar, 10));
  lcd.print("   ");
  
  lcd.setCursor(0, 1);
  lcd.print("Time played:");
  long seconds = millis() / 1000;
  lcd.write(itoa(seconds, convertChar, 10));
  lcd.print("s  ");
  
  foodBlink();
  int yValue = analogRead(pinY);
  int xValue = analogRead(pinX);

  if(yValue > maxThreshold && joyMoved == false && lastOrientation != UP){
    orientation = DOWN;
    joyMoved = true;
  }
  else if(yValue < minThreshold && joyMoved == false && lastOrientation != DOWN){
    orientation = UP;
    joyMoved = true;
  }
  
  if(xValue > maxThreshold && joyMoved == false && lastOrientation != LEFT){
    orientation = RIGHT;
    joyMoved = true;
  }
  else if(xValue < minThreshold && joyMoved == false && lastOrientation != RIGHT){
    orientation = LEFT;
    joyMoved = true;
  }

  if(yValue > minThreshold && yValue < maxThreshold && xValue > minThreshold && xValue < maxThreshold){
    joyMoved = false;
  }
  
  if (millis() - lastMoved > 1000 / snakeSpeed){
    if(gameOn == false){
      lc.clearDisplay(0);
      lc.setLed(0, 4, 1, true);
      lc.setLed(0, 4, 2, true);
      lc.setLed(0, 4, 3, true);
      lc.setLed(0, 4, 4, true);
      snakeLength = 4;
      
      orientation = RIGHT;
      lastOrientation = RIGHT;

      snakeBody[0][0] = 4;
      snakeBody[0][1] = 0;
      snakeBody[1][0] = 4;
      snakeBody[1][1] = 1;
      snakeBody[2][0] = 4;
      snakeBody[2][1] = 2;
      snakeBody[3][0] = 4;
      snakeBody[3][1] = 3;
      snakeBody[4][0] = 4;
      snakeBody[4][1] = 4;

      foodLocation[0] = -1;
      foodLocation[1] = -1;
      foodLife = 5000;
      isFood = false;
      snakeSpeed = 1;
      lastMoved = 0;
      ate = false;
      gameOn = true;
    }
    if (snakeBody[snakeLength][0] == 7 && orientation == DOWN)
      die();
    else if (snakeBody[snakeLength][0] == 0 && orientation == UP)
      die();
    else if (snakeBody[snakeLength][1] == 7 && orientation == RIGHT)
      die();
    else if (snakeBody[snakeLength][1] == 0 && orientation == LEFT)
      die();
    else
    {
      if (ate == false){
        lc.setLed(0, snakeBody[0][0], snakeBody[0][1], false);
      
        for (int i = 0; i < snakeLength; ++i){
          snakeBody[i][0] = snakeBody[i+1][0];
          snakeBody[i][1] = snakeBody[i+1][1];
        }           
      }
      else{
        snakeLength++;
        snakeBody[snakeLength][0] = snakeBody[snakeLength-1][0];
        snakeBody[snakeLength][1] = snakeBody[snakeLength-1][1];
        snakeSpeed += 0.05 * difficulty;
        ate = false;
        if (snakeLength > 10 && snakeLength < 41)
          foodLife -= 30 * difficulty;               
      }
      // Serial.println(orientation);
      // Serial.println(lastOrientation);

      if (orientation == RIGHT) {
        lastOrientation = orientation;
        snakeBody[snakeLength][1]++;
        lc.setLed(0, snakeBody[snakeLength][0], snakeBody[snakeLength][1], true);
      }
      else if (orientation == LEFT) {
        lastOrientation = orientation;
        snakeBody[snakeLength][1]--;
        lc.setLed(0, snakeBody[snakeLength][0], snakeBody[snakeLength][1], true);
      }
      else if (orientation == UP) {
        lastOrientation = orientation;
        snakeBody[snakeLength][0]--;
        lc.setLed(0, snakeBody[snakeLength][0], snakeBody[snakeLength][1], true);
      }
      else if (orientation == DOWN) {
        lastOrientation = orientation;
        snakeBody[snakeLength][0]++;
        lc.setLed(0, snakeBody[snakeLength][0], snakeBody[snakeLength][1], true);
      }

      for (int i = 0; i < snakeLength-1; ++i){
        if (snakeBody[snakeLength][0] == snakeBody[i][0] && snakeBody[snakeLength][1] == snakeBody[i][1])
          die();
      }     

      if (ate == false && snakeBody[snakeLength][0] == foodLocation[0] && snakeBody[snakeLength][1] == foodLocation[1]){
        if(hasSound)        
          tone(pinBuzzer, eatTone, 30);
        ate = true;
      }

      if (ate == true or lastMoved == 0 or isFood == false) {
        bool searching = true;
        
        while (searching == true) {
          searching = false;
          foodLocation[0] = random(8);
          foodLocation[1] = random(8);
          foodSpawn = millis();
          isFood = true;
          
          for (int i = 0; i < snakeLength; ++i){
            if (foodLocation[0] == snakeBody[i][0] && foodLocation[1] == snakeBody[i][1])
              searching = true;
          }   
        }        
      }
      lastMoved = millis();
    }      
  }
}

void die(){
  if(hasSound){
    tone(pinBuzzer, 300, 100);
    delay(100);
    tone(pinBuzzer, 250, 100);
    delay(100);
    tone(pinBuzzer, 150, 500);    
  }
  
  lastMoved = 0;
  gameOn = false;
  congrats = 0;
  
  int score = difficulty * (snakeLength+1);
  //Serial.println(score);  
  for (int i = 0; i < 5; ++i){
    if (score > highscore[i]){
      congrats = i+1;
      for (int j = 4; j > i; --j){
        highscore[j] = highscore[j-1];
        names[j][0] = names[j-1][0];
        names[j][1] = names[j-1][1];
        names[j][2] = names[j-1][2];        
      }
      highscore[i] = score;
      names[i][0] = currentName[0];
      names[i][1] = currentName[1];
      names[i][2] = currentName[2];
      break;
    }
  }
  for (int i = 0; i < 5; ++i){
    EEPROM.update(i, highscore[i]);
    int nameCode = (names[i][0]-'A'+1) * 100 + (names[i][1]-'A'+1) * 10 + (names[i][2]-'A'+1);
    Serial.println(nameCode);
    EEPROM.update(i+5, nameCode);
  }

  delay(5000);
  if (congrats == 0){
    lc.clearDisplay(0);
    programState = 0;
    menuCursor = 0;   
    lcd.clear(); 
  }
  else {
    programState = 100;  
  }
}

void foodBlink(){
  if (millis() - lastBlink > blinkDelay){
    foodOn = !foodOn;
    //Serial.println(foodOn);
    lc.setLed(0, foodLocation[0], foodLocation[1], foodOn);
    lastBlink = millis();              
  }
  if (millis() - foodSpawn > foodLife && snakeLength > 9){
    lc.setLed(0, foodLocation[0], foodLocation[1], false);    
    foodLocation[0] = -1;
    foodLocation[1] = -1;
    isFood = false;
  }
}

void showScores(){    
  lcd.setCursor(0, 0);
  lcd.print("   Highscores   ");
  lcd.setCursor(0, 1);
  char convertChar[3];    
  lcd.write(itoa(scoreIndex+1, convertChar, 10));
  lcd.setCursor(2, 1);
  lcd.write(itoa(highscore[scoreIndex], convertChar, 10));
  lcd.print("  ");
  lcd.setCursor(6, 1); 
  lcd.write(names[scoreIndex][0]);
  lcd.write(names[scoreIndex][1]);
  lcd.write(names[scoreIndex][2]);
  lcd.setCursor(9, 1);
  lcd.write("       ");    
  
  if (millis() - lastScoreMove > scoreDelay){
    scoreIndex = (scoreIndex + 1) % 5;
    lastScoreMove = millis();
  }
 
  int reading = digitalRead(pinSW);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        //Serial.println(programState);
        scoreIndex = 0;
        programState = 0;
        menuCursor = 0;   
        lcd.clear();          
      }      
    }
  }

  lastReadState = reading;
}

void selectName(){
  lcd.setCursor(0, 0);
  lcd.print("Set Name: ");

  if (millis() - lastBlink > charBlinkDelay){    
    if(charIndex == 0){
      if (charOn == true){
        lcd.write(currentName[0]);
        charOn = false;    
      }
      else{
        lcd.write(3);
        charOn = true;
      }
      lcd.write(currentName[1]);
      lcd.write(currentName[2]);       
    }
    else if(charIndex == 1){
      lcd.write(currentName[0]); 
           
      if (charOn == true){
        lcd.write(currentName[1]);
        charOn = false;    
      }
      else{
        lcd.write(3);
        charOn = true;
      }
      
      lcd.write(currentName[2]);       
    }
    else if(charIndex == 2){
      lcd.write(currentName[0]);
      lcd.write(currentName[1]);

      if (charOn == true){
        lcd.write(currentName[2]);
        charOn = false;    
      }
      else{
        lcd.write(3);
        charOn = true;
      }
             
    }
    lastBlink = millis();
  }

  int yValue = analogRead(pinY);  
  if(yValue > maxThreshold && currentName[charIndex] < 90 && joyMoved == false){
    currentName[charIndex]++;
    joyMoved = true;
  }
  else if(yValue < minThreshold && currentName[charIndex] > 65 && joyMoved == false){
    currentName[charIndex]--;
    joyMoved = true;
  }
  else if(yValue > minThreshold && yValue < maxThreshold){
    joyMoved = false;
  }

  int reading = digitalRead(pinSW);
  //Serial.println(reading);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        charIndex++;
        charOn = true;

        if(charIndex > 2){
          charIndex = 0;
          settingsCursor = 0;
          programState = 3;
          lcd.clear(); 
        }
                 
      }      
    }
  }  
  lastReadState = reading;
}

void selectDifficulty(){
  lcd.setCursor(0, 0);
  lcd.print("Difficulty: ");
  lcd.setCursor(0, 1);
  if (difficulty == 1){
    lcd.write(" ");
    lcd.write(3);
    lcd.write("01");
    lcd.write(3);
    lcd.write("            ");
  }
  else if (difficulty == 2){
    lcd.write("      ");
    lcd.write(3);
    lcd.write("02");
    lcd.write(3);
    lcd.write("      ");
  }
  else if (difficulty == 3){
    lcd.write("           ");
    lcd.write(3);
    lcd.write("03");
    lcd.write(3);
    lcd.write("  ");
  }

  int xValue = analogRead(pinX);  
  if(xValue > maxThreshold && difficulty < 3 && joyMoved == false){
    difficulty++;
    joyMoved = true;
  }
  else if(xValue < minThreshold && difficulty > 1 && joyMoved == false){
    difficulty--;
    joyMoved = true;
  }
  else if(xValue > minThreshold && xValue < maxThreshold){
    joyMoved = false;
  }

  int reading = digitalRead(pinSW);
  //Serial.println(reading);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        settingsCursor = 0;
        programState = 3;
        lcd.clear();    
      }      
    }
  }  
  lastReadState = reading;
}

void selectBrightness(){
  lcd.setCursor(0, 0);
  lcd.print("Brightness: ");
  lcd.setCursor(0, 1);
  for (int i = 0; i <= lcdBrightness; ++i){
    lcd.write(3);    
  }
  lcd.print("                 ");
  analogWrite(backlight, (lcdBrightness * 16));

  int xValue = analogRead(pinX);  
  if(xValue > maxThreshold && lcdBrightness < 15 && joyMoved == false){
    lcdBrightness++;
    joyMoved = true;
  }
  else if(xValue < minThreshold && lcdBrightness > 0 && joyMoved == false){
    lcdBrightness--;
    joyMoved = true;
  }
  else if(xValue > minThreshold && xValue < maxThreshold){
    joyMoved = false;
  }

  int reading = digitalRead(pinSW);
  //Serial.println(reading);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        settingsCursor = 0;
        programState = 3;
        EEPROM.update(10, lcdBrightness);        
        lcd.clear();    
      }      
    }
  }  
  lastReadState = reading;
}

void setSound(){
  lcd.setCursor(0, 0);
  lcd.print("Sound: ");
  if (hasSound){
    lcd.write("ON   ");
  }
  else{
    lcd.write("OFF  ");
  }

  int xValue = analogRead(pinX);  
  if(xValue > maxThreshold && joyMoved == false){
    hasSound = !hasSound;
    joyMoved = true;
  }
  else if(xValue < minThreshold && joyMoved == false){
    hasSound = !hasSound;
    joyMoved = true;
  }
  else if(xValue > minThreshold && xValue < maxThreshold){
    joyMoved = false;
  }  

  int reading = digitalRead(pinSW);
  //Serial.println(reading);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        settingsCursor = 0;
        programState = 3;
        EEPROM.update(11, hasSound);     
        lcd.clear();    
      }      
    }
  }  
  lastReadState = reading;
}

void setMatrix(){
  for (int i = 0; i < 8; ++i){
    for (int j = 0; j < 8; ++j){
      lc.setLed(0, i, j, true);
    }
  }
  lcd.setCursor(0, 0);
  lcd.print("Brightness: ");
  lcd.setCursor(0, 1);
  for (int i = 0; i <= matrixBrightness && i <= 8; ++i){
    lcd.write(3); 
    lcd.write(3);       
  }
  lcd.print("                 ");
  lc.setIntensity(0, matrixBrightness);
  
  int xValue = analogRead(pinX);  
  if(xValue > maxThreshold && matrixBrightness < 8 && joyMoved == false){
    matrixBrightness++;
    joyMoved = true;
  }
  else if(xValue < minThreshold && matrixBrightness > 0 && joyMoved == false){
    matrixBrightness--;
    joyMoved = true;
  }
  else if(xValue > minThreshold && xValue < maxThreshold){
    joyMoved = false;
  }

  int reading = digitalRead(pinSW);
  //Serial.println(reading);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        settingsCursor = 0;
        programState = 3;
        EEPROM.update(12, matrixBrightness);
        lc.clearDisplay(0);               
        lcd.clear();    
      }      
    }
  }  
  lastReadState = reading;
}

void about(){
  if(millis() - lastRefresh > refreshDelay)
  {
    if(aboutPage == 0) {      
      lcd.setCursor(0, 0);
      lcd.print("Title:         ");
      lcd.write(" ");
      lcd.setCursor(0, 1);
      lcd.print("Snake          ");
      lcd.write(1);
    }
    else if(aboutPage == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Creator: Stefan");
      lcd.write(2);
      lcd.setCursor(0, 1);
      lcd.print("Placintescu    ");
      lcd.write(1);
    }
    else if(aboutPage == 2){
      lcd.setCursor(0, 0);
      lcd.print("Github:        ");
      lcd.write(2);    
      lcd.setCursor(0, 1);
      lcd.print("WildCola       ");
      lcd.write(" ");
    }

    lastRefresh = millis();
  }

  int yValue = analogRead(pinY);

  if(yValue > maxThreshold && aboutPage < 2 && joyMoved == false){
    aboutPage++;
    joyMoved = true;
  }
  else if(yValue < minThreshold && aboutPage > 0 && joyMoved == false){
    aboutPage--;
    joyMoved = true;
  }
  else if(yValue > minThreshold && yValue < maxThreshold){
    joyMoved = false;
  }

  int reading = digitalRead(pinSW);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        //Serial.println(programState);
        aboutPage = 0;
        programState = 0;
        menuCursor = 0;   
        lcd.clear();          
      }      
    }
  }
  lastReadState = reading;
}

void howTo(){
  if(millis() - lastRefresh > refreshDelay)
  {
    if(aboutPage == 0) {      
      lcd.setCursor(0, 0);
      lcd.print("Use joystick to ");
      lcd.write(" ");
      lcd.setCursor(0, 1);
      lcd.print("navigate.      ");
      lcd.write(1);
    }
    else if(aboutPage == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Eat apples to  ");
      lcd.write(2);
      lcd.setCursor(0, 1);
      lcd.print("grow length.   ");
      lcd.write(1);
    }
    else if(aboutPage == 2){
      lcd.setCursor(0, 0);
      lcd.print("Avoid walls    ");
      lcd.write(2);    
      lcd.setCursor(0, 1);
      lcd.print("and your tail. ");
      lcd.write(" ");
    }

    lastRefresh = millis();
  }

  int yValue = analogRead(pinY);

  if(yValue > maxThreshold && aboutPage < 2 && joyMoved == false){
    aboutPage++;
    joyMoved = true;
  }
  else if(yValue < minThreshold && aboutPage > 0 && joyMoved == false){
    aboutPage--;
    joyMoved = true;
  }
  else if(yValue > minThreshold && yValue < maxThreshold){
    joyMoved = false;
  }

  int reading = digitalRead(pinSW);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        //Serial.println(programState);
        aboutPage = 0;
        programState = 0;
        menuCursor = 0;   
        lcd.clear();          
      }      
    }
  }
  lastReadState = reading;
}

void congratulation(){
  lcd.setCursor(0, 0);
  lcd.print("Congrats you    ");
  lcd.setCursor(0, 1);
  lcd.print("the no ");
  lcd.write(congrats + '0');
  lcd.print(" score    ");

  int reading = digitalRead(pinSW);
  if(reading != lastReadState){
    lastDebounceTime = millis();
  }
  if(millis() - lastDebounceTime > debounceDelay){
    if(reading != buttonState) {
      buttonState = reading;

      if(buttonState == 1){
        lc.clearDisplay(0);        
        programState = 0;
        menuCursor = 0;   
        lcd.clear();          
      }      
    }
  }
  lastReadState = reading;
}