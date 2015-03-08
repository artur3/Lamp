//Lampa_v1.ino

#include <FastLED.h>
#include "IRLremote.h"
#include <EEPROM.h>


#define _DEBUG_


#define BUZZER_PIN  		2
#define RELAY1_PIN 		7
#define RELAY2_PIN  		6
#define RELAY3_PIN  		5

#define IR_PIN      		3
#define LIGHTS_PIN      	A0

#define LED_PIN     		10
#define COLOR_ORDER 		RGB
#define CHIPSET     		WS2812
#define NUM_LEDS    		12

#define BRIGHTNESS  		100
#define BEEPTIME 		11
#define LIGHTLEVEL  		300

#define LEDONSPEED		70
#define LEDOFFSPEED		15

#define NOOFPROGRAMMS		11

#define SBLEDON			0
#define SBRELY1 		1
#define SBRELY2			2
#define SBRELY3			3


#define TBDBEEP		   	0
#define TBBEEP			1
#define TBLED			2
#define TBLEDREF		3


// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  				50

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 				100



CRGB leds[NUM_LEDS];

const int interruptIR = digitalPinToInterrupt(IR_PIN);

uint8_t IRProtocol = 0;
uint16_t IRAddress = 0;
uint32_t IRCommand = 0;

unsigned long ledDalayTimerReload;

unsigned long runTime = 0;
long ledDalayTimer = 0;
long beepTimer = BEEPTIME;

byte ledBrightness;
byte currentLedBrightness;
byte outStatus;
byte timerBits;
byte program;
int lightLevel;
int setLightLevel;
byte counter1;
byte needs_save;
unsigned int personsIn;

CRGBPalette16 myPalette;


//*********************************************************************************
//
//*********************************************************************************

void setup() 
{

	delay(1000); // sanity delay

	currentLedBrightness = 0;
	outStatus = 0x00;
	timerBits = 0;
	program = 0;
	personsIn = 0;
	ledDalayTimerReload = LEDONSPEED;
	ledDalayTimer = ledDalayTimerReload;
	needs_save = 0;


	pinMode(BUZZER_PIN, OUTPUT);
	pinMode(RELAY1_PIN, OUTPUT);
	pinMode(RELAY2_PIN, OUTPUT);
	pinMode(RELAY3_PIN, OUTPUT);

	digitalWrite(BUZZER_PIN, HIGH);
	digitalWrite(RELAY1_PIN, HIGH);
	delay(1000);
	digitalWrite(RELAY2_PIN, HIGH);
	delay(1000);
	digitalWrite(RELAY3_PIN, HIGH);

	

	
	FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	FastLED.setBrightness(currentLedBrightness);
	FastLED.clear(true);

	myPalette = RainbowStripesColors_p;


#if defined(_DEBUG_)

	Serial.begin(57600);
	Serial.println("Startup");

#endif

	

	if(EEPROM.read(0) == 0xA5)
	{
    ledBrightness = EEPROM.read(1);
   	lightLevel = eepromReadInt(2);

	}
	else  // save default values to EEPROM
	{
		EEPROM.write(0, 0xA5);
		delay(5);
		ledBrightness = BRIGHTNESS;
		EEPROM.write(1, ledBrightness);
		delay(5);
		lightLevel = LIGHTLEVEL;
		eepromWriteInt(2, lightLevel);
	};

	setLightLevel = analogRead(LIGHTS_PIN);

	// choose your protocol here to reduce flash/ram/performance
	// see readme for more information
	IRLbegin<IR_NEC>(interruptIR);


	bitSet(timerBits, TBBEEP);
	bitSet(timerBits, TBLED);
	bitSet(timerBits, TBLEDREF);
}

//*********************************************************************************
//
//*********************************************************************************

