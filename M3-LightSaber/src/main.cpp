#include <Arduino.h>

#include <Adafruit_NeoPixel.h>
#include "FastLED.h"
#include <Math.h>
#include "LowPower.h"


#ifdef __AVR__
  #include <avr/power.h>
#endif


//==========================  DEFINES  ==========================
//----------PIN Setup-----------
#define LightStrip_pin 12
#define ON_OFF_Button_pin 5
#define Mode_Button_pin 9    

//--------LED Strip set up-------------
#define NUMBER_OF_LEDS 144
//=============================================================

//==========================  Buttons  ==========================
bool ON_OFF_Button_State = false;
bool ON_OFF_Button_CanChange = true;

uint32_t ON_OFF_Time = millis();
uint32_t ON_OFF_Timer = 500;

uint32_t Enter_Sleep_Time = millis();
uint32_t Enter_Sleep__Timer = 10000;
//=============================================================




//==========================  ENUMS  ==========================
// ---------STATES----------
enum SABER_STATE{ TURN_ON, TURN_OFF, IDLE }; // Define the states the  light saber can be in
SABER_STATE m_CurrentSaberState; // Define the starting state of the light saber
// --------MODES----------
enum SABER_MODE{ LEGO, REAL, XMAS }; // Define the Modes the  light saber can be in
SABER_MODE m_CurrentMode; // Define the Mode of the light saber
// --------COLOURS----------
enum SABER_COLOUR{ GREEN, RED, BLUE  }; // Define the Colours the  light saber can be in
SABER_COLOUR m_CurrentColour; // Define the Colours of the light saber
//=============================================================

//==========================  Variables  ==========================
// -----Plasma effect----------
// Plasma
bool realFullyoff = false;
uint32_t m_flame_coolDown = 10; // higher number shorter flame.
uint32_t m_flame_coolDown_multiplier = 1; // flame multipler (just leave at 10 , higher = smaller flame, smaller = larger flame)
// Spark
uint32_t m_Spark_chance = 140; // (0-255) chance of a spark to light
uint32_t m_min_height_spark = 45; // 0-100% the min height of the spark base
uint32_t m_min_spark_colour = 120; // min colour of the spark
uint32_t m_max_spark_colour = 255; // max colour of the spark
// Delay of effect
uint32_t m_Speed_delay = 0; // Speed delay higher slower
// ---------------------------

uint32_t Brightness = 1;


// -----Lego Starting effect-------
uint32_t Lego_ON_OFF_Delay = 2; // the delay in the colour turn on 
uint32_t LegoFadeDelay = 0;
bool LegoFullyOn = false;

// --------------------------------

// -----Lego ending effect-------
bool LegoFullyOFF = false;
// --------------------------------


// -----Xmas Starting effect-------
uint32_t XMAS_ON_OFF_Delay = 1; // the delay in the colour turn on 

// --------------------------------


bool m_ModeChangeWait = false;
uint32_t ModeSwapTime = millis();
uint32_t ModeSwapTime_Timer = 500;

uint32_t ColourSwapTime = millis();
uint32_t ColourSwapTime_Timer = 2000;


// -----------Pixal type guide--------
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
// -----------------------------------
// Define the lightstrip (pixal amount, Arduino pin number , pixel type flags, add together as needed)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_LEDS, LightStrip_pin, NEO_GRB + NEO_KHZ800);
//------------------------------------
//=============================================================

