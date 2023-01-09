/*
GameBoy emulator for ESPboy.

You are able to try Nintendo retro games like SuperMario, Zelda, Pokemon, etc! Pressing both side buttons, you can adjust view port (original GB screen resolution is 160х144 but it's only 128x128 window visible on ESPboy display).

Peanut-GB core by deltabeard
https://github.com/deltabeard/Peanut-GBhttps://github.com/deltabeard/Peanut-GB

https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-funhttps://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun

MIT license
*/

#pragma GCC optimize ("-O3")
#pragma GCC push_options

#include "Arduino.h"
#include <ESP_EEPROM.h>
#include <Adafruit_MCP23017.h>
//#include <Adafruit_MCP4725.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include "ESPboyLogo.h"
#include "sound.h"
#include "peanut_gb.c"
#include "LittleFS.h"
#include "nbSPI.h"
#include <sigma_delta.h>


//------------------------------------sprite_1-----------------------------------------------------------sprite_2-------------------------------------------------------------background----------
  const uint32_t PROGMEM palette0[] = { 0x79DF, 0x2D7E, 0x6D2B, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x3FE6, 0x0200, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A }; // PeanutGB
  const uint32_t PROGMEM palette1[] = { 0x7FFF, 0x7EAC, 0x40C0, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x3FE6, 0x0200, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A }; // OBJ0
  const uint32_t PROGMEM palette2[] = { 0x7FFF, 0x3FE6, 0x0200, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x3FE6, 0x0200, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x3FE6, 0x0200, 0x0000 }; // OBJ1
  const uint32_t PROGMEM palette3[] = { 0x7FFF, 0x7EAC, 0x40C0, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x7EAC, 0x40C0, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x7EAC, 0x40C0, 0x0000 }; // BG
  const uint32_t PROGMEM palette4[] = { 0x6ACE, 0x279D, 0xE46B, 0x613A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A }; //nostalgia
  const uint32_t PROGMEM palette5[] = { 0x5685, 0x8F9D, 0x6843, 0x6541, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5685, 0x8F9D, 0x6843, 0x6541, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5685, 0x8F9D, 0x6843, 0x6541 }; //greeny
  const uint32_t PROGMEM palette6[] = { 0x16F6, 0xC9D3, 0xC091, 0x8041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x16F6, 0xC9D3, 0xC091, 0x8041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x16F6, 0xC9D3, 0xC091, 0x8041 }; //reddy
  const uint32_t PROGMEM palette7[] = { 0x79DF, 0x2D7E, 0x6D2B, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x79DF, 0x2D7E, 0x6D2B, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x79DF, 0x2D7E, 0x6D2B, 0x6308 }; //retro lcd
  const uint32_t PROGMEM palette8[] = { 0x1F87, 0x795C, 0x7C72, 0x6959, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1F87, 0x795C, 0x7C72, 0x6959, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1F87, 0x795C, 0x7C72, 0x6959 };  //WISH GB
  const uint32_t PROGMEM palette9[] = { 0xDDF7, 0xB7C5, 0xCE52, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xDDF7, 0xB7C5, 0xCE52, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xDDF7, 0xB7C5, 0xCE52, 0x6308 };  //HOLLOW
  const uint32_t PROGMEM palette10[] ={ 0x9B9F, 0xB705, 0xF102, 0x4A01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x9B9F, 0xB705, 0xF102, 0x4A01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x9B9F, 0xB705, 0xF102, 0x4A01 };  //BLK AQU4
  const uint32_t PROGMEM palette11[] ={ 0x49CD, 0x099B, 0x0549, 0x4320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x49CD, 0x099B, 0x0549, 0x4320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x49CD, 0x099B, 0x0549, 0x4320 };  //GOLD GB
  const uint32_t PROGMEM palette12[] ={ 0x719F, 0x523D, 0xEE42, 0x0629, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x719F, 0x523D, 0xEE42, 0x0629, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x719F, 0x523D, 0xEE42, 0x0629 };  //NYMPH GB
  const uint32_t PROGMEM palette13[] ={ 0x16F6, 0xC9D3, 0xC091, 0x8041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x16F6, 0xC9D3, 0xC091, 0x8041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5685, 0x8F9D, 0x6843, 0x6541 };  //BOOTLEG BY PIXELSHIFT
  const uint32_t PROGMEM palette14[] ={ 0x49CD, 0x099B, 0x0549, 0x4320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x49CD, 0x099B, 0x0549, 0x4320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A };  //nostalgia+GOLD GB
  
  
  const PROGMEM uint32_t *paletteN[] = {palette0, palette1, palette2, palette3, palette4, palette5, palette6, palette7, palette8, palette9, palette10, palette11, palette12, palette13, palette14};
  uint32_t *paletteNN;
  