void loop()
{
	unsigned long tim;
	uint8_t oldSREG;



	random16_add_entropy(random());

	oldSREG = SREG;
	cli();

	if(IRProtocol) 
	{


#if defined(_DEBUG_)

		// print as much as you want in this function
		// see source to terminate what number is for each protocol
		Serial.println();
		Serial.print("Protocol:");
		Serial.println(IRProtocol);
		Serial.print("Address:");
		Serial.println(IRAddress, HEX);
		Serial.print("Command:");
		Serial.println(IRCommand, HEX);

#endif


		if(IRAddress == 0xFD00) //Hitachi to Keyes
		{
			switch(IRCommand) 
			{
				case 0xF609: // 0 
					IRCommand = 0xAD52; 
					break;

				case 0xEC13: // 1
					IRCommand = 0xE916; 
					break;

				case 0xEF10: // 2
					IRCommand = 0xE619; 
					break;

				case 0xED12: // 3
					IRCommand = 0xF20D; 
					break;

				case 0xFC03: // 5
					IRCommand = 0xE718; 
					break;

				case 0xFA05: // 7
					IRCommand = 0xF708; 
					break;

				case 0xF906: // 8
					IRCommand = 0xE31C; 
					break;

				case 0xE11E: // 9
					IRCommand = 0xA55A; 
					break;

				case 0xE31C: // Power
					IRCommand = 0xE916; 
					break;

				case 0xF708: // Up
					IRCommand = 0xB946; 
					break;

				case 0xEB14: // Down
					IRCommand = 0xEA15; 
					break;

				case 0xFF00: // Enter
					IRCommand = 0xBF40; 
					break;

				default:
					IRCommand = 0;
			};
		};


	
		switch(IRCommand) // Keyes from Kjell&Co
		{
			case 0xAD52: // 0 - ALL OFF
				bitSet(timerBits, TBBEEP);
				bitClear(outStatus, SBLEDON);
				bitClear(outStatus, SBRELY1);
				bitClear(outStatus, SBRELY2);
				bitClear(outStatus, SBRELY3);
				break;

			case 0xE916: // 1 - LED ON/OFF PROGRAM 1
				program = 201;
				bitSet(timerBits, TBBEEP);
				//bitSet(outStatus, SBLEDON);
				bitWrite(outStatus, SBLEDON, !bitRead(outStatus, SBLEDON));

				if(needs_save == 1)
				{
				  saveToEeprom();
				  //bitSet(timerBits, TBDBEEP);
				};
				break;

			case 0xE619: // 2 - LCHANGE PROGRAM +
				ledDalayTimerReload = LEDONSPEED;
				program++;
				if(program > NOOFPROGRAMMS)
				  program = 201;
				bitSet(timerBits, TBBEEP);
				//bitSet(outStatus, SBLEDON);
				bitWrite(outStatus, SBLEDON, true);
				needs_save = 0;
				break;

			case 0xF20D: // 3 - CHANGE PROGRAM -
				ledDalayTimerReload = LEDONSPEED;
				program--;
				if(program == 0)
				  program = NOOFPROGRAMMS;
				  
				if(program > NOOFPROGRAMMS)
				  program = NOOFPROGRAMMS;

				if(program == 1)
				  program = 201;

				bitSet(timerBits, TBBEEP);
				//bitSet(outStatus, SBLEDON);
				bitWrite(outStatus, SBLEDON, true);
				needs_save = 0;
				break;

			case 0xE718: // 5 - ALL RELAYS ON
				bitSet(timerBits, TBBEEP);
				bitSet(outStatus, SBRELY1);
				bitSet(outStatus, SBRELY2);
				bitSet(outStatus, SBRELY3);
				needs_save = 0;
				break;

			case 0xF708: // 7 - RELAY 1 ON/OFF
				bitWrite(outStatus, SBRELY1, !bitRead(outStatus, SBRELY1));
				bitSet(timerBits, TBBEEP);
				needs_save = 0;
				break;

			case 0xE31C: // 8 - RELAY 2 ON/OFF
				bitWrite(outStatus, SBRELY2, !bitRead(outStatus, SBRELY2));
				bitSet(timerBits, TBBEEP);
				needs_save = 0;
				break;

			case 0xA55A: // 9 - RELAY 3 ON/OFF
				bitWrite(outStatus, SBRELY3, !bitRead(outStatus, SBRELY3));
				bitSet(timerBits, TBBEEP);
				needs_save = 0;
				break;

			case 0xB946: // Up - BRIGHTNESS UP WHEN LEDS ON or SET LIGHT LEVEL WHEN LEDS OFF
			  if(bitRead(outStatus, SBLEDON))
			  {
				  if(ledBrightness <= 229)
				    ledBrightness += 25;

				  bitSet(timerBits, TBBEEP);
				  needs_save = 0;
				}
				else
				{
				  if(setLightLevel <= 1000)
				    setLightLevel += 20;

					if(analogRead(LIGHTS_PIN) >= setLightLevel)
					{
						fill_solid(leds, NUM_LEDS, CRGB::Black);
						leds[0] = CRGB::Red;
						FastLED.show(255);
					}
					else
					{
						fill_solid(leds, NUM_LEDS, CRGB::Black);
						FastLED.show();
					};

				  bitSet(timerBits, TBBEEP);
				};
				
				break;

			case 0xEA15: // Down - BRIGHTNESS DOWN WHEN LEDS ON or SET LIGHT LEVEL WHEN LEDS OFF
			  if(bitRead(outStatus, SBLEDON))
			  {
				  if(ledBrightness > 25)
				    ledBrightness -= 25;

				  bitSet(timerBits, TBBEEP);
				  needs_save = 0;
				}
				else
				{
				  if(setLightLevel > 51)
				    setLightLevel -= 20;

				  if(setLightLevel < 0)
				    setLightLevel = LIGHTLEVEL;

					if(analogRead(LIGHTS_PIN) >= setLightLevel)
					{
						fill_solid(leds, NUM_LEDS, CRGB::Black);
						leds[0] = CRGB::Red;
						FastLED.show(255);
					}
					else
					{
						fill_solid(leds, NUM_LEDS, CRGB::Black);
						FastLED.show();
					};

				  bitSet(timerBits, TBBEEP);
				};
			
				break;

			case 0xBF40: // OK/ENTER - SAVE SETTINGS, THEN PRESS 1 TO CONFIRM
			  needs_save = 1;
			  bitSet(timerBits, TBBEEP);
			  //needs_save = 0;
				break;

			case 0xBD42: // SWITCH ON - FROM EXTERNAL MOTION SENSOR
			  if(analogRead(LIGHTS_PIN) >= lightLevel) // check light level
				{
					personsIn++;
					ledDalayTimerReload = LEDONSPEED;
					program = 201;
					bitSet(timerBits, TBBEEP);
					bitSet(outStatus, SBLEDON);
				};

			  bitSet(timerBits, TBBEEP);
			  needs_save = 0;
				break;

			case 0xB54A: // SWITCH OFF - FROM EXTERNAL MOTION SENSOR
				if(personsIn > 0)
				{
					personsIn--;
				};

				if(personsIn == 0)
				{
					ledDalayTimerReload = LEDOFFSPEED;
					bitClear(outStatus, SBLEDON);
				};

				bitSet(timerBits, TBBEEP);
				needs_save = 0;
				break;

			default:
				needs_save = 0;
		};

		// reset variable to not read the same value twice
		IRProtocol = 0;
		IRAddress = 0;
		IRCommand = 0;
	};

	SREG = oldSREG;



	if(!bitRead(outStatus, SBLEDON))
	{
		counter1 = 0;
	};


//**************** OUTPUTS *******************

	if(bitRead(outStatus, SBLEDON))
	{	

		if(bitRead(timerBits, TBLED))
		{
			
			if(program == 201)  // 20x - do olny once
			{
				ledDalayTimerReload = LEDONSPEED;
				fill_solid(leds, NUM_LEDS, CRGB::FairyLight);
				bitSet(timerBits, TBLEDREF);
				program = 1;
			};


			if(program == 2)
			{
				ledDalayTimerReload = 100;

				fill_rainbow(leds, NUM_LEDS, random8(20, 255));
					
				if(counter1 >= NUM_LEDS)
					counter1 = 0;

				leds[counter1] = ColorFromPalette(myPalette, random8(0, 255));
				counter1++;
				bitSet(timerBits, TBLEDREF);
			};


			if(program == 3)
			{
				ledDalayTimerReload = 100;
		
				fill_solid(leds, NUM_LEDS, CRGB::Black);
				
				if(counter1 >= NUM_LEDS)
				  counter1 = 0;

				leds[counter1] = ColorFromPalette(myPalette, random8(0, 255));
				counter1++;
				bitSet(timerBits, TBLEDREF);
			};


			if(program == 4)
			{
				ledDalayTimerReload = 200;
			
				fill_solid(leds, NUM_LEDS, CRGB::Black);
				
				if(counter1 >= NUM_LEDS)
				  counter1 = 0;

				leds[counter1] = ColorFromPalette(myPalette, random8(0, 255));
				byte i = counter1;
				counter1++;

				if(counter1 >= NUM_LEDS)
				  counter1 = 0;

				leds[counter1] = leds[i];
				counter1++;
				bitSet(timerBits, TBLEDREF);
			};


			if(program == 5)
			{
				
        if(counter1)
        {
				  leds[random8(0, NUM_LEDS)] =  ColorFromPalette(myPalette, random8(0, 255));
				  counter1 = 1;
				  ledDalayTimerReload = random16(20, 700);
				  nscale8(leds, NUM_LEDS, 224);
				}
				else
				{
          counter1 = 1;
          ledDalayTimerReload = random16(50, 1000);
				};
				bitSet(timerBits, TBLEDREF);
			};


			if(program == 6)
			{
				ledDalayTimerReload = 50;
		
				counter1++;                              
  			fill_rainbow(leds, NUM_LEDS, counter1, 10);
  			bitSet(timerBits, TBLEDREF);
			
			};


			if(program == 7)
			{
				ledDalayTimerReload = random16(20, 300);
				nscale8(leds, NUM_LEDS, 224);
				
				byte i = random8(0, 12);

				switch(i) 
				{
				    case 0:
				      leds[0] = CRGB::WhiteSmoke;
				      leds[3] = CRGB::WhiteSmoke;
				      break;

				    case 1:
				      leds[1] = CRGB::White;
				      leds[2] = CRGB::White;
				      break;

				    case 2:
				      leds[4] = CRGB::WhiteSmoke;
				      leds[7] = CRGB::WhiteSmoke;
				      break;

				    case 3:
				      leds[5] = CRGB::White;
				      leds[6] = CRGB::White;
				      break;

				    case 4:
				      leds[8] = CRGB::WhiteSmoke;
				      leds[11] = CRGB::WhiteSmoke;
				      break;

				    case 5:
				      leds[9] = CRGB::FairyLight;
				      leds[10] = CRGB::FairyLight;
				      break;

				    default:
				    	break;
				};
				bitSet(timerBits, TBLEDREF);
			};


			if(program == 8)
			{
				ledDalayTimerReload = 20;

				uint8_t middle = 0;
				uint8_t side = 0;
				uint8_t other = 0;

				counter1++;                                                     // overflowing a byte => 0.
				uint8_t x = quadwave8(counter1);

				other = map(x, 0, 255, NUM_LEDS/4, NUM_LEDS/4*3);            // 1/4 to 3/4 of strip.
				side = map(x, 0, 255, 0, NUM_LEDS-1);                        // full length of strip.
				middle = map(x, 0, 255, NUM_LEDS/3, NUM_LEDS/3*2);           // 1/3 to 2/3 of strip.

				leds[middle] = CRGB::Red;
				leds[side] = CRGB::Blue;
				leds[other] = CRGB::Green;

				nscale8(leds, NUM_LEDS, 224); 
				bitSet(timerBits, TBLEDREF);
				
			};


			if(program == 9)
			{
				ledDalayTimerReload = 90;
			
				Fire2012();	
				bitSet(timerBits, TBLEDREF);
			};


			if(program == 10)
			{
				ledDalayTimerReload = 130;
			
				(counter1 ? leds[0] = CRGB::Red : leds[0] = CRGB::MediumBlue);
				(counter1 ? leds[1] = CRGB::MediumBlue : leds[1] = CRGB::Red);
				(counter1 ? leds[2] = CRGB::MediumBlue : leds[2] = CRGB::Red);
				(counter1 ? leds[3] = CRGB::Red : leds[3] = CRGB::MediumBlue);

				(counter1 ? leds[4] = CRGB::Red : leds[4] = CRGB::MediumBlue);
				(counter1 ? leds[5] = CRGB::MediumBlue : leds[5] = CRGB::Red);
				(counter1 ? leds[6] = CRGB::MediumBlue : leds[6] = CRGB::Red);
				(counter1 ? leds[7] = CRGB::Red : leds[7] = CRGB::MediumBlue);

				(counter1 ? leds[8] = CRGB::Red : leds[8] = CRGB::Navy);
				(counter1 ? leds[9] = CRGB::Navy : leds[9] = CRGB::Red);
				(counter1 ? leds[10] = CRGB::Navy : leds[10] = CRGB::Red);
				(counter1 ? leds[11] = CRGB::Red : leds[11] = CRGB::Navy);

				if(counter1)
				{
					counter1 = 0;
				}
				else
				{

					counter1 = 1;
				};
				bitSet(timerBits, TBLEDREF);
			};


			if(program == 11)
			{
				ledDalayTimerReload = 140;
			
				(counter1 ? leds[0] = CRGB::Red : leds[0] = CRGB::Black);
				(counter1 ? leds[1] = CRGB::Black : leds[1] = CRGB::MediumBlue);
				(counter1 ? leds[2] = CRGB::Black : leds[2] = CRGB::MediumBlue);
				(counter1 ? leds[3] = CRGB::Red : leds[3] = CRGB::Black);

				(counter1 ? leds[4] = CRGB::Black : leds[4] = CRGB::Red);
				(counter1 ? leds[5] = CRGB::MediumBlue : leds[5] = CRGB::Black);
				(counter1 ? leds[6] = CRGB::MediumBlue : leds[6] = CRGB::Black);
				(counter1 ? leds[7] = CRGB::Black : leds[7] = CRGB::Red);

				(counter1 ? leds[8] = CRGB::Black : leds[8] = CRGB::Navy);
				(counter1 ? leds[9] = CRGB::Navy : leds[9] = CRGB::Black);
				(counter1 ? leds[10] = CRGB::Navy : leds[10] = CRGB::Black);
				(counter1 ? leds[11] = CRGB::Black : leds[11] = CRGB::Navy);

				if(counter1)
				{
					counter1 = 0;
				}
				else
				{

					counter1 = 1;
				};



				bitSet(timerBits, TBLEDREF);
			};
			

			bitClear(timerBits, TBLED);
		};
	}
	else
	{
		if(currentLedBrightness > 0)
		{	
			ledDalayTimerReload = LEDOFFSPEED;
			if(bitRead(timerBits, TBLED))
			{
				//fill_solid(leds, NUM_LEDS, CRGB::FairyLight);
				currentLedBrightness--;
				if(currentLedBrightness > 80)
				  currentLedBrightness--;

				FastLED.setBrightness(currentLedBrightness);	
				bitSet(timerBits, TBLEDREF);
				bitClear(timerBits, TBLED);
			};
		}
		else
			personsIn = 0;
	};


	digitalWrite(RELAY1_PIN, !bitRead(outStatus, SBRELY1));
	digitalWrite(RELAY2_PIN, !bitRead(outStatus, SBRELY2));
	digitalWrite(RELAY3_PIN, !bitRead(outStatus, SBRELY3));



	//**************** TIMERS *******************

	tim = millis() - runTime;
	runTime = millis();


	ledDalayTimer -= tim;
	if(ledDalayTimer <= 0)
	{	
		ledDalayTimer = ledDalayTimerReload;
		bitSet(timerBits, TBLED);

		if(bitRead(outStatus, SBLEDON))  // Brightness controll
		{
			if(currentLedBrightness < ledBrightness)
			{
				currentLedBrightness++;

				if(currentLedBrightness > (ledBrightness / 2))
					currentLedBrightness++;

				if(currentLedBrightness > ledBrightness)
					currentLedBrightness = ledBrightness;	

				FastLED.setBrightness(currentLedBrightness);
				bitSet(timerBits, TBLEDREF);
			};

			if(currentLedBrightness > ledBrightness)
			{
				currentLedBrightness--;
				if(currentLedBrightness > 80)
					currentLedBrightness--;

				FastLED.setBrightness(currentLedBrightness);
				bitSet(timerBits, TBLEDREF);
			};
		};

		if(bitRead(timerBits, TBLEDREF))  // update leds only when needed 
		{
			FastLED.show();
			bitClear(timerBits, TBLEDREF);
		};
	};


	if(bitRead(timerBits, TBBEEP))
	{
		digitalWrite(BUZZER_PIN, LOW);
		beepTimer = beepTimer - tim;
		if(beepTimer <= 0)
		{	
			digitalWrite(BUZZER_PIN, HIGH);
			bitClear(timerBits, TBBEEP);
			beepTimer = BEEPTIME;
		};
	}
	else
	{
		digitalWrite(BUZZER_PIN, HIGH);
		beepTimer = BEEPTIME;
	};
	
	delay(1);
}

