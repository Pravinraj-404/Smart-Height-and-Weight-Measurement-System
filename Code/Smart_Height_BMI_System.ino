#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ---------------- PINS ----------------
#define TRIG_PIN 5
#define ECHO_PIN 18
#define DF_RX 26
#define DF_TX 25
#define POWER_BUTTON 23

#define GREEN_LED 2
#define YELLOW_LED 16
#define RED_LED 19

LiquidCrystal_I2C lcd(0x27, 16, 2);
HardwareSerial mySerial(2);
DFRobotDFPlayerMini player;

// ---------------- KEYPAD ----------------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {32, 33, 4, 17};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------------- VARIABLES ----------------
bool systemOn = true;
bool lastPowerState = HIGH;
bool modeSelected = false;
bool bmiMode = false;

String weightInput = "";
bool decimalUsed = false;
float sensorHeight=212.7;

// =========================================
// AUDIO WAIT
// =========================================
void playAndWait(int fileNumber) {
player.playMp3Folder(fileNumber);
while (true) {
if (player.available()) {
if (player.readType() == DFPlayerPlayFinished) break;
}
}
delay(100);
}

// =========================================

void setup() {

pinMode(TRIG_PIN, OUTPUT);
pinMode(ECHO_PIN, INPUT);
pinMode(POWER_BUTTON, INPUT_PULLUP);

pinMode(GREEN_LED, OUTPUT);
pinMode(YELLOW_LED, OUTPUT);
pinMode(RED_LED, OUTPUT);

Wire.begin(21,22);
lcd.init();
lcd.backlight();

mySerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);
delay(2000);
player.begin(mySerial);
player.volume(25);

startSystem();
}

// =========================================

void loop() {

handlePowerButton();
if (!systemOn) return;

if (!modeSelected)
handleModeSelection();
else if (bmiMode)
handleWeightInput();
else
measureHeightOnly();
}

// =========================================
// POWER CONTROL
// =========================================

void handlePowerButton() {

bool currentState = digitalRead(POWER_BUTTON);

if (lastPowerState == HIGH && currentState == LOW) {
delay(200);
systemOn = !systemOn;

if (systemOn) startSystem();  
else shutdownSystem();

}

lastPowerState = currentState;
}

void shutdownSystem() {

player.stop();

digitalWrite(GREEN_LED, LOW);
digitalWrite(YELLOW_LED, LOW);
digitalWrite(RED_LED, LOW);

lcd.clear();
lcd.noBacklight();

modeSelected = false;
bmiMode = false;
weightInput = "";
decimalUsed = false;
}

// =========================================
// START SYSTEM
// =========================================

void startSystem() {

lcd.init();
lcd.backlight();
lcd.clear();

digitalWrite(GREEN_LED, LOW);
digitalWrite(YELLOW_LED, LOW);
digitalWrite(RED_LED, LOW);

modeSelected = false;
bmiMode = false;
weightInput = "";
decimalUsed = false;

lcd.setCursor(4,0);
lcd.print("Welcome");
playAndWait(4);

lcd.clear();
lcd.setCursor(0,0);
lcd.print("1:Height  2:BMI");
playAndWait(9);
}

// =========================================
// MODE SELECTION
// =========================================

void handleModeSelection() {

char key = keypad.getKey();
if (!key) return;

if (key == '1') {
bmiMode = false;
modeSelected = true;
}

if (key == '2') {
bmiMode = true;
modeSelected = true;

lcd.clear();  
lcd.print("Enter Weight kg/:");  
playAndWait(5);

}
}

// =========================================
// HEIGHT ONLY
// =========================================

void measureHeightOnly() {

lcd.clear();
lcd.print("Stand Steady");
playAndWait(6);

float currentDistance = measureHeight();
float height = sensorHeight - currentDistance;

lcd.clear();
lcd.print("Height:");
lcd.setCursor(0,1);
lcd.print(height,2);
lcd.print(" cm");

speakHeight(height);

modeSelected = false;
}

// =========================================
// WEIGHT INPUT
// =========================================

void handleWeightInput() {

char key = keypad.getKey();
if (!key) return;

if (key >= '0' && key <= '9') {
weightInput += key;
speakDigit(key - '0');
displayInput();
}

else if (key == '*' && !decimalUsed) {
weightInput += '.';
decimalUsed = true;
playAndWait(2);
displayInput();
}

else if (key == 'B' && weightInput.length() > 0) {
if (weightInput.endsWith(".")) decimalUsed = false;
weightInput.remove(weightInput.length() - 1);
displayInput();
}

else if (key == 'C') {
weightInput = "";
decimalUsed = false;
displayInput();
}

else if (key == 'A' && weightInput.length() > 0) {

float weight = weightInput.toFloat();  

lcd.clear();  
lcd.print("Stand Steady");  
playAndWait(6);  

float currentDistance = measureHeight();
float height = sensorHeight - currentDistance;
lcd.clear();  
lcd.print("Height:");  
lcd.setCursor(0,1);  
lcd.print(height,2);  
lcd.print(" cm");  

speakHeight(height);  

calculateBMI(weight, height);  

weightInput = "";  
decimalUsed = false;

}
}

