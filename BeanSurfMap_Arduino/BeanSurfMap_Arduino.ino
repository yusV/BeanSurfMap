#include <Adafruit_NeoPixel.h>

/*
  Receive surf conditions, send them to the neopixel
  
 */
#define FORECASTDAYS 6
#define DATA_PIN 5
#define PIXELS_IN_STRIP 88//56
#define PIXELS_PER_ROW 8
#define PIXELS_PER_RING 16    


#define FADE_ITERATIONS  50
#define FADE_SLEEP_MS    30
#define FADE_INCREMENT_VALUE 2
#define SPOT_REPORT_SIZE 16

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS_IN_STRIP, DATA_PIN, NEO_GRB + NEO_KHZ800);

static uint8_t waveHeights[FORECASTDAYS];
static uint8_t waveConditions[FORECASTDAYS];
uint32_t pixels[PIXELS_IN_STRIP];
uint8_t brightness;
uint8_t loopCount;
bool receivedConditions=false;
uint8_t offSeconds=10;
uint8_t onSeconds=4;
void setAllPixelsRGB(int r, int g, int b){
  for (int i=0; i< PIXELS_IN_STRIP; i++){
    pixels[i]=((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
  }
}
void  setAllPixels(uint32_t p){
  for (int i=0; i< PIXELS_IN_STRIP; i++){
    pixels[i]=p;
  }
}
void setNPixelsAtIndex(uint8_t index, uint8_t n, uint32_t color){
    for (int i=index; i< index+n; i++){
    pixels[i]=color;
  }
}

void updateStripWithBrightness(uint8_t br){
  for (int i=0; i< PIXELS_IN_STRIP; i++){
    uint8_t
      r = (uint8_t)(pixels[i] >> 16),
      g = (uint8_t)(pixels[i] >>  8),
      b = (uint8_t)pixels[i];
      r = (r * br) >> 8;
      g = (g * br) >> 8;
      b = (b * br) >> 8;
    //pixels[i]=((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
    //Serial.println(g);
    strip.setPixelColor(i,r,g,b);
  }
  strip.show();
}  
void updateStripWithPixels(){
  for (int i=0; i< PIXELS_IN_STRIP; i++){
    strip.setPixelColor(i, pixels[i]);
  }
  strip.show();  
}


void setup() {
  
  // initialize serial communication 
  Serial.begin(57600);
  // this makes it so that the arduino read function returns
  // immediatly if there are no less bytes than asked for.
  Serial.setTimeout(25);
  for (int i=0; i++; i<FORECASTDAYS){
   waveHeights[i]=0;
   waveConditions[i]=0; 
  }
  strip.begin();
  //setAllPixelsRGB(201,0,0);
  brightness=0;
  loopCount=0;
  setAllPixels(strip.Color(0, 0, 0));
  updateStripWithBrightness(brightness);
}

void loop() 
{
  //16 bytes per surf spot, 7 spots, 112 total bytes
  char buffer[128];
  uint8_t lastSpotUpdated=0;  
  
    if ( Serial.available() >= SPOT_REPORT_SIZE )
    {
      Serial.readBytes(buffer, SPOT_REPORT_SIZE);
      if (buffer[1]==0x00 && buffer[2]==FORECASTDAYS){       //0 denotes wave heights
      lastSpotUpdated=buffer[0];
      if (lastSpotUpdated>3 && lastSpotUpdated<8){ //skip over the ring LEDs (16 each, 32 total / 8 == 4. The tide times index is 8
        lastSpotUpdated+=4; 
      }

      for (int i=0; i<FORECASTDAYS; i++){
         waveHeights[i]=buffer[i+3];
       }
       for (int i=0; i<FORECASTDAYS; i++){        //surf conditions
         waveConditions[i]=buffer[i+10];
       }
       receivedConditions=true;
      for (int i=0; i<FORECASTDAYS; i++){
        int ht=waveHeights[i]*waveHeights[i]*4;
        if (ht>255){
          ht=255;
        }
        if (waveConditions[i]<2){                 //flat or no forecast
          strip.setPixelColor(i+lastSpotUpdated*8,0,0,10 );
        }
        else if (waveConditions[i]>=2 && waveConditions[i]<4){ //poor = blue. The color of the summer here in NorCal
          pixels[i+lastSpotUpdated*8]=strip.Color(0, 0, ht);
        }
        else if (waveConditions[i]==4){           //poor to fair = bluegreen. Let's be optimistic here... ;)
          pixels[i+lastSpotUpdated*8]=strip.Color(0, ht/2+20, ht);
        }
        else if (waveConditions[i]>=5 && waveConditions[i]<7){ //fair = green
          pixels[i+lastSpotUpdated*8]=strip.Color(0,ht,0 );         
        }
        else if (waveConditions[i]>=7 && waveConditions[i]<9){ //good = orange
          pixels[i+lastSpotUpdated*8]=strip.Color(ht,ht,0 );         
        }
        else if (waveConditions[i]>=9){           //EPIC!! Stop what you're doing and go surf!!1
          pixels[i+lastSpotUpdated*8]=strip.Color(ht,0,00 );         
        }
        if (i==FORECASTDAYS-1){
          pixels[i+lastSpotUpdated*8+1]=pixels[i+lastSpotUpdated*8];    //have the next 2 pixels match the last day in the forecast
          pixels[i+lastSpotUpdated*8+2]=pixels[i+lastSpotUpdated*8];
        }
      }
    }
    else if (buffer[1]==0x02 ){  //tides
      lastSpotUpdated=buffer[0];
      if (buffer[3]>12) //if PM
        setNPixelsAtIndex(lastSpotUpdated*8,PIXELS_PER_RING,strip.Color(0,60,0));     //dimly turn on all ring LEDs so the ring shape comes through.
      else 
        setNPixelsAtIndex(lastSpotUpdated*8,PIXELS_PER_RING,strip.Color(0,0,80));     //dimly turn on all ring LEDs so the ring shape comes through.
      if (buffer[5]>12) //if PM
        setNPixelsAtIndex(lastSpotUpdated*8+PIXELS_PER_RING,PIXELS_PER_RING,strip.Color(0,60,0));     //dimly turn on all ring LEDs so the ring shape comes through.
      else 
        setNPixelsAtIndex(lastSpotUpdated*8+PIXELS_PER_RING,PIXELS_PER_RING,strip.Color(0,0,80));     //dimly turn on all ring LEDs so the ring shape comes through.

      //low tide first 
      uint8_t pixelHour,pixelMinute;
      
      if (buffer[3]==0 && buffer[4]==0){
        buffer[3]=1;
        buffer[4]=20;
        pixelHour= (buffer[3]%12)*(16.0/12.0);     //mod 12 to remove 24hr time, 16/12 to go from 12 values to 16 LEDs. 
        pixels[pixelHour+lastSpotUpdated*8]=strip.Color(200,0,0);
        pixelMinute= buffer[4]*(16.0/60.0); //
        pixels[pixelMinute+lastSpotUpdated*8]=strip.Color(0,200,0);
      }
      else{
        pixelHour= (buffer[3]%12)*(16.0/12.0);     //mod 12 to remove 24hr time, 16/12 to go from 12 values to 16 LEDs. 
        pixels[pixelHour+lastSpotUpdated*8]=strip.Color(200,0,0);
        pixelMinute= buffer[4]*(16.0/60.0); //
        pixels[pixelMinute+lastSpotUpdated*8]=strip.Color(0,200,0);          
      }
      //high tide next
      if (buffer[5]==0 && buffer[6]==0){//no high tide

      }
      else{
        pixelHour= (buffer[5]%12)*(16.0/12.0);    //mod 12 to remove 24hr time, 16/12 to go from 12 values to 16 LEDs. 
        pixels[pixelHour+lastSpotUpdated*8+PIXELS_PER_RING]=strip.Color(200,0,0);
        pixelMinute= buffer[6]*(16.0/60.0); //
        pixels[pixelMinute+lastSpotUpdated*8+PIXELS_PER_RING]=strip.Color(0,200,0);     
      }
    }
    else if (buffer[1]=0x03){ //set up 
      offSeconds=buffer[2];
      onSeconds=buffer[3];
      
    }
    updateStripWithPixels();
   }
   //fade on
   for (int i=0;i<FADE_ITERATIONS;i++){
    if (Serial.available()>=SPOT_REPORT_SIZE){      //don't sleep or fade if you have another report waiting.
     break; 
    }
    brightness+=FADE_INCREMENT_VALUE;
    Bean.sleep(FADE_SLEEP_MS);
    updateStripWithBrightness(brightness);
   }
   brightness=FADE_INCREMENT_VALUE*FADE_ITERATIONS;
   updateStripWithBrightness(brightness);
   if (Serial.available()<SPOT_REPORT_SIZE){        //don't sleep or fade if you have another report waiting.
     Bean.sleep(onSeconds*1000);
   }
    
   //fade off
   for (int i=0;i<FADE_ITERATIONS;i++){
   if (Serial.available()>=SPOT_REPORT_SIZE){       //don't sleep or fade if you have another report waiting.
     break; 
    }
    brightness-=FADE_INCREMENT_VALUE;
    Bean.sleep(FADE_SLEEP_MS);
    updateStripWithBrightness(brightness);
   }
   brightness=0;
   updateStripWithBrightness(brightness);
    if (Serial.available()<SPOT_REPORT_SIZE){       //don't sleep or fade if you have another report waiting.
     Bean.sleep(offSeconds*1000);
   }
      
}























        /* The Surfline Surf Quality Scale
     © Copyright 2000-2010 Surfline/Wavetrak, Inc.
     http://www.surfline.com/surf-science/rating-of-surf-heights-and-quality_31942/
     1 = FLAT: Unsurfable or flat conditions. No surf.
     2 = VERY POOR: Due to lack of surf, very poor wave shape for surfing, bad surf due to other conditions like wind, tides, or very stormy surf.
     3 = POOR: Poor surf with some (30%) FAIR waves to ride.
     4 = POOR to FAIR: Generally poor surf many (50%) FAIR waves to ride.
     5 = FAIR: Very average surf with most (70%) waves rideable.
     6 = FAIR to GOOD: Fair surf with some (30%) GOOD waves.
     7 = GOOD: Generally fair surf with many (50%) GOOD waves.
     8 = VERY GOOD: Generally good surf with most (70%) GOOD waves.
     9 = GOOD to EPIC: Very good surf with many (50%) EPIC waves.
     10 = EPIC: Incredible surf with most (70%) waves being EPIC to ride and generally some of the best surf all year.
     */