//*********************************************************************************
//
//*********************************************************************************

void Fire2012()
{
// Array of temperature readings at each simulation cell
	static byte heat[NUM_LEDS];

	// Step 1.  Cool down every cell a little
		for( int i = 0; i < NUM_LEDS; i++) {
			heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
		}
	
		// Step 2.  Heat from each cell drifts 'up' and diffuses a little
		for( int k= NUM_LEDS - 1; k >= 2; k--) {
			heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
		}
		
		// Step 3.  Randomly ignite new 'sparks' of heat near the bottom
		if( random8() < SPARKING ) {
			int y = random8(7);
			heat[y] = qadd8( heat[y], random8(160,255) );
		}

		// Step 4.  Map from heat cells to LED colors
		for( int j = 0; j < NUM_LEDS; j++) {
				leds[j] = HeatColor( heat[j]);
		}
}

//*********************************************************************************
//
//*********************************************************************************

void IREvent(uint8_t protocol, uint16_t address, uint32_t command) 
{
	// called when directly received an interrupt CHANGE.
	// do not use Serial inside, it can crash your program!

	// update the values to the newest valid input
	IRProtocol = protocol;
	IRAddress = address;
	IRCommand = command;
}

//*********************************************************************************
//	
//*********************************************************************************

void saveToEeprom() 
{
	if(ledBrightness != EEPROM.read(1))
	{
		EEPROM.write(1, ledBrightness);
		delay(5);
	};

	if(setLightLevel != EEPROM.read(2))
	{
		lightLevel = setLightLevel;
		eepromWriteInt(2, lightLevel);
	};
	 

	needs_save = 0;
}

//*********************************************************************************
//
//*********************************************************************************

int eepromReadInt(int address)
{
   int value = 0x0000;
   value = value | (EEPROM.read(address) << 8);
   value = value | EEPROM.read(address+1);
   return value;
}

//*********************************************************************************
//
//********************************************************************************* 

void eepromWriteInt(int address, int value)
{
   EEPROM.write(address, (value >> 8) & 0xFF );
   delay(5);
   EEPROM.write(address+1, value & 0xFF);
   delay(5);
}

//*********************************************************************************
//
//*********************************************************************************
