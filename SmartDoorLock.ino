#include <EEPROM.h>
#include <LiquidCrystal.h>

byte data0 = 2;
byte data1 = 3;

byte maglockPin = 4;

byte sounderPin = 5;

byte D7 = 6;
byte D6 = 7;
byte D5 = 8;
byte D4 = 9;
byte E = 10;
byte RS = 11;
byte btn = 12;

boolean canStartTimer = 0;

byte numberOfCardsSaved = 0;
byte failedAttempts = 0;

volatile long valueRead = 0;
volatile unsigned long readerTime = 0;

volatile unsigned long startTime = 0;
volatile boolean alarm = false;
boolean addingCard = false;

byte difference = 70;

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);

void setup()
{
  Serial.begin(9600);  
  EEPROMWritelong(1, 4012360618); //This writes the value of the master card to the EEPROM
  numberOfCardsSaved = EEPROM.read(0); //This loads the number of cards stored in the EEPROM
  pinMode(maglockPin, OUTPUT); //This sets the pin connected to the driver for the maglock as an output
  pinMode(btn, INPUT); //this sets the button pin as an input
  digitalWrite(btn, HIGH); //this enables the internal pull up resistor on the button pin
  digitalWrite(maglockPin, HIGH); //This enables the maglock
  setupInterrupts(); //This initialises the interrupt functions on pin 2 and 3
  lcd.begin(16, 2); //This initialises the lcd display
  lcd.print("Please scan card");
  lcd.setCursor(0, 0);
}

//This associates the interrupt functions with a RISING edge input. And intialises the values.
void setupInterrupts()
{
  attachInterrupt(0, read0, RISING);
  attachInterrupt(1, read1, RISING);
  valueRead = 0;
  readerTime = 0;
  canStartTimer = 1;
}

void loop()
{
  if ((alarm) && (failedAttempts >= 3)) tones(); //This enables the alarm
  if (!digitalRead(btn)) addingCard = true; //This sets the addingCard mode to true when the button is held down
  else addingCard = false; //This disables the adding card mode

  
  //This checks that the card has been read for more than 60 milliseconds since the first interrupt of that scan
  if (((readerTime - startTime) >= 60) && ((readerTime - startTime) < 100))
  {
	//This disables the interrupt
    detachInterrupt(0);
    detachInterrupt(1);
    unsigned long valueReadU = valueRead; //This stores the value read as an unsigned long.
    lcd.clear();
    lcd.print("Card Scanned");
    lcd.setCursor(0, 0);
    Serial.print("Number of cards ");
    Serial.println(numberOfCardsSaved);
    for (int i = 1; i < (numberOfCardsSaved * 4); i += 4) //This loops through all the cards saved
    {
      unsigned long valueOfCard = (EEPROMReadlong(i)); //This reads the value from the EEPROM at the current address

      Serial.print("Allowed card value ");
      Serial.println(valueOfCard);
      if (((valueReadU) == valueOfCard) && (valueReadU != 0)) //This checks if the card value is an authorised one
      {
        if(alarm) alarm = false; //This disables the alarm
        if(addingCard)
        {
          lcd.clear();
          lcd.print("Scan new card");
          Serial.println("New card being scanned");
          lcd.setCursor(0, 0);
          setupInterrupts(); //This sets the interrupts back up.
          boolean loopB = true;
          while (loopB)
          {
            delay(10);
            if (((readerTime - startTime) >= 60) && ((readerTime - startTime) < 100))
            {
              boolean existsAlready = false;
              unsigned long valueReadU = valueRead;
			  //This checks if the card to be added is already added
              for (int i = 1; i < (numberOfCardsSaved * 4); i += 4)
              {
                unsigned long valueOfCard = EEPROMReadlong(i);
                if (((valueReadU) == valueOfCard) && (valueReadU != 0))
                {
                  Serial.println("Card already added");
                  existsAlready = true;
                  break;
                }
              }

              if(!existsAlready)
              {
				  //This checks that there is no more than 5 cards added to the EEPROM.
                if(numberOfCardsSaved < 5)
                {
                  Serial.print("Adding card value: ");
                  Serial.println(valueReadU);
				  //This writes the new card value to EEPROM 
                  EEPROMWritelong((numberOfCardsSaved * 4) + 1, (valueReadU));
                  unsigned long v = EEPROMReadlong(numberOfCardsSaved);
                  numberOfCardsSaved++;
				  //This updates the number of cards saved in EEPROM
                  EEPROM.write(0, numberOfCardsSaved);
                  Serial.print("Number of cards ");
                  Serial.println(numberOfCardsSaved);
                  loopB = false;

                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Card added");
                }
                else
                {
				  //This lets the user know too many cards have been added.
                  lcd.setCursor(0,0);
                  lcd.print("Too many cards");
                  Serial.println("Too many cards added");
                  lcd.setCursor(0,1);
                  lcd.print("added");
                  lcd.setCursor(0,0);
                  loopB = false;
                }
              }
              else
              {
                Serial.println("Card not added");
                loopB = false;
              }
            }            
          }
          break;  
        }
        else
        {
		  //This sets disables the maglock for three seconds. And in effect resets the system.
          digitalWrite(maglockPin, LOW);
          delay(3000);
          digitalWrite(maglockPin, HIGH);
          failedAttempts = 0;
          startTime = 0;
          lcd.clear();
          lcd.print("Please scan card");
          lcd.setCursor(0, 0);
          break;
        }
      }
      else if (valueRead == 0) break;
      else alarm = true;
      if (addingCard) break;
    }

    if (alarm)
    {
	  //This adds one to the invalid counter if an invalid card has been scanned.
      lcd.setCursor(0, 1);
      lcd.print("Invalid card");
      lcd.setCursor(0, 0);
      failedAttempts++;
    }
	//This resets the interrupt functions.
    setupInterrupts();
  }
}

//This sounds the alarm, this is called in the main loop so is continuous until an authorised card is scanned.
void tones()
{
  tone(sounderPin, 5000, 100);
  delay(100);
  tone(sounderPin, 800, 100);
  delay(100);
  tone(sounderPin, 1000, 100);
  delay(100);
}

//This is the interrupt method for if the RFID reader sends a 0.
void read0()
{
  readerTime = millis(); //This updates the timing
  if (canStartTimer) //This sets the start time.
  {
    startTime = readerTime;
    canStartTimer = false;
  }
  valueRead = valueRead << 1; //This updates the valueRead variable
  valueRead |= 1;
}

void read1()
{
  readerTime = millis(); //This updates the timing
  valueRead = valueRead << 1; //This updates the valueRead variable
}

//This splits the long value into four bytes and writes it to EEPROM
void EEPROMWritelong(int address, unsigned long value)
{
  byte by[4];
  long val = value;
  for (int i = 4; i > 0; i--)
  {
    by[i] = (val & 0xFF);
    val = val >> 8;
  }
  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, by[4]);
  EEPROM.write(address + 1, by[3]);
  EEPROM.write(address + 2, by[2]);
  EEPROM.write(address + 3, by[1]);
}

//This reads four bytes from the EEPROM and combines them to one long.
unsigned long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  long ret = ((four) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
  unsigned long returnVal = ret;
  return returnVal;
}



