//----clear----
void Clear() {
  for(uint16_t i=0; i<NUMBER_OF_LEDS; i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
}
//------------
void VOID(void)
{
}

//==========================  REAL  ==========================
//----Plasma effect functions----
void setPlasmatColor (int Pixel, byte temp) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temp/255.0)*191);

  // calculate ramp up from
  byte heatramp = t192 & 63; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  switch (m_CurrentColour)
    {
      case GREEN:
          // figure out which third of the spectrum we're in:
          // hottest
          if( t192 > 128) {
            strip.setPixelColor(Pixel, strip.Color(255, 255, heatramp));
          } 
          // middle
          else if( t192 > 64 ) { 
           // strip.setPixelColor(Pixel, strip.Color(20, heatramp ,5));
            strip.setPixelColor(Pixel, strip.Color(200, 200, heatramp));

          } 
          // coolest
          else {
            strip.setPixelColor(Pixel, strip.Color(20, heatramp ,5));
          }
        break;
      
      case RED:
          // figure out which third of the spectrum we're in:
          // hottest
          if( t192 > 128) {
            strip.setPixelColor(Pixel, strip.Color(255, 255, heatramp));
          } 
          // middle
          else if( t192 > 64 ) { 
            strip.setPixelColor(Pixel, strip.Color(255 , heatramp, 0));
          } 
          // coolest
          else {
            strip.setPixelColor(Pixel, strip.Color(heatramp, 0 ,0));
          }
        break;

      case BLUE:
          // figure out which third of the spectrum we're in:
          // hottest
          if( t192 > 128) {
            strip.setPixelColor(Pixel, strip.Color(255, 255, heatramp));
          } 
          // middle
          else if( t192 > 64 ) { 
            strip.setPixelColor(Pixel, strip.Color(100 , heatramp, 255));
          } 
          // coolest
          else {
            strip.setPixelColor(Pixel, strip.Color(20, 20 ,heatramp));
          }
        break;
      default:
        break;
  }
}

void PlasmaEffect (int flameCooling, int SparkingChance, int SpeedDelay) {

  //set up heatmap
  static byte heat[NUMBER_OF_LEDS];
  //set up flicker cool down
  uint32_t m_flickerCoolDown;

  // --------------------------Dimmer----------------------------------
  // dim every cell a little
  for( int i = 0; i < NUMBER_OF_LEDS; i++) {
    m_flickerCoolDown = random(0, ((flameCooling * m_flame_coolDown_multiplier) / NUMBER_OF_LEDS) + 2);
   
    if(m_flickerCoolDown>heat[i]) {
      heat[i]=0;
    }
    else {
      heat[i]=heat[i]-m_flickerCoolDown;
    }
  }
  // ---------------------------------------------------------------------

  // --------------------------Heat Increase----------------------------------
  // intensity from each cell drifts 'up' and diffuses a little
  for( int k= NUMBER_OF_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  // ---------------------------------------------------------------------
   
  // --------------------------Sparking----------------------------------
  // Randomly start a flicker near the bottom section
  if( random(255) < SparkingChance ) {
    int y = random(m_min_height_spark);
    heat[y] = heat[y] + random(m_min_spark_colour,m_max_spark_colour);
  }
  // ---------------------------------------------------------------------

  // --------------------------Heat to colour----------------------------------
  // Convert heat to colors
  for( int j = 0; j < NUMBER_OF_LEDS; j++) {
    setPlasmatColor(j, heat[j] );
  }
  // ---------------------------------------------------------------------

  strip.show();//display strip
  delay(SpeedDelay);
}
//--------------------------------

//----Starting effect function----
void StartUpEffectOld() {

  int Position=0; // start at 0
  int count = 1; //set count to 1 so it does 1 then 2 then 3 etc
  for(int j = 0; j< NUMBER_OF_LEDS; j++)
  {
      Position++; // = 0; //Position + Rate;

      switch (m_CurrentColour)
      {
        case GREEN:
          for(int i=0; i< count; i++) {
          strip.setPixelColor(i,((sin(i+Position) * 127 + 128)/255)*50,
                                ((sin(i+Position) * 127 + 128)/255)*255,
                                ((sin(i+Position) * 127 + 128)/255)*50);
          }
          break;
        
        case RED:
          for(int i=0; i< count; i++) {
          strip.setPixelColor(i,((sin(i+Position) * 127 + 128)/255)*139,
                                ((sin(i+Position) * 127 + 128)/255)*0,
                                ((sin(i+Position) * 127 + 128)/255)*0);
          }
          break;

        case BLUE:
          for(int i=0; i< count; i++) {
          strip.setPixelColor(i,((sin(i+Position) * 127 + 128)/255)*10,
                                ((sin(i+Position) * 127 + 128)/255)*10,
                                ((sin(i+Position) * 127 + 128)/255)*255);
          }
          break;

        default:
          break;
      }

      count = count +1;
      strip.show();//display strip
      delay(50);
  }

}
void StartUpEffect() {
  bool Active= true;
  int BallCount = 25;
  float Gravity = -50;
  int StartHeightMin = 1;
  int StartHeightMax = 8;

  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * random(StartHeightMin,StartHeightMax) );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];
 
  for (int i = 0 ; i < BallCount ; i++) {  
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = random(StartHeightMin,StartHeightMax);
    Position[i] = 0;
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.90 - float(i)/pow(BallCount,2);
  }

  while (Active) {
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i]/1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i]/1000;
 
      if ( Height[i] < 0 ) {                      
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();
 
        if ( ImpactVelocity[i] < 5.0 ) {
              Active = false;
        }      
      }
      Position[i] = round( Height[i] * (NUMBER_OF_LEDS - 1) / random(StartHeightMin,StartHeightMax));
    }
 
    for (int i = 0 ; i < BallCount ; i++) {

       switch (m_CurrentColour)
    {
      case GREEN:
        strip.setPixelColor(Position[i] ,10 , random(0,255), 10);
        break;
      
      case RED:
        strip.setPixelColor(Position[i] ,random(0,255) , 10, 10);
        break;

      case BLUE:
        strip.setPixelColor(Position[i] ,10 , 10, random(0,255));
        break;
      default:
        break;
  }
    }   
    strip.show();
    Clear();    
  }
}
//---------------------------------