//#include "GAMES/rom_1.h"  //test rom
#include "GAMES/rom_2.h"  //super mario land
//#include "GAMES/rom_3.h"  //tetris
//#include "GAMES/rom_4.h"  //lemmings
//#include "GAMES/rom_5.h"  //kirby's dream land
//#include "GAMES/rom_6.h"  //mega man
//#include "GAMES/rom_7.h"  //zelda
//#include "GAMES/rom_8.h"  //prince of persia
#include "GAMES/rom_9.h"  //contra
//#include "GAMES/rom_10.h" //Felix the cat
//#include "GAMES/rom_11.h" //Pokemon
//#include "GAMES/rom_12.h" //Castelian
//#include "GAMES/rom_13.h" //Castelvania
//#include "GAMES/rom_14.h" //Donkey Kong Land
//#include "GAMES/rom_15.h" //Double dragon
//#include "GAMES/rom_16.h" //R-type
//#include "GAMES/rom_17.h" //Mega man III
//#include "GAMES/rom_18.h" //R-type II
//#include "GAMES/rom_19.h" //nemezis
//#include "GAMES/rom_20.h" //ninja gaiden shadow
//#include "GAMES/rom_21.h" //spy vs spy
//#include "GAMES/rom_22.h" //Robocop
//#include "GAMES/rom_23.h" //Solar Striker
//#include "GAMES/rom_26.h" //Mortal Combat
//#include "GAMES/rom_27.h" //Mortal Combat 2
//#include "GAMES/rom_28.h" //Pokemon blue
//#include "GAMES/rom_29.h" //Q-bert
//#include "GAMES/rom_30.h" //PacMan
//#include "GAMES/rom_31.h" //Adventure Island
//#include "GAMES/rom_32.h" //Adventure Island II
//#include "GAMES/rom_33.h" //Castlevania II - Belmont's Revenge
//#include "GAMES/rom_34.h" //Chase H.Q. 
//#include "GAMES/rom_35.h" //Speedy Gonzales 
//#include "GAMES/rom_36.h" //Star Wars - The Empire Strikes Back 
//#include "GAMES/rom_37.h" //Super Mario Land 2 - 6 Golden Coins
//#include "GAMES/rom_38.h" //Super Off Road 
//#include "GAMES/rom_50.h" //pokemon blue


#define APP_MARKER 0xCCAA
#define WRITE_DELAY 2000
#define CART_SIZE 10000
#define GB_ROM rom

File fle;

uint8_t *cartSave;
uint8_t previousSoundFlag;
uint8_t cartSaveFlag = 0;
uint8_t paletteAndOffsetChangeFlag = 1;
uint32_t timeToSave;


struct SaveStruct{
  uint32_t marker = APP_MARKER;
  uint8_t  soundFlag = 1;
  uint8_t  paletteNo = 2;
  uint8_t  offset_x = 16;
  uint8_t  offset_y = 8;
};


SaveStruct defaultSaveStruct, realSaveStruct;


#define PAD_LEFT        0x01
#define PAD_UP          0x02
#define PAD_DOWN        0x04
#define PAD_RIGHT       0x08
#define PAD_ACT         0x10
#define PAD_ESC         0x20
#define PAD_LFT         0x40
#define PAD_RGT         0x80
#define PAD_ANY         0xff

