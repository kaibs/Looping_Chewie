#include <LiquidCrystal_I2C.h> //I2C Display Bibliothek HEX entweder 0x27 oder 0x3F
#include <Wire.h> //Wire.h Bibliothek einbinden

// Angabe der angeschlossenen Arduino-Pins
LiquidCrystal_I2C lcd(0x3F,16,2); //Hier wird Display 2 als lcd2 benannt, die Adresse angegeben „0x3D“ und die Pinnbelegung durch das I²C Modul angegeben (4,5) sind Analog Ins

int motor_A = 10; //A HIGH und B LOW -> Motor dreht rechts
int motor_B = 9; // A LOW und B HIGH -> Motor dreht links
int motor = 6;
// Used for generating interrupts using CLK signal
const int PinA = 3; 
// Used for reading DT signal
const int PinB = 4;  
// Used for the push button switch
const int PinSW = 8;

long last = 0; //letzter Encoder Wert
long value; //aktueller Encoder Wert
boolean clicked = false; //true wenn encode knopf einmal gedrückt wurde, true gesetzt in encoderRead()
byte randomDirection; //true: vorwärts, false:rückwärts

boolean drehRichtung; // true = links, false = rechts

boolean menu = true;
/***************************************************************************************/
//Geschwindigkeitsvariablen
int maxSpeed = 255;
int minSpeed = 116;
int speedSchritt = 5;      //Schritt, um den Geschwindigkeit bei einem Encoderschritt verändert wird
int aktuelleSpeed = 116;
byte aktuelleSpeedProzent;
byte letzteSpeedProzent;
int randomSpeed;


//Zeitvariablen
int randomDelay;
int randomDuration;
int minDelay = 1500;  //mindestens 0.8s die gleiche Geschwindigkeit
int maxDelay = 8000; //höchstens 4s die gleiche Geschwindigkeit
unsigned long tinit; 
unsigned long tLaufschritt;

const int pauseButton = 8;
//Variablen für Modi
const int modeButton = 7;
int modeState = 1;
byte AnzahlModi = 4;
String modeName;
int displayTimer = 0;
volatile boolean downsizing = false;
volatile boolean biturbo = false;

//char printText[] = "Hallo ich bin eine Nutteee";
char printText1[] = { "Konstante Geschwindigkeit\n" };
char printText2[] = { "Zuf\341llige Geschwindigkeit\n" };
char printText3[] = { "Zufallsgeschwindigkeit mit Pausen\n" };
char printText4[] = { "Wildes Kuddelmuddel\n" };
String subPrintText;

volatile int virtualPosition = 50;

byte laufSchritt=17;
// Variablen für Auswahlmenü
String modi[4] = {"Konstante v     ", "Random v,konst r", "Rand v+p, kons r","Random v,random r"};

// ------------------------------------------------------------------
// FUNKTIONEN   FUNKTIONEN   FUNKTIONEN   FUNKTIONEN   FUNKTIONEN
// ------------------------------------------------------------------

void kaisGeilerScheiss(const char* printText){
  
  Serial.println(String(printText));
//  lcd.setCursor(0, 1);
//  lcd.print("                ");
  Serial.println(String(printText).length());
  for(int k=0; k<16;k++){
    lcd.setCursor(k, 1);
    lcd.print(" ");
  } 
  if(laufSchritt <= String(printText).length()){
    Serial.println(laufSchritt);
   if(laufSchritt > 16){
      lcd.setCursor(0,1);
      subPrintText="";
      for(int p=17; p>1; p--){
        subPrintText += printText[(laufSchritt-p)];
      }
      lcd.print(subPrintText);
      Serial.println(subPrintText);
      laufSchritt++; 
    }else{
    lcd.setCursor((16-laufSchritt),1);
    lcd.print(printText);
    laufSchritt++; 
    }
  }else{
    laufSchritt=17;
  }
  tLaufschritt = millis();
  
}
  
//    for (int printStart = 15; printStart >= 0; printStart--)  //scroll on from right
// {
//   for(int k=0; k<16; k++){
//    lcd.setCursor(k, 1);
//    lcd.print(" ");
//   }
//   lcd.setCursor(printStart,1);
//   lcd.print("Nutt\357\357\357");
//   delay(250);
// }