//----Starting effect function----
void EndingEffect() {

  bool Active= true;
  int InActiveCount = 0;
  int BallCount = 25;
  float Gravity = -100.0;
  int StartHeightMin = 1;
  int StartHeightMax = 5;

  float Height[BallCount];
  float ImpactVelocityStart = sqrt( -2 * Gravity * random(StartHeightMin,StartHeightMax) );
  float ImpactVelocity[BallCount];
  float TimeSinceLastBounce[BallCount];
  int   Position[BallCount];
  long  ClockTimeSinceLastBounce[BallCount];
  float Dampening[BallCount];
 
  for (int i = 0 ; i < BallCount ; i++) {  
    ClockTimeSinceLastBounce[i] = millis();
    Height[i] = random(StartHeightMin,StartHeightMax);
    Position[i] = 0;
    ImpactVelocity[i] = ImpactVelocityStart;
    TimeSinceLastBounce[i] = 0;
    Dampening[i] = 0.8 - float(i)/pow(BallCount,2);
  }

  while (Active) {
    InActiveCount = 0;
    for (int i = 0 ; i < BallCount ; i++) {
      TimeSinceLastBounce[i] =  millis() - ClockTimeSinceLastBounce[i];
      Height[i] = 0.5 * Gravity * pow( TimeSinceLastBounce[i]/1000 , 2.0 ) + ImpactVelocity[i] * TimeSinceLastBounce[i]/1000;
 
      if ( Height[i] < 0 ) {                      
        Height[i] = 0;
        ImpactVelocity[i] = Dampening[i] * ImpactVelocity[i];
        ClockTimeSinceLastBounce[i] = millis();
 
        if ( ImpactVelocity[i] < 0.1 ) {
          ImpactVelocity[i] = 0;
          InActiveCount++;
        }
        if(InActiveCount >=BallCount )
        {
          Active = false; 
        }
      }
      Position[i] = round( Height[i] * (NUMBER_OF_LEDS - 1) / random(StartHeightMin,StartHeightMax));
    }
 
    for (int i = 0 ; i < BallCount ; i++) {

       switch (m_CurrentColour)
    {
      case GREEN:
        strip.setPixelColor(Position[i] ,10 , random(100,255), 10);
        break;
      
      case RED:
        strip.setPixelColor(Position[i] ,random(0,255) , 10, 10);
        break;

      case BLUE:
        strip.setPixelColor(Position[i] ,10 , 10, random(100,255));
        break;
      default:
        break;
  }
    }
   
    strip.show();
    Clear();
    
  }
}
//---------------------------------
//=============================================================