#define MCP23017address 0  
#define LEDPIN D4
#define SOUNDPIN D3
#define CSTFTPIN 8  
#define LEDLOCK  9

Adafruit_MCP23017 mcp;
//Adafruit_MCP4725 dac;
TFT_eSPI tft = TFT_eSPI(); 

static struct gb_s gb;
enum gb_init_error_e ret;


void loadFS(){
    fle = LittleFS.open("/save.dat", "r+");
    for(uint16_t i=0; i<CART_SIZE; i++) 
      fle.write(cartSave[i]);
    fle.close();
};


uint8_t inline getKeys() __attribute__((always_inline));
uint8_t inline getKeys() { return (~mcp.readGPIOAB() & 255); }

void inline __attribute__((always_inline)) IRAM_ATTR readkeys(){
 static uint8_t nowkeys;
  nowkeys = getKeys();
  if (nowkeys&PAD_LFT && nowkeys&PAD_RGT && cartSaveFlag == 0) adjustOffset();
  else{
    gb.direct.joypad_bits.a = (nowkeys&PAD_ACT)?0:1;
    gb.direct.joypad_bits.b = (nowkeys&PAD_ESC)?0:1;
    gb.direct.joypad_bits.up = (nowkeys&PAD_UP)?0:1;
    gb.direct.joypad_bits.down = (nowkeys&PAD_DOWN)?0:1;
    gb.direct.joypad_bits.left = (nowkeys&PAD_LEFT)?0:1;
    gb.direct.joypad_bits.right = (nowkeys&PAD_RIGHT)?0:1;
    gb.direct.joypad_bits.start = (nowkeys&PAD_LFT)?0:1;
    gb.direct.joypad_bits.select = (nowkeys&PAD_RGT)?0:1;}
}


void adjustOffset(){
  static uint8_t nowkeys;
  previousSoundFlag = realSaveStruct.soundFlag;
  realSaveStruct.soundFlag=0;
  gb_run_frame(&gb);
  while(getKeys()) delay(100);
  while(1){
    tft.drawString(F("Adjusting LCD"), 24, 60);
    tft.drawString(F("up/down/left/right"), 8, 70);
    
    if (previousSoundFlag) tft.drawString(F("Sound ON "), 0, 0);
    else tft.drawString(F("Sound OFF"), 0, 0);
    tft.drawString(F("Palette N  "), 0, 10);
    tft.drawString((String)realSaveStruct.paletteNo, 66, 10);
    tft.drawString(F("Save marker "), 0, 20);
    if (realSaveStruct.marker) tft.drawString(F("ON"), 72, 20);
    else tft.drawString(F("OFF"), 72, 20);
    delay(150);
    while(!(nowkeys = getKeys())) delay(50);
    if (nowkeys&PAD_UP && realSaveStruct.offset_y>0) realSaveStruct.offset_y--;
    if (nowkeys&PAD_DOWN && realSaveStruct.offset_y<16) realSaveStruct.offset_y++;
    if (nowkeys&PAD_LEFT && realSaveStruct.offset_x>0) realSaveStruct.offset_x--;
    if (nowkeys&PAD_RIGHT && realSaveStruct.offset_x<32) realSaveStruct.offset_x++;
    if (nowkeys&PAD_ACT) previousSoundFlag = !previousSoundFlag;
    if (nowkeys&PAD_RGT) {realSaveStruct.paletteNo++; if(realSaveStruct.paletteNo==sizeof(paletteN)/sizeof(uint32_t *)) realSaveStruct.paletteNo=0;}
    if (nowkeys&PAD_LFT) {realSaveStruct.marker = !realSaveStruct.marker;}
    if (nowkeys&PAD_ESC) {break;}

    paletteAndOffsetChangeFlag = 1;
    gb_run_frame(&gb);
    gb_run_frame(&gb);
  }

  paletteAndOffsetChangeFlag = 1;
  realSaveStruct.soundFlag = previousSoundFlag;
  saveparameters();
};