int buttonRead(){
  if ((!digitalRead(PinSW))) {
        clicked = true;
        menu = false;
    while (!digitalRead(PinSW))
      delay(5);
    Serial.println("Reset");
    }
}
int richtung(){
  if (!drehRichtung) {
      digitalWrite(motor_A, LOW);
      digitalWrite(motor_B, HIGH);
    } else {
      digitalWrite(motor_A, HIGH);
      digitalWrite(motor_B, LOW);
    }
}
int printAktuelleSpeed(){
  aktuelleSpeedProzent = min(100, max(1, aktuelleSpeedProzent));
  lcd.setCursor(0,0);
  lcd.print("max. v: " + String(aktuelleSpeedProzent));
  if((letzteSpeedProzent != aktuelleSpeedProzent) || (aktuelleSpeedProzent == 1)){
    if(aktuelleSpeedProzent < 10){
      lcd.setCursor(9,0);
      lcd.print("%   ");
    }
    if(aktuelleSpeedProzent >= 10){
      lcd.setCursor(10,0);
      lcd.print("% ");
    } 
    if(aktuelleSpeedProzent == 100){
      lcd.setCursor(11,0);
      lcd.print("%");
    }
  letzteSpeedProzent = aktuelleSpeedProzent;
  }
  
}

//Modus mit konstanter Geschwindigkeit, nur vorwärts
int modus1(){
  clicked = false;
  while(!clicked){  
  if(biturbo){
    if(aktuelleSpeed<maxSpeed){
    aktuelleSpeed += speedSchritt;
    }else{
      aktuelleSpeed= maxSpeed;
    }
    biturbo=false;
  }
  if(downsizing && aktuelleSpeed>minSpeed){
    aktuelleSpeed -= speedSchritt;
    downsizing=false;
  }
  
  aktuelleSpeedProzent= (int) ((aktuelleSpeed-minSpeed)/1.39);
  printAktuelleSpeed();
  if((millis()-tLaufschritt)>350){
    kaisGeilerScheiss(printText1);
  }  
  analogWrite(motor, aktuelleSpeed);
  Serial.println(clicked);
  buttonRead();
  if(clicked){
    menu = true;
    aktuelleSpeed = 1;
    analogWrite(motor, 0);
    break;
  }
 }
}

//Random Geschwindigkeit, nur vorwärts, keine Pausen
void modus2(){
  clicked = false;
  while(!clicked){  
  randomSpeed = random(minSpeed,aktuelleSpeed);
  randomDelay = random(minDelay,maxDelay);
  tinit = millis();
  Serial.println(millis()-tinit);
  while((millis()-tinit) < randomDelay){
    lcd.setCursor(0,0);
    aktuelleSpeedProzent= (int) ((aktuelleSpeed-minSpeed)/1.39);
    printAktuelleSpeed();
    analogWrite(motor, randomSpeed);
    buttonRead();
    speedAnpassung();
    if(clicked){
      menu = true;
      aktuelleSpeed = 1;
      analogWrite(motor, 0);
      break;
    }
  }
  buttonRead();
    speedAnpassung();
    if(clicked){
      menu = true;
      aktuelleSpeed = 1;
      analogWrite(motor, 0);
      break;
    }
  }
}
// Zufällige Geschwindigkeit, nur vorwärts, zufällige Pausen
int modus3(){
  clicked = false;
  while(!clicked){  
  randomSpeed = random(minSpeed,aktuelleSpeed);
  randomDuration = random(minDelay,maxDelay);
  randomDelay = random(1000,3000);
  tinit = millis();
  Serial.println(millis()-tinit);
  while((millis()-tinit) < randomDuration){
    lcd.setCursor(0,0);
    aktuelleSpeedProzent= (int) ((aktuelleSpeed-minSpeed)/1.39);
    printAktuelleSpeed();
    analogWrite(motor, randomSpeed);
    buttonRead();
    speedAnpassung();
    if(clicked){
      menu = true;
      aktuelleSpeed = 1;
      analogWrite(motor, 0);
      break;
    }
  }
  // Pause nach Fahrt abwarten
  tinit = millis();
  while((millis()-tinit) < randomDelay){
    lcd.setCursor(0,0);
    aktuelleSpeedProzent= (int) ((aktuelleSpeed-minSpeed)/1.39);
    printAktuelleSpeed();
    analogWrite(motor, 0);
    buttonRead();
    speedAnpassung();
    if(clicked){
      menu = true;
      aktuelleSpeed = 1;
      analogWrite(motor, 0);
      break;
    }
  }
  
    speedAnpassung();
    if(clicked){
      menu = true;
      aktuelleSpeed = 1;
      analogWrite(motor, 0);
      break;
    }
  }
}
// Zufällige Geschwindigkeit, beide Richtungen, zufällige Pausen
int modus4(){
  clicked = false;
  while(!clicked){  
  randomSpeed = random(minSpeed,aktuelleSpeed);
  randomDuration = random(minDelay,maxDelay);
  randomDelay = random(400,2000);
  randomDirection = random(0, 10);
  tinit = millis();
  Serial.println(millis()-tinit);
  while((millis()-tinit) < randomDuration){
    lcd.setCursor(0,0);
    aktuelleSpeedProzent= (int) ((aktuelleSpeed-minSpeed)/1.39);
    printAktuelleSpeed();
       
    if(randomDirection < 6){
      drehRichtung = true;
      richtung();
    }else{
      drehRichtung = false;
      richtung();
    }
    analogWrite(motor, randomSpeed);
    buttonRead();
    speedAnpassung();
    if(clicked){
      menu = true;
      aktuelleSpeed = 1;
      analogWrite(motor, 0);
      break;
    }
  }
  // Pause nach Fahrt abwarten
  tinit = millis();
  while((millis()-tinit) < randomDelay){
    lcd.setCursor(0,0);
    aktuelleSpeedProzent= (int) ((aktuelleSpeed-minSpeed)/1.39);
    printAktuelleSpeed();
    analogWrite(motor, 0);
    buttonRead();
    speedAnpassung();
    if(clicked){
      menu = true;
      aktuelleSpeed = 1;
      analogWrite(motor, 0);
      break;
    }
  }
  
    speedAnpassung();
    if(clicked){
      menu = true;
      aktuelleSpeed = 1;
      analogWrite(motor, 0);
      break;
    }
  }
}




