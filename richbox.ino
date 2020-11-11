#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <EEPROM.h>

const String pass = "1234";
const int cutoff = 100;

int ledPin = 3;
int resistorPin = A0;
int servoPin = 10;
int enablePin = 12;
String msg = "";

int last;
int current;
int maximum;
unsigned long int goal;
unsigned long int money;

bool reachedMax = false;

// Values for median filtering
int vals[7] = {0, 0, 0, 0, 0, 0, 0};
int copy[7] = {0, 0, 0, 0, 0, 0, 0};

int coins[5] = {150, 520, 430, 650, 700};
int coinVals[5] = {10, 20, 50, 100, 200};

LiquidCrystal lcd(1, 2, 4, 5, 6, 7); // Creates an LC object. Parameters: (rs, enable, d4, d5, d6, d7)
SoftwareSerial bt(8,9);
Servo servo;

void(* resetFunc) (void) = 0;

void setup() {
  //Serial.begin(9600);
  pinMode(resistorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  bt.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  printInitialize();
  closeDoor();
  waitForBluetooth();
  if (EEPROM.read(0) == 0) {
    waitForGoal();
  }
  //writeMoney();
  readState();
}

void loop () {  
  analogWrite(ledPin, 255);
  
  if(bt.available()){
    msg = bt.readString();
    bt.println(msg);
  }
  
  readCoin();

  if (msg.equals("open")) {
    openDoor();
  } else if (msg.equals("close")) {
    closeDoor();
  } else if (msg.equals("reset")) {
    clearMemory();
    lcdWrite("Resetting.", "");
    delay(500);
    lcdWrite("Resetting..", "");
    delay(500);
    lcdWrite("Resetting...", "");
    delay(500);
    resetFunc();
  }
  
  msg = "";
}

void readCoin() {
  for (int i = 0; i < 7; i++) {
    vals[i] = vals[i+1];
    copy[i] = vals[i];
  }
  int r = analogRead(resistorPin);
  vals[7] = r;
  copy[7] = r;
  bubbleSort(copy, 7);
  
  current = copy[3];

  //Serial.println(last);
  if (last > current && last > cutoff && !reachedMax) {
    maximum = last;
    reachedMax = true;
    Serial.println(last);
    
    int res = abs(coins[0] - maximum);
    int coinIndex = 0;
    for (int i = 1; i < 5; i++) { 
      if (res > abs(coins[i] - maximum)) {
        coinIndex = i;
      }
      res = min(res, abs(coins[i] - maximum));
    }
    money += coinVals[coinIndex];
    writeMoney();
    readMoney();
    printState();
    checkGoal();
  }
  
  if (current < cutoff && last > current) {
    reachedMax = false;
  }
  
  last = current;
}

void openDoor() {
  servo.attach(servoPin);
  servo.write(0);
  delay(500);
  servo.detach();
}

void checkGoal() {
  if (money >= goal) {
    lcdWrite("Congrats!", "Goal reached!");
    
    openDoor();
  }
}

void closeDoor() {
  servo.attach(servoPin);
  servo.write(90);
  delay(500);
  servo.detach();
}

void lcdWrite(String str1, String str2) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(str1);
  lcd.setCursor(0,1);
  lcd.print(str2);
}

void printInitialize() {
  lcdWrite("Initializing.", "");
  delay(500);
  lcdWrite("Initializing..", "");
  delay(500);
  lcdWrite("Initializing...", "");
  delay(500);
}

void waitForBluetooth() {
  while (true) {
    lcdWrite("Enter password", "with bluetooth.");
    while (!bt.available()) { }
    if (bt.readString() == pass) {
      break;
    }
    lcdWrite("Incorrect key,", "try again.");
    delay(1000);
  }
  lcdWrite("Bluetooth", "connected!");
  delay(1000);
}

void waitForGoal() {
  lcdWrite("Enter goal with", "bluetooth.");
  while (!bt.available()) { }
  goal = bt.parseInt() * 100;
  writeGoal();
}

void writeGoal () {
  int len = 0;
  while (goal > 0) {
    int r = goal % 10;
    len++;
    EEPROM.write(len, r);
    goal /= 10;
  }
  
  EEPROM.write(0, len); 
  lcdWrite("Saving goal.", "");
  delay(500);
  lcdWrite("Saving goal..", "");
  delay(500);
  lcdWrite("Saving goal...", "");
  delay(500);
}

void readMoney() {
  int len = EEPROM.read(256);
  money = 0;
  for (int i = 257; i <= 257 + len; i++) {
    money += 0.5 + EEPROM.read(i) * pow(10, i - 257);
  }
}

void writeMoney() {
  int len = 0;
  while (money > 0) {
    int r = money % 10;
    len++;
    EEPROM.write(len + 256 , r);
    money /= 10;
  }
  
  EEPROM.write(256, len);
}

void readState() {
  readGoal();
  readMoney();
  lcdWrite("Reading state.", "");
  delay(500);
  lcdWrite("Reading state..", "");
  delay(500);
  lcdWrite("Reading state...", "");
  delay(500);
  printState();
  delay(1000);
}

void printState() {
  lcdWrite("Current state: ", String((double) money / 100) + "/" + String(goal / 100) + " GEL");
}

void readGoal() {
  int len = EEPROM.read(0);
  goal = 0;
  for (int i = 1; i <= len; i++) {
    goal += 0.5 + EEPROM.read(i) * pow(10, i - 1);
  }
}

void clearMemory() {
  lcdWrite("Preparing to", "clear memory.");
  delay(500);
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
  lcdWrite("Memory cleared!", "");
  delay(1000);
}

void bubbleSort(int a[], int size) {
    for(int i=0; i<(size-1); i++) {
        for(int o=0; o<(size-(i+1)); o++) {
                if(a[o] > a[o+1]) {
                    int t = a[o];
                    a[o] = a[o+1];
                    a[o+1] = t;
                }
        }
    }
}