// =========================================
// BMI CALCULATION
// =========================================

void calculateBMI(float weight, float heightCm) {

lcd.clear();
lcd.print("Calculating...");
playAndWait(7);

float bmi = weight / ((heightCm / 100.0) * (heightCm / 100.0));

lcd.clear();
lcd.print("BMI:");
lcd.setCursor(0,1);
lcd.print(bmi,2);

playAndWait(8);
speakFloat(bmi);

classifyBMI(bmi);
}

// =========================================
// CATEGORY DISPLAY + LED
// =========================================

void classifyBMI(float bmi) {

digitalWrite(GREEN_LED, LOW);
digitalWrite(YELLOW_LED, LOW);
digitalWrite(RED_LED, LOW);

lcd.clear();

// -------- NORMAL --------
if (bmi >= 18.5 && bmi <= 24.9) {

lcd.setCursor(2,0);  
lcd.print("Your BMI is");  

lcd.setCursor(5,1);  
lcd.print("NORMAL");  

blinkDuringAudio(20, GREEN_LED);  
blinkDuringAudio(24, GREEN_LED);  
blinkDuringAudio(25, GREEN_LED);  

digitalWrite(GREEN_LED, HIGH);

}

// -------- OVERWEIGHT --------
else if (bmi >= 25 && bmi <= 29.9) {

lcd.setCursor(3,0);  
lcd.print("OVERWEIGHT");  

lcd.setCursor(2,1);  
lcd.print("Exercise More");  

blinkDuringAudio(26, YELLOW_LED);  
blinkDuringAudio(27, YELLOW_LED);  
blinkDuringAudio(28, YELLOW_LED);  

digitalWrite(YELLOW_LED, HIGH);

}

// -------- UNDERWEIGHT --------
else if (bmi < 18.5) {

lcd.setCursor(2,0);  
lcd.print("UNDERWEIGHT");  

lcd.setCursor(2,1);  
lcd.print("Improve Diet");  

blinkDuringAudio(29, RED_LED);  
blinkDuringAudio(30, RED_LED);  

digitalWrite(RED_LED, HIGH);

}

// -------- OBESE --------
else {

lcd.setCursor(5,0);  
lcd.print("OBESE");  

lcd.setCursor(1,1);  
lcd.print("Consult Doctor");  

blinkDuringAudio(31, RED_LED);  
blinkDuringAudio(32, RED_LED);  

digitalWrite(RED_LED, HIGH);

}

modeSelected = false;
}

// =========================================
// LED BLINK
// =========================================

void blinkDuringAudio(int fileNumber, int ledPin) {

player.playMp3Folder(fileNumber);

while (true) {
if (player.available()) {
if (player.readType() == DFPlayerPlayFinished) break;
}

digitalWrite(ledPin, HIGH);  
delay(250);  
digitalWrite(ledPin, LOW);  
delay(250);

}
}

// =========================================
// HEIGHT MEASURE
// =========================================

float measureHeight() {

delay(3000);

const int samples = 20;
float readings[samples];

for(int i=0;i<samples;i++){
digitalWrite(TRIG_PIN, LOW);
delayMicroseconds(2);
digitalWrite(TRIG_PIN, HIGH);
delayMicroseconds(10);
digitalWrite(TRIG_PIN, LOW);

long duration = pulseIn(ECHO_PIN, HIGH, 30000);  
readings[i] = duration * 0.0343 / 2;  
delay(350);

}

for(int i=0;i<samples-1;i++)
for(int j=0;j<samples-1-i;j++)
if(readings[j] > readings[j+1]){
float t = readings[j];
readings[j] = readings[j+1];
readings[j+1] = t;
}

float sum = 0;
for(int i=8;i<=11;i++)
sum += readings[i];

return sum / 4.0;
}

// =========================================
// SPEAK
// =========================================

void speakHeight(float height){
playAndWait(1);
speakFloat(height);
playAndWait(3);
}

void speakFloat(float value){
String text = String(value,2);
for(int i=0;i<text.length();i++){
char c = text.charAt(i);
if(c == '.')
playAndWait(2);
else
speakDigit(c - '0');
}
}

void speakDigit(int digit){
playAndWait(10 + digit);
}

void displayInput(){
lcd.clear();
lcd.print("Enter Weight:");
lcd.setCursor(0,1);
lcd.print(weightInput);
}