uint8_t inline __attribute__((always_inline)) IRAM_ATTR gb_rom_read(struct gb_s *gb, const uint32_t addr){
  return pgm_read_byte(GB_ROM+addr);
}


uint8_t gb_cart_ram_read(struct gb_s *gb, const uint32_t addr){
  if(realSaveStruct.marker){
    tft.drawString("R", 0, 0);
    paletteAndOffsetChangeFlag=1;}

  if (addr<CART_SIZE){
    return cartSave[addr];}
  else {
    if(realSaveStruct.marker){
      tft.drawString(F("ERROR: OUT OF CART!"), 0, 0);
      paletteAndOffsetChangeFlag=1;
      delay(1000);
    }  
    //Serial.print("Read fail addr: "); Serial.println(addr);
  }
  return(0);
}


void gb_cart_ram_write(struct gb_s *gb, const uint32_t addr, const uint8_t val){
  
  cartSaveFlag = 1;
  timeToSave = millis(); 
  
  if(realSaveStruct.marker){ 
    tft.drawString("W", 0, 0);
    paletteAndOffsetChangeFlag=1;
  }

  if (addr<CART_SIZE){
    cartSave[addr] = val;
  }
  else {    
    if(realSaveStruct.marker){
      tft.drawString(F("ERROR: OUT OF CART!"), 0, 0);
      paletteAndOffsetChangeFlag=1;}  
    //Serial.print("Save fail addr: "); Serial.println(addr);
  }
}


void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val){
	const char* gb_err_str[4] = {
		"UNKNOWN",
		"INVALID OPCODE",
		"INVALID READ",
		"INVALID WRITE"
	};

	//Serial.print(F("Error "));
	//Serial.print(gb_err);
	//Serial.print(F(" occurred:  "));
	//Serial.println(gb_err >= GB_INVALID_MAX ?gb_err_str[0] : gb_err_str[gb_err]);
	//Serial.print(F("At Address "));
	//Serial.println(val,HEX);
}


  uint_fast8_t offset_xx;
  uint_fast8_t offset_yy;
  uint_fast8_t offset_yyy;
  uint16_t uiBuff1[128] __attribute__((aligned(32)));
  uint16_t uiBuff2[128] __attribute__((aligned(32)));
  uint16_t *currentBuf;
  uint8_t prevLine=0;
  bool flipBuf;
  

void IRAM_ATTR lcd_draw_line(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line){
   if (paletteAndOffsetChangeFlag) {
     paletteNN = (uint32_t *)paletteN[realSaveStruct.paletteNo];
     offset_xx = realSaveStruct.offset_x;
     offset_yy = realSaveStruct.offset_y;
     offset_yyy = offset_yy+128;
     tft.setAddrWindow(0, line-offset_yy, 128, 128);
     if (line == 0){
       paletteAndOffsetChangeFlag=0;
       tft.setAddrWindow(0, 0, 128, 128);
     }
   }

  uint_fast8_t pixels_x;
  if(line >= offset_yy && line < offset_yyy){
    if(line != prevLine+1)
      tft.setAddrWindow(0, line-offset_yy, 128, 128);
    if(flipBuf) currentBuf = uiBuff1;
    else currentBuf = uiBuff2;
    pixels_x = offset_xx;
    for (auto x = 0; x < 128; x++)
      currentBuf[x] = pgm_read_dword (&paletteNN[pixels[pixels_x++]]);
    while(nbSPI_isBusy());
    nbSPI_writeBytes((uint8_t*)currentBuf, 256);  

    prevLine=line;
   }

   flipBuf = !flipBuf;
}


volatile uint8_t sound_dac;

void IRAM_ATTR sound_ISR(){
  static bool sound_prev;
  if(realSaveStruct.soundFlag && sound_prev!=sound_dac){
    sigmaDeltaWrite(0, sound_dac);
    sound_dac = audio_update();}
  sound_prev = !sound_prev;
}


bool loadparameters(){
  EEPROM.get(0, realSaveStruct);
  if(realSaveStruct.marker != APP_MARKER){
    EEPROM.put(0, defaultSaveStruct);
    EEPROM.commit();
    realSaveStruct = defaultSaveStruct;
    return(1);
  }
  return(0);
};