//==========================  LEGO  ==========================
//----Lego Start-----
void LegoStart() {

  for(uint16_t i=0; i< NUMBER_OF_LEDS; i++) {

    switch (m_CurrentColour)
    {
      case GREEN:
        strip.setPixelColor(i, strip.Color(0 , 255, 0));
        break;
      
      case RED:
        strip.setPixelColor(i, strip.Color(255 , 0, 0));
        break;

      case BLUE:
        strip.setPixelColor(i, strip.Color(0 , 0, 255));
        break;
      default:
        break;
    }
    strip.show();
    delay(Lego_ON_OFF_Delay);
  }
}
//-------------------
//----Lego End-----
void LegoEnd() {
  uint16_t COUNT = NUMBER_OF_LEDS;
  for(uint16_t J=0; J <= NUMBER_OF_LEDS; J++) {
    Clear();
    for(uint16_t i=0; i <= COUNT; i++) {
      switch (m_CurrentColour)
      {
        case GREEN:
          strip.setPixelColor(i, strip.Color(0 , 255, 0));
          break;
        
        case RED:
          strip.setPixelColor(i, strip.Color(255 , 0, 0));
          break;

        case BLUE:
          strip.setPixelColor(i, strip.Color(0 , 0, 255));
          break;
        default:
          break;
      }
    }
    strip.show();
    delay(Lego_ON_OFF_Delay);
    COUNT--;
  }
  Clear();
  strip.show();

}
//-------------------
//-----Set colour----
void LegoSetColour()
{
    switch (m_CurrentColour)
    {
    case GREEN:
      for(uint16_t i=0; i<32; i++) {
        strip.setPixelColor(i, 0, 255, 0);
      }
      break;
    
    case RED:
      for(uint16_t i=0; i<32; i++) {
        strip.setPixelColor(i, 255, 0, 0);
      }
      break;

    case BLUE:
      for(uint16_t i=0; i < NUMBER_OF_LEDS; i++) {
        strip.setPixelColor(i, 0, 0, 255);
      }
      break;
    default:
      break;
    }
}
//---------------------
//---------------------------------
//=============================================================


//==========================  X-Mas  ==========================
//----X-mas Start-----
void XmasStart() {

    int Count = -1;
    switch (m_CurrentColour)
    {
      {
      case RED:
          for(uint16_t i=0; i< NUMBER_OF_LEDS; i++) {
              Count++;
              if(Count >= 2)
              {
                Count = 0;
              }

              if(Count == 0)
              {
                strip.setPixelColor(i, strip.Color(255 , 0, 0));
              }
              else if(Count == 1)
              {
                strip.setPixelColor(i, strip.Color(255 ,255, 255));
              }              
              strip.show();
              delay(XMAS_ON_OFF_Delay);
            }
        break;
      
      case GREEN:
            for(uint16_t i=0; i< NUMBER_OF_LEDS; i++) {

              strip.setPixelColor(i, strip.Color(0 , random(0,255), random(0,30)));

              strip.show();
              delay(XMAS_ON_OFF_Delay);
            }        break;

      case BLUE:
          for(uint16_t i=0; i< NUMBER_OF_LEDS; i++) {
              Count = random(0,4) ;
              if(Count == 0)
              {
                strip.setPixelColor(i, strip.Color(255 , 0, 0));
              }
              else if(Count == 1)
              {
                strip.setPixelColor(i, strip.Color( 0 ,255, 0));
              }
              else if(Count == 3)
              {
                strip.setPixelColor(i, strip.Color(0 ,0, 255));
              }                            
              strip.show();
              delay(XMAS_ON_OFF_Delay);
            
            }
      break;
      default :
        break;
      }
    }

}
//----Xmas End-----
void XmasEnd() {
  int CountLED = -1;
  uint16_t COUNT = NUMBER_OF_LEDS;

  switch (m_CurrentColour)
  {
  case RED:
      for(uint16_t J=0; J <= NUMBER_OF_LEDS; J++) {
        Clear();
        for(uint16_t i=0; i <= COUNT; i++) {

            CountLED++;
            if(CountLED >= 2)
            {
              CountLED = 0;
            }

            if(CountLED == 0)
            {
              strip.setPixelColor(i, strip.Color(255 , 0, 0));
            }
            else if(CountLED == 1)
            {
              strip.setPixelColor(i, strip.Color(255 ,255, 255));
            }
            
        }
        strip.show();
        delay(XMAS_ON_OFF_Delay);
        COUNT--;
      }
      Clear();
      strip.show();
    break;
  
  case GREEN:
      for(uint16_t J=0; J <= NUMBER_OF_LEDS; J++) {
        Clear();
        for(uint16_t i=0; i <= COUNT; i++) {
            strip.setPixelColor(i, strip.Color(0 , random(0,255), random(0,30)));            
            delay(Lego_ON_OFF_Delay/2);
        }
        strip.show();
        delay(Lego_ON_OFF_Delay);
        COUNT--;
      }
      Clear();
      strip.show();
    break;

  case BLUE:
      for(uint16_t J=0; J <= NUMBER_OF_LEDS; J++) {
        Clear();
        for(uint16_t i=0; i <= COUNT; i++) {

              CountLED = random(0,4) ;
              if(CountLED == 0)
              {
                strip.setPixelColor(i, strip.Color(255 , 0, 0));
              }
              else if(CountLED == 1)
              {
                strip.setPixelColor(i, strip.Color( 0 ,255, 0));
              }
              else if(CountLED == 3)
              {
                strip.setPixelColor(i, strip.Color(0 ,0, 255));
              }          
            
        }
        strip.show();
        delay(Lego_ON_OFF_Delay);
        COUNT--;
      }
      Clear();
      strip.show();
  break;
  default :
    break;
  }

}
//----Xmas Idel-----
void XmasIdel() {
    int Count = -1;
    switch (m_CurrentColour)
    {
      {
      case RED:
          for(uint16_t i=0; i< NUMBER_OF_LEDS; i++) {
              Count++;
              if(Count >= 2)
              {
                Count = 0;
              }

              if(Count == 0)
              {
                strip.setPixelColor(i, strip.Color(255 , 0, 0));
              }
              else if(Count == 1)
              {
                strip.setPixelColor(i, strip.Color(255 ,255, 255));
              }              
              strip.show();
              delay(XMAS_ON_OFF_Delay);
            }
        break;
      
      case GREEN:
          for(uint16_t i=0; i< NUMBER_OF_LEDS; i++) {

              strip.setPixelColor(i, strip.Color(0 , random(0,255), random(0,30)));

              strip.show();
              delay(XMAS_ON_OFF_Delay);
            }
        break;

      case BLUE:
          for(uint16_t i=0; i< NUMBER_OF_LEDS; i++) {
              Count = random(0,4) ;

              if(Count == 0)
              {
                strip.setPixelColor(i, strip.Color(255 , 0, 0));
              }
              else if(Count == 1)
              {
                strip.setPixelColor(i, strip.Color( 0 ,255, 0));
              }
              else if(Count == 3)
              {
                strip.setPixelColor(i, strip.Color(0 ,0, 255));
              }                            
              strip.show();
              delay(XMAS_ON_OFF_Delay);
            }
      break;
      default :
        break;
      }
    }
}
//=============================================================


