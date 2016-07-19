/*
Softqware based on the code made by Richard Visokey AD7C - www.ad7c.com
*/

// Include the library code
#include <LiquidCrystal.h>
#include <rotary.h>
#include <EEPROM.h>
#include <OneButton.h>

//Setup some items
#define W_CLK 8   // Pin 8 - connect to AD9850 module word load clock pin (CLK)
#define FQ_UD 9   // Pin 9 - connect to frequency update pin (FQ)
#define DATA 10   // Pin 10 - connect to serial data load pin (DATA)
#define RESET 11  // Pin 11 - connect to reset pin (RST) 
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

#define INITIAL_FREQUENCY_SET 100000
#define LOWER_FREQUENCY_LIMIT 1000 
#define UPPER_FREQUENCY_LIMIT 31000000
#define VOLTAGE_PIN A0
#define EXTERNAL_VOLTAGE_REFERENCE 3.3  //voltage connected as AREF 
#define AC_RESOLUTION 1024

#define intervalInMilisToDoThingsMoreSlower   1000 
#define intervalInMilisToDoThingsLittleSlower  500

#define SET_MSEC_FOR_ONE_CLICK 200

#define SEPARATOR ','

Rotary r = Rotary(2,3); // sets the pins the rotary encoder uses. TODO: check if you really need pins with ext interrupts. 
LiquidCrystal lcd(12, 13, 7, 6, 5, 4);
volatile unsigned long freqencyValue=INITIAL_FREQUENCY_SET; // Starting frequency of VFO
volatile unsigned long freqencyValuePrev=1; // variable to hold the updated frequency
volatile unsigned long increment; // starting VFO update increment in HZ.

boolean buttonstate; 
String hertz;
int  hertzPosition;
byte ones,tens,hundreds,thousands,tenthousands,hundredthousands,millions ;  //Placeholders

String frequency; // string to hold the frequency

unsigned long timepassed = millis(); // int to hold the arduino miilis since startup
 
int voltagePin = A0; // pin to measure voltage
unsigned int voltageValue = 0;
float calculatedVoltageValue;
long previousMillisForSlow = 0;   //to slowdown sending data to serial interface
long previousMillisForLittleSlow =0;

// Setup a new OneButton on pin A4.  
OneButton button(A4, true);

// Setup a new OneButton on pin A3.  
OneButton buttonTwo(A5, true); //TODO: cannot check what port I used, check that

//functions 
void readVoltage(){
  voltageValue = analogRead(voltagePin);
  calculateVoltageValue();
  // wait 2 milliseconds before the next loop
  // for the analog-to-digital converter to settle after the last reading:
  delay(2);
}

float getCalculatedVoltageValue(){
  return calculatedVoltageValue;
}

void showVoltage(){
  showOnLcd();
}

void showOnLcd(){
  lcd.setCursor(0,1); 
  lcdPrintFloat(calculatedVoltageValue,3);     
}

void calculateVoltageValue(){
  //5 volts / 1024 units or, .0049 volts (4.9 mV) per unit
 // print the results to the serial monitor:
  calculatedVoltageValue=voltageValue*EXTERNAL_VOLTAGE_REFERENCE/AC_RESOLUTION;
}

void sendSampleAsText(){
    Serial.print(freqencyValue);
    Serial.print(SEPARATOR);
    serialPrintFloat(getCalculatedVoltageValue(),3);
    Serial.println(); 
}


void testSomething(){
  scanFrequencyRange(1000.0, 29000000.0, 1000); //
}

void scanFrequencyRange(long minFrequency, long maxFrequency, int frequencyStep){
  long storeCurrentValue = freqencyValue;
  for(freqencyValue=minFrequency;freqencyValue<=maxFrequency;freqencyValue=freqencyValue+frequencyStep){
    sendfrequency(freqencyValue);
   readVoltage();
    sendSampleAsText();
  }
  
//  for(int k=1;k<8;k++){
//    for (int i=1; i<=10;i++){ 
//       unsigned long z=pow(10,k)*i;
//        Serial.println(long(z));   
//    }
//  }
  delay(10000);
  
  
  freqencyValue=storeCurrentValue;
  //1,2,3,4,5,6,7,8,9 //100,1000,10000,100000,1000000,1
  //10,20,30,40,50,60,70,80,90
  //100,200,300,400,500,600,700,800,900
  //1000,2000,3000,4000,5000,6000,7000,8000,9000
  //1*10,1*100,1*1000,1*10000,1*100000,1*1000000
}
/*
void scanFrequencyRange(long minFrequency, long maxFrequency, int frequencyStep){
  long storeCurrentValue = freqencyValue;
  for(freqencyValue=minFrequency;freqencyValue<=maxFrequency;freqencyValue=freqencyValue+frequencyStep){
    sendfrequency(freqencyValue);
    readVoltage();
    sendSampleAsText();
  }
  freqencyValue=storeCurrentValue;
}
*/