void speedAnpassung(){
  if(biturbo){
      if(aktuelleSpeed<maxSpeed){
      aktuelleSpeed += speedSchritt;
      }else{
        aktuelleSpeed= maxSpeed;
      }
      biturbo=false;
    }
    if(downsizing && aktuelleSpeed>minSpeed){
      aktuelleSpeed -= speedSchritt;
      downsizing=false;
    }
}




// ------------------------------------------------------------------
// INTERRUPT     INTERRUPT     INTERRUPT     INTERRUPT     INTERRUPT 
// ------------------------------------------------------------------
void isr ()  {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  // If interrupts come faster than 5ms, assume it's a bounce and ignore
  if (interruptTime - lastInterruptTime > 5) {
    if (digitalRead(PinB) == LOW)
    {
      virtualPosition-- ; // Could be -5 or -10
      biturbo = true;
    }
    else {
      virtualPosition++ ; // Could be +5 or +10
      downsizing = true;
    }

    // Restrict value from 0 to +100
    virtualPosition = min(100, max(0, virtualPosition));

    // Keep track of when we were here last (no more than every 5ms)
    lastInterruptTime = interruptTime;
  }
}


// ------------------------------------------------------------------
// SETUP    SETUP    SETUP    SETUP    SETUP    SETUP    SETUP    
// ------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  //I2C Einstellungen
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);

  pinMode(PinA, INPUT);
  pinMode(PinB, INPUT);
  pinMode(PinSW, INPUT_PULLUP);
  pinMode(motor_A, OUTPUT);
  pinMode(motor_B, OUTPUT);
  

  // Nachricht am Display ausgeben  
  lcd.print("Hallo Hure!");
  delay(1000);
  // Attach the routine to service the interrupts
  attachInterrupt(digitalPinToInterrupt(PinA), isr, LOW);
}

// ------------------------------------------------------------------
// MAIN LOOP     MAIN LOOP     MAIN LOOP     MAIN LOOP     MAIN LOOP
// ------------------------------------------------------------------
void loop() {
  buttonRead();
  Serial.println(virtualPosition);
  while(menu){
    lcd.setCursor(0, 0);
    lcd.print("Moduswahl:  ");
    lcd.setCursor(0, 1);
    
//    lcd.print(String(modi[modeState-1]));
    switch(modeState){
      case 1:
        if((millis()-tLaufschritt)>350){
           kaisGeilerScheiss(printText1);
        }  
        break;
      case 2:
        if((millis()-tLaufschritt)>350){
           kaisGeilerScheiss(printText2);
        }  
        break;
      case 3:
       if((millis()-tLaufschritt)>350){
           kaisGeilerScheiss(printText3);
        }  
        break;  
      case 4:
        if((millis()-tLaufschritt)>350){
           kaisGeilerScheiss(printText4);
        }  
        break; 
      default:
        break;
    }
//    lcd.setCursor(0, 13);
//    lcd.print(modeState);

    if(downsizing){
      if(modeState > 1){
        modeState--;
      }else{
        modeState = AnzahlModi;
      }
      downsizing = false;
      Serial.println(modeState);
    }
    if(biturbo){
      if(modeState < AnzahlModi){
        modeState++;
      }else{
        modeState = 1;
      }
      biturbo = false;
      
      Serial.println(modeState);
    }
    buttonRead();
  }
  if(clicked){    
    aktuelleSpeed = minSpeed;
    lcd.clear();
    
    switch(modeState){
    case 1:
      drehRichtung = true;
      richtung();
      modus1();
      break;
    case 2:
      drehRichtung = true;
      modus2();
      break;
    case 3:
      drehRichtung = true;
      modus3();
      break;
    case 4:
      modus4();
      break;
   }
   
  }
}
      