//+++++++++++++++++++++  Modes  +++++++++++++++++++++
void Lego() {
  switch (m_CurrentSaberState)
    {
    case TURN_ON:
      LegoStart();
      m_CurrentSaberState = IDLE;
      break;

    case TURN_OFF:

      if(!LegoFullyOFF)
      {
        LegoEnd();
        LegoFullyOFF = true;
      }

      // check for when to turn back on

      if(ON_OFF_Button_State == HIGH)
      {
        LegoFullyOFF = false;
        m_CurrentSaberState = TURN_ON; 
      }
      break;

    case IDLE:


      // Button check to turn off Saber
      if(ON_OFF_Button_State == LOW)
      {
        m_CurrentSaberState = TURN_OFF; 
      }

      break;

    default:
      break;
    }
}

void Real() {
  switch (m_CurrentSaberState)
    {
    case TURN_ON:
      StartUpEffect();
      m_CurrentSaberState = IDLE;
      break;

    case TURN_OFF:

      if(realFullyoff == false)
      {
        EndingEffect();
      }
      realFullyoff = true;

      if(ON_OFF_Button_State == HIGH)
      {
        m_CurrentSaberState = TURN_ON;
        realFullyoff = false;
      }
      break;

    case IDLE:
      // Effect for the idel sequance
      PlasmaEffect(m_flame_coolDown,m_Spark_chance,m_Speed_delay);

      // Button check to turn off Saber
      if(ON_OFF_Button_State == LOW)
      {
        m_CurrentSaberState = TURN_OFF;
      }
      break;

    default:
      break;
    }
}