void doHereThingsOncePerInterval(){
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillisForSlow > intervalInMilisToDoThingsMoreSlower) {
    sendSampleAsText(); 
    previousMillisForSlow = currentMillis;
  }
  if(currentMillis - previousMillisForLittleSlow > intervalInMilisToDoThingsLittleSlower){
      readVoltage();
       showVoltage(); 
       previousMillisForLittleSlow = currentMillis;
  }
}

void setIncrementToHigh(){
  increment=100000;
  setincrement();  
}

void doIfFrequencyWasChanged(){
  if (freqencyValue != freqencyValuePrev){    
        showFrequency();
        sendfrequency(freqencyValue);
        freqencyValuePrev = freqencyValue;
      }
}

void setup() {
  //One button will have 3 modes. Click to change frequency, dClick to set frequeny faster
  //press to store current frequency in memory
  // link the myClickFunction function to be called on a click event.   
  button.attachClick(setincrement);
  // link the doubleclick function to be called on a doubleclick event.   
  button.attachDoubleClick(setIncrementToHigh);
  //link to press
  
  //button.attachPress(storeMEM);
  button.attachPress(testSomething);
  
  button.setClickTicks(SET_MSEC_FOR_ONE_CLICK);
  
  buttonTwo.attachPress(storeMEM);
  buttonTwo.setClickTicks(SET_MSEC_FOR_ONE_CLICK);
  
  lcd.begin(16, 2);
  
  attachInterrupt(0, catchRotor, CHANGE);
  attachInterrupt(1, catchRotor, CHANGE);
  
  pinMode(FQ_UD, OUTPUT);
  pinMode(W_CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(RESET, OUTPUT); 
  pulseHigh(RESET);
  pulseHigh(W_CLK);
  pulseHigh(FQ_UD);  // this pulse enables serial mode on the AD9850 - Datasheet page 12.
  
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
  analogReference(EXTERNAL); // use AREF for reference voltage 3.3V. It makes better resolution.
  
  //initial step settings
  increment = 1000;
  hertz = "1 kHz";
  hertzPosition = 11;
 
  lcd.setCursor(hertzPosition,1);    
  lcd.print(hertz);
   // Load the stored frequencyuency  
    frequency = String(EEPROM.read(0))+String(EEPROM.read(1))+String(EEPROM.read(2))+String(EEPROM.read(3))+String(EEPROM.read(4))+String(EEPROM.read(5))+String(EEPROM.read(6));
    freqencyValue = frequency.toInt();  //TODO: more check if toInt creates error during conversion
    if(freqencyValue<LOWER_FREQUENCY_LIMIT and freqencyValue>UPPER_FREQUENCY_LIMIT){
      freqencyValue=INITIAL_FREQUENCY_SET;
    } 
}

void loop() {
  // keep watching the push button:
  button.tick();
  doIfFrequencyWasChanged();
   doHereThingsOncePerInterval();
}

void catchRotor(){
  unsigned char result = r.process();
  if (result) {    
    if (result == DIR_CW){freqencyValue=freqencyValue+increment;}
    else {freqencyValue=freqencyValue-increment;};       
      if (freqencyValue >=UPPER_FREQUENCY_LIMIT){freqencyValue=freqencyValuePrev;}; // UPPER VFO LIMIT
      if (freqencyValue <=LOWER_FREQUENCY_LIMIT){freqencyValue=freqencyValuePrev;}; // LOWER VFO LIMIT   #to moje
}}

// frequencyuency calc from datasheet page 8 = <sys clock> * <frequencyuency tuning word>/2^32
void sendfrequency(double frequency) {  
  int32_t calculatedFrequency = frequency * 4294967295/125000000;  // note 125 MHz clock on 9850.  You can make 'slight' tuning variations here by adjusting the clock frequencyuency.
  for (int b=0; b<4; b++, calculatedFrequency>>=8) {
    transferByte(calculatedFrequency & 0xFF);
  }
  transferByte(0x000);   // Final control byte, all 0 for 9850 chip
  pulseHigh(FQ_UD);  // Done!  Should see output
}
// transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void transferByte(byte data)
{
  for (int i=0; i<8; i++, data>>=1) {
    digitalWrite(DATA, data & 0x01);
    pulseHigh(W_CLK);   //after each bit sent, CLK is pulsed high
  }
}

void setincrement(){
  if (increment == 10){increment = 100;  hertz = "100 Hz"; hertzPosition=9;}
  else if (increment == 100){increment = 1000; hertz="1 Khz"; hertzPosition=11;}
  else if (increment == 1000){increment = 10000; hertz="10 Khz"; hertzPosition=10;}
  else if (increment == 10000){increment = 100000; hertz="100 Khz"; hertzPosition=9;}
  else if (increment == 100000){increment = 1000000; hertz="1 Mhz"; hertzPosition=11;}  
  else{increment = 10; hertz = "10 Hz"; hertzPosition=10;};  
   lcd.setCursor(0,1);
   lcd.print("                ");
   lcd.setCursor(hertzPosition,1); 
   lcd.print(hertz); 
};

void showFrequency(){
    millions = int(freqencyValue/1000000);
    hundredthousands = ((freqencyValue/100000)%10);
    tenthousands = ((freqencyValue/10000)%10);
    thousands = ((freqencyValue/1000)%10);
    hundreds = ((freqencyValue/100)%10);
    tens = ((freqencyValue/10)%10);
    ones = ((freqencyValue/1)%10);
    lcd.setCursor(0,0);
    lcd.print("                ");
   if (millions > 9){lcd.setCursor(1,0);}
   else{lcd.setCursor(2,0);}
    lcd.print(millions);
    lcd.print(".");
    lcd.print(hundredthousands);
    lcd.print(tenthousands);
    lcd.print(thousands);
    lcd.print(".");
    lcd.print(hundreds);
    lcd.print(tens);
    lcd.print(ones);
    lcd.print(" Mhz  ");
    timepassed = millis();
};

void storeMEM(){
  //Write each frequencyuency section to a EPROM slot.  Yes, it's cheating but it works!
   EEPROM.write(0,millions);
   EEPROM.write(1,hundredthousands);
   EEPROM.write(2,tenthousands);
   EEPROM.write(3,thousands);
   EEPROM.write(4,hundreds);       
   EEPROM.write(5,tens);
   EEPROM.write(6,ones);   
   lcd.setCursor(0,0);
   lcd.print("FREQUENCY SAVED ");
   delay(1000);
   showFrequency();
};


/*
Found somewhere in a forum
*/
//TODO create one function returns String
void lcdPrintFloat( float val, byte precision){
  // prints val on a ver 0012 text lcd with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimial places
  // example: lcdPrintFloat( 3.1415, 2); // prints 3.14 (two decimal places)

  if(val < 0.0){
    lcd.print('-');
    val = -val;
  }

  lcd.print ((long)val);  //prints the integral part
  if( precision > 0) {
    lcd.print("."); // print the decimal point
    unsigned long frac;
    unsigned long mult = 1;
    byte padding = precision -1;
    while(precision--)
  mult *=10;

    if(val >= 0)
 frac = (val - int(val)) * mult;
    else
 frac = (int(val)- val ) * mult;
    unsigned long frac1 = frac;
    while( frac1 /= 10 )
 padding--;
    while(  padding--)
 lcd.print("0");
    lcd.print(frac,DEC) ;
  }
}

void serialPrintFloat( float val, byte precision){
  // prints val on a ver 0012 text lcd with number of decimal places determine by precision
  // precision is a number from 0 to 6 indicating the desired decimial places
  // example: lcdPrintFloat( 3.1415, 2); // prints 3.14 (two decimal places)

  if(val < 0.0){
    Serial.print('-');
    val = -val;
  }
  Serial.print ((long)val);  //prints the integral part
  if( precision > 0) {
    Serial.print("."); // print the decimal point
    unsigned long frac;
    unsigned long mult = 1;
    byte padding = precision -1;
    while(precision--)
  mult *=10;

    if(val >= 0)
 frac = (val - int(val)) * mult;
    else
 frac = (int(val)- val ) * mult;
    unsigned long frac1 = frac;
    while( frac1 /= 10 )
 padding--;
    while(  padding--)
 Serial.print("0");
    Serial.print(frac,DEC) ;
  }
}