void saveparameters(){
  realSaveStruct.soundFlag = previousSoundFlag;
  EEPROM.put(0, realSaveStruct);
  EEPROM.commit();
};


void setup() {
  //system_update_cpu_freq(SYS_CPU_160MHZ);
    
  //Serial.begin(115200);
  
  //Serial.println();
  //Serial.println(ESP.getFreeHeap());
  
//EEPROM init (for game cart RAM)  
  EEPROM.begin(sizeof(SaveStruct));
  
//DAC init 
  //dac.begin(0x60);
  //delay (100);
  //dac.setVoltage(0, false);

//MCP23017 GPIO extantion init
  mcp.begin(MCP23017address);
  delay(100);

  for (int i = 0; i < 8; ++i) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);
  }

//LED init
  //mcp.pinMode(LEDLOCK, OUTPUT);
  //mcp.digitalWrite(LEDLOCK, HIGH); 

// Sound init and test
  pinMode(SOUNDPIN, OUTPUT);

// TFT init
  mcp.pinMode(CSTFTPIN, OUTPUT);
  mcp.digitalWrite(CSTFTPIN, LOW);
  tft.begin();
  delay(100);
  tft.startWrite(); 
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

// draw ESPboylogo
  tft.drawXBitmap(30, 20, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(F("GameBoy emulator"), 20, 95);
  delay(1500);
  //for (uint8_t i = 0; i<100; i++){
  //  dac.setVoltage(i* 10, false);
  //  delay(7);}
  //dac.setVoltage(4095, true);
  //delay(1000);

// clear screen
  tft.fillScreen(TFT_BLACK);

  WiFi.mode(WIFI_OFF);


// init game boy emulator
   ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, NULL);

	//if(ret != GB_INIT_NO_ERROR){
		//Serial.print("Error: ");
		//Serial.println(ret);}

	  gb_init_lcd(&gb, &lcd_draw_line);
    gb.direct.interlace = 0;
    gb.direct.frame_skip = 1;


  // File system init
  cartSave = (uint8_t *)malloc (CART_SIZE+1);  
  LittleFS.begin();
  if (loadparameters()){
    tft.drawString(F("Formatting FS..."), 0, 0);
    LittleFS.format();
    tft.fillScreen(TFT_BLACK);
    tft.drawString(F("Init file system..."), 0, 0);
    fle = LittleFS.open("/save.dat", "a");
    for(uint16_t i=0; i<CART_SIZE; i++) fle.write(0);
    fle.close();
    tft.fillScreen(TFT_BLACK);
  }

  loadFS();
  
  sigmaDeltaSetup(0, F_CPU / 256);
  sigmaDeltaAttachPin(SOUNDPIN);
  sigmaDeltaEnable();

  sound_dac = 0;

  noInterrupts();
  timer1_isr_init();
  timer1_attachInterrupt(sound_ISR);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
  timer1_write(80 * 1000000 / SAMPLING_RATE);//AUDIO_SAMPLE_RATE);
  interrupts();
  
  //Serial.println(ESP.getFreeHeap());

  tft.setAddrWindow(0, 0, 128, 128);
}



#define FRAME_TIME 15000
uint32_t nextScreen;
bool everysecondgetkey;

void loop() { 

   if(everysecondgetkey) readkeys();
   everysecondgetkey = !everysecondgetkey;

   nextScreen = micros() + FRAME_TIME ;
   
   gb_run_frame(&gb);
 
  if (cartSaveFlag == 1 && millis() - timeToSave > WRITE_DELAY){
    //Serial.println("Saving");
    if(realSaveStruct.marker){
      tft.drawString("S", 0, 0);
      paletteAndOffsetChangeFlag=1;}
      
    previousSoundFlag = realSaveStruct.soundFlag;
    realSaveStruct.soundFlag=0;

    loadFS();

    realSaveStruct.soundFlag = previousSoundFlag;
    cartSaveFlag = 0;
  }

  while(nextScreen > micros());
};