void Xmas() {

    switch (m_CurrentSaberState)
    {
    case TURN_ON:
      XmasStart();
      m_CurrentSaberState = IDLE;
      break;

    case TURN_OFF:

      if(!LegoFullyOFF)
      {
        XmasEnd();
        LegoFullyOFF = true;
      }

      // check for when to turn back on
      if(ON_OFF_Button_State == HIGH)
      {
        LegoFullyOFF = false;
        m_CurrentSaberState = TURN_ON; 
      }
      break;

    case IDLE:
      XmasIdel();
      // Button check to turn off Saber
      if(ON_OFF_Button_State == LOW)
      {
        m_CurrentSaberState = TURN_OFF; 
      }
      break;

    default:
      break;
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++  CORE  ++++++++++++++++++++++
// Set up function
void setup() {

  // set up serial connection
  Serial.begin(9600); 

  // pinMode(ON_OFF_Button_pin, INPUT_PULLDOWN);
  // pinMode(Mode_Button_pin, INPUT);

  pinMode(Mode_Button_pin, INPUT_PULLUP);
  pinMode(ON_OFF_Button_pin, INPUT);

  // Set up light saber pin
  m_min_height_spark = ((NUMBER_OF_LEDS * m_min_height_spark)/100); // take the percentage and change it into the number of pixals needed

  strip.begin();
  strip.setBrightness(Brightness); // Initialize all pixels to starting brightness 
  strip.show(); // Initialize all pixels to 'off'

  // Define starting state of the saber
  m_CurrentSaberState = TURN_ON;
  m_CurrentColour = RED;
  m_CurrentMode = LEGO;


  //turn off button
  ON_OFF_Button_State = true;
}

// Loop up function
void loop() {
    
  if(digitalRead(ON_OFF_Button_pin) == HIGH && ON_OFF_Button_CanChange == true)
  {
    ON_OFF_Button_State = !ON_OFF_Button_State;
    ON_OFF_Button_CanChange = false;  
    ON_OFF_Time = millis();
    Enter_Sleep_Time = millis();

  }
  else if (ON_OFF_Button_State == LOW)
  {
    if((millis() - Enter_Sleep_Time) >= Enter_Sleep__Timer)
    {
      attachInterrupt(ON_OFF_Button_pin, VOID, LOW);
      USBDevice.detach();  
      LowPower.standby();      
      USBDevice.attach();
      detachInterrupt(ON_OFF_Button_pin);

    }
  }
  
  if(ON_OFF_Button_CanChange == false)
  {
    if((millis() - ON_OFF_Time) >= ON_OFF_Timer)
    {
      ON_OFF_Button_CanChange = true;
    }
  }

  //Switch between modes
  switch (m_CurrentMode)
    {
    case LEGO:
      Lego();
      break;

    case REAL:
      Real();
      break;
      
    case XMAS:
      Xmas();
      break;

    default:
      break;
    }
    
    // // check for mod switch
    if(digitalRead(Mode_Button_pin) == LOW)
    {
      
      //check if saber is on
      if(ON_OFF_Button_State == HIGH)
        {
          if(m_ModeChangeWait == false)
          {
            m_CurrentSaberState = TURN_OFF;
            switch (m_CurrentMode)
            {
            case LEGO:
              Lego();
              break;

            case REAL:
              Real();
              break;
              
            case XMAS:
              Xmas();
              break;

            default:
              break;
            }

            ModeSwapTime = millis();
            m_ModeChangeWait = true;
            //based on current mode switch to next mode
            switch (m_CurrentMode)
            {
            case LEGO:
              m_CurrentMode = REAL;
              break;
            
            case REAL:
              m_CurrentMode = XMAS;
              break;

            case XMAS:
              m_CurrentMode = LEGO;
            break;
            default :
              break;
            }

            m_CurrentSaberState = TURN_ON;
          }
        }
        else //if saber is off turn the colour to the next one
        {
          if((millis() - ColourSwapTime) >= ColourSwapTime_Timer)
          {
            switch (m_CurrentColour)
            {
            case RED:
              m_CurrentColour = GREEN;
              break;
            
            case GREEN:
              m_CurrentColour = BLUE;
              break;

            case BLUE:
              m_CurrentColour = RED;
            break;
            default :
              break;
            }
            ColourSwapTime = millis();
            ON_OFF_Button_State = !ON_OFF_Button_State;
            m_CurrentSaberState = TURN_ON; 

          }
        }
    }
    else
    {
      ColourSwapTime = millis();
    }
    // wait if for button cooldown
    if(m_ModeChangeWait == true)
    {
      if( (millis() - ModeSwapTime) >= ModeSwapTime_Timer )
        {
          m_ModeChangeWait = false;
        }
    }
  
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++

