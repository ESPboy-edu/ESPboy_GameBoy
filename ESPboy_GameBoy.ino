/*
GameBoy emulator for ESPboy.

You are able to try Nintendo retro games like SuperMario, Zelda, Pokemon, etc! 
Pressing both side buttons, you can adjust view port (original GB screen resolution is 160х144 but it's only 128x128 window visible on ESPboy display).

Peanut-GB core by deltabeard
https://github.com/deltabeard/Peanut-GBhttps://github.com/deltabeard/Peanut-GB

https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-funhttps://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun

MIT license
*/

#pragma GCC optimize ("-O3")
#pragma GCC push_options

#include "Arduino.h"

#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"
//#include "lib/ESPboyTerminalGUI.h"
//#include "lib/ESPboyTerminalGUI.cpp"
//#include "lib/ESPboyOTA2.h"
//#include "lib/ESPboyOTA2.cpp"

#include <ESP_EEPROM.h>
#include "sound.h"
#include "peanut_gb.c"
#include "LittleFS.h"
#include "nbSPI.h"
#include <sigma_delta.h>

ESPboyInit myESPboy;
//ESPboyTerminalGUI *terminalGUIobj = NULL;
//ESPboyOTA2 *OTA2obj = NULL;


//------------------------------------sprite_1-----------------------------------------------------------sprite_2-------------------------------------------------------------background----------
//  const uint16_t palette0[] = { 0x79DF, 0x2D7E, 0x6D2B, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x3FE6, 0x0200, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A }; // PeanutGB
//  const uint16_t palette1[] = { 0x7FFF, 0x7EAC, 0x40C0, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x3FE6, 0x0200, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A }; // OBJ0
  const uint16_t palette2[] = { 0x7FFF, 0x3FE6, 0x0200, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x3FE6, 0x0200, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x3FE6, 0x0200, 0x0000 }; // OBJ1
  const uint16_t palette3[] = { 0x7FFF, 0x7EAC, 0x40C0, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x7EAC, 0x40C0, 0x0000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x7EAC, 0x40C0, 0x0000 }; // BG
  const uint16_t palette4[] = { 0x6ACE, 0x279D, 0xE46B, 0x613A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A }; //nostalgia
//  const uint16_t palette5[] = { 0x5685, 0x8F9D, 0x6843, 0x6541, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5685, 0x8F9D, 0x6843, 0x6541, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5685, 0x8F9D, 0x6843, 0x6541 }; //greeny
  const uint16_t palette6[] = { 0x16F6, 0xC9D3, 0xC091, 0x8041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x16F6, 0xC9D3, 0xC091, 0x8041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x16F6, 0xC9D3, 0xC091, 0x8041 }; //reddy
//  const uint16_t palette7[] = { 0x79DF, 0x2D7E, 0x6D2B, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x79DF, 0x2D7E, 0x6D2B, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x79DF, 0x2D7E, 0x6D2B, 0x6308 }; //retro lcd
//  const uint16_t palette8[] = { 0x1F87, 0x795C, 0x7C72, 0x6959, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1F87, 0x795C, 0x7C72, 0x6959, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1F87, 0x795C, 0x7C72, 0x6959 };  //WISH GB
  const uint16_t palette9[] = { 0xDDF7, 0xB7C5, 0xCE52, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xDDF7, 0xB7C5, 0xCE52, 0x6308, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xDDF7, 0xB7C5, 0xCE52, 0x6308 };  //HOLLOW
//  const uint16_t palette10[] ={ 0x9B9F, 0xB705, 0xF102, 0x4A01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x9B9F, 0xB705, 0xF102, 0x4A01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x9B9F, 0xB705, 0xF102, 0x4A01 };  //BLK AQU4
  const uint16_t palette11[] ={ 0x49CD, 0x099B, 0x0549, 0x4320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x49CD, 0x099B, 0x0549, 0x4320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x49CD, 0x099B, 0x0549, 0x4320 };  //GOLD GB
//  const uint16_t palette12[] ={ 0x719F, 0x523D, 0xEE42, 0x0629, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x719F, 0x523D, 0xEE42, 0x0629, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x719F, 0x523D, 0xEE42, 0x0629 };  //NYMPH GB
//  const uint16_t palette13[] ={ 0x16F6, 0xC9D3, 0xC091, 0x8041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x16F6, 0xC9D3, 0xC091, 0x8041, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5685, 0x8F9D, 0x6843, 0x6541 };  //BOOTLEG BY PIXELSHIFT
//  const uint16_t palette14[] ={ 0x49CD, 0x099B, 0x0549, 0x4320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x49CD, 0x099B, 0x0549, 0x4320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x6ACE, 0x279D, 0xE46B, 0x613A };  //nostalgia+GOLD GB

  const uint16_t *paletteN[] = {/*palette0, palette1,*/ palette2, palette3, palette4, /*palette5,*/ palette6, /*palette7, palette8,*/ palette9, /*palette10, */palette11/*, palette12, palette13, palette14*/};
  uint16_t *paletteNN;
  

//#include "GAMES/rom_1.h"  //test rom
//#define APP_MARKER 0xCC01
#include "GAMES/rom_2.h"  //super mario land
#define APP_MARKER 0xCC02
//#include "GAMES/rom_3.h"  //tetris
//#define APP_MARKER 0xCC03
//#include "GAMES/rom_4.h"  //lemmings
//#define APP_MARKER 0xCC04
//#include "GAMES/rom_5.h"  //kirby's dream land
//#define APP_MARKER 0xCC05
//#include "GAMES/rom_6.h"  //mega man
//#define APP_MARKER 0xCC06
//#include "GAMES/rom_7.h"  //zelda
//#define APP_MARKER 0xCC07
//#include "GAMES/rom_8.h"  //prince of persia
//#define APP_MARKER 0xCC08
//#include "GAMES/rom_9.h"  //contra
//#define APP_MARKER 0xCC09
//#include "GAMES/rom_10.h" //Felix the cat
//#define APP_MARKER 0xCC10
//#include "GAMES/rom_11.h" //Pokemon
//#define APP_MARKER 0xCC11
//#include "GAMES/rom_12.h" //Castelian
//#define APP_MARKER 0xCC12
//#include "GAMES/rom_13.h" //Castelvania
//#define APP_MARKER 0xCC13
//#include "GAMES/rom_14.h" //Donkey Kong Land
//#define APP_MARKER 0xCC14
//#include "GAMES/rom_15.h" //Double dragon
//#define APP_MARKER 0xCC15
//#include "GAMES/rom_16.h" //R-type
//#define APP_MARKER 0xCC16
//#include "GAMES/rom_17.h" //Mega man III
//#define APP_MARKER 0xCC17
//#include "GAMES/rom_18.h" //R-type II
//#define APP_MARKER 0xCC18
//#include "GAMES/rom_19.h" //nemezis
//#define APP_MARKER 0xCC19
//#include "GAMES/rom_20.h" //ninja gaiden shadow
//#define APP_MARKER 0xCC20
//#include "GAMES/rom_21.h" //spy vs spy
//#define APP_MARKER 0xCC21
//#include "GAMES/rom_22.h" //Robocop
//#define APP_MARKER 0xCC22
//#include "GAMES/rom_23.h" //Solar Striker
//#define APP_MARKER 0xCC23
//#include "GAMES/rom_26.h" //Mortal Combat
//#define APP_MARKER 0xCC24
//#include "GAMES/rom_27.h" //Mortal Combat 2
//#define APP_MARKER 0xCC25
//#include "GAMES/rom_28.h" //Pokemon blue
//#define APP_MARKER 0xCC26
//#include "GAMES/rom_29.h" //Q-bert
//#define APP_MARKER 0xCC27
//#include "GAMES/rom_30.h" //PacMan
//#define APP_MARKER 0xCC28
//#include "GAMES/rom_31.h" //Adventure Island
//#define APP_MARKER 0xCC29
//#include "GAMES/rom_32.h" //Adventure Island II
//#define APP_MARKER 0xCC30
//#include "GAMES/rom_33.h" //Castlevania II - Belmont's Revenge
//#define APP_MARKER 0xCC31
//#include "GAMES/rom_34.h" //Chase H.Q. 
//#define APP_MARKER 0xCC32
//#include "GAMES/rom_35.h" //Speedy Gonzales 
//#define APP_MARKER 0xCC33
//#include "GAMES/rom_36.h" //Star Wars - The Empire Strikes Back 
//#define APP_MARKER 0xCC34
//#include "GAMES/rom_37.h" //Super Mario Land 2 - 6 Golden Coins
//#define APP_MARKER 0xCC35
//#include "GAMES/rom_38.h" //Super Off Road 
//#define APP_MARKER 0xCC36
//#include "GAMES/rom_39.h" //Pocket_Monsters_-_Green_Version_J_V1.0_S_patched
//#define APP_MARKER 0xCC37
//#include "GAMES/rom_40.h" //bomberman
//#define APP_MARKER 0xCC38
//#include "GAMES/rom_41.h" //super chase hq
//#define APP_MARKER 0xCC39
//#include "GAMES/rom_42.h" //Burai Fighter Deluxe
//#define APP_MARKER 0xCC40
//#include "GAMES/rom_50.h" //pokemon blue
//#define APP_MARKER 0xCC41


#define WRITE_DELAY 2000
#define CART_SIZE 10000
#define GB_ROM rom

File fle;

uint8_t  *cartSave;
uint8_t  previousSoundFlag;
uint8_t  paletteAndOffsetChangeFlag = 0;
uint32_t timeToSave;
bool     cartSaveFlag = 0;

int32_t maxout=0;
int32_t minch1=0, minch2=0, minch3=0, minch4=0;
uint32_t divider=64;

struct SaveStruct{
  uint32_t appMarker = APP_MARKER;
  bool     soundFlag = 1;
  bool     forceRescale = 0;
  uint8_t  paletteNo = 3;
  uint8_t  offset_x = 16;
  uint8_t  offset_y = 8;
};


SaveStruct defaultSaveStruct, realSaveStruct;

struct gb_s *gb;
enum gb_init_error_e ret;


void writeFS(){
    delay(100);
    fle = LittleFS.open("/save.dat", "r+");
    for(uint16_t i=0; i<CART_SIZE; i++) 
      fle.write(cartSave[i]);
    fle.close();
};



void loadFS(){
    delay(100);
    fle = LittleFS.open("/save.dat", "r+");
    for(uint16_t i=0; i<CART_SIZE; i++) 
      cartSave[i] = fle.read();
    fle.close();
};


void inline __attribute__((always_inline)) IRAM_ATTR readkeys(){
 static uint8_t nowkeys;
  nowkeys = myESPboy.getKeys();
  gb->direct.joypad_bits.a = (nowkeys&PAD_ACT)?0:1;
  gb->direct.joypad_bits.b = (nowkeys&PAD_ESC)?0:1;
  gb->direct.joypad_bits.up = (nowkeys&PAD_UP)?0:1;
  gb->direct.joypad_bits.down = (nowkeys&PAD_DOWN)?0:1;
  gb->direct.joypad_bits.left = (nowkeys&PAD_LEFT)?0:1;
  gb->direct.joypad_bits.right = (nowkeys&PAD_RIGHT)?0:1;
  gb->direct.joypad_bits.start = (nowkeys&PAD_LFT)?0:1;
  gb->direct.joypad_bits.select = (nowkeys&PAD_RGT)?0:1;
  if (nowkeys&PAD_LFT && nowkeys&PAD_RGT && cartSaveFlag == 0) adjustOffset();
}


void adjustOffset(){
  static uint8_t nowkeys;
  paletteAndOffsetChangeFlag = 1;
  previousSoundFlag = realSaveStruct.soundFlag;
  realSaveStruct.soundFlag=0;
  while(myESPboy.getKeys()) delay(100);
  while(1){
    delay(200);
    myESPboy.tft.drawString(F("Adjusting LCD"), 24, 60);
    myESPboy.tft.drawString(F("up/down/left/right"), 8, 70);  
    myESPboy.tft.drawString(F("press B to go back"), 9, 90);  
    if (previousSoundFlag) myESPboy.tft.drawString(F("Sound ON "), 0, 0);
    else myESPboy.tft.drawString(F("Sound OFF"), 0, 0);
    myESPboy.tft.drawString(F("Palette N  "), 0, 10);
    myESPboy.tft.drawString((String)realSaveStruct.paletteNo, 66, 10);
    if (realSaveStruct.forceRescale) myESPboy.tft.drawString(F("Rescale ON "), 0, 20);
    else myESPboy.tft.drawString(F("Rescale OFF"), 0, 20);
    while(!(nowkeys = myESPboy.getKeys())) delay(50);
    if (!realSaveStruct.forceRescale){
      if (nowkeys&PAD_UP && realSaveStruct.offset_y>0) realSaveStruct.offset_y--;
      if (nowkeys&PAD_DOWN && realSaveStruct.offset_y<16) realSaveStruct.offset_y++;
      if (nowkeys&PAD_LEFT && realSaveStruct.offset_x>0) realSaveStruct.offset_x--;
      if (nowkeys&PAD_RIGHT && realSaveStruct.offset_x<32) realSaveStruct.offset_x++;}
    if (nowkeys&PAD_ACT) {previousSoundFlag = !previousSoundFlag;}
    if (nowkeys&PAD_RGT) {realSaveStruct.paletteNo++; if(realSaveStruct.paletteNo==sizeof(paletteN)/sizeof(uint32_t *)) realSaveStruct.paletteNo=0;}
    if (nowkeys&PAD_LFT) {realSaveStruct.forceRescale = !realSaveStruct.forceRescale;}
    if (nowkeys&PAD_ESC) {break;}

    gb_run_frame(gb);
    gb_run_frame(gb);
  }

  realSaveStruct.soundFlag = previousSoundFlag;
  saveparameters();
  paletteAndOffsetChangeFlag = 0;
  myESPboy.tft.fillScreen(TFT_BLACK);
};


uint8_t inline __attribute__((always_inline)) IRAM_ATTR gb_rom_read(struct gb_s *gb, const uint32_t addr){
  return pgm_read_byte(GB_ROM+addr);
}


uint8_t gb_cart_ram_read(struct gb_s *gb, const uint32_t addr){
  if (addr<CART_SIZE){
    return cartSave[addr];}
  else {
    //Serial.print("Read fail addr: "); Serial.println(addr);
  }
  return(0);
}


void gb_cart_ram_write(struct gb_s *gb, const uint32_t addr, const uint8_t val){
  
  cartSaveFlag = 1;
  timeToSave = millis(); 
  
  if (addr<CART_SIZE){
    cartSave[addr] = val;
  }
  else {     
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
  uint16_t uiBuff1[128] __attribute__((aligned(32)));
  uint16_t uiBuff2[128] __attribute__((aligned(32)));
  uint8_t uiBuff3[160];
  uint16_t *currentBuf;
  uint16_t *currentBuf2;
  uint16_t *currentBuf3;
  uint8_t prevLine=0;
  bool flipBuf;
  bool skippedLineFlag = 0;
  uint8_t *pix;
  

void IRAM_ATTR lcd_draw_line(struct gb_s *gb, const uint8_t *pixels, uint_fast8_t line){
  if (paletteAndOffsetChangeFlag) {
     paletteNN = (uint16_t *)paletteN[realSaveStruct.paletteNo];
     offset_xx = realSaveStruct.offset_x;
     offset_yy = realSaveStruct.offset_y;
   }

  if(!realSaveStruct.forceRescale){
    if (line < offset_yy || line > offset_yy+127) return;}
  else{
    if (!(line%8)) {memcpy(uiBuff3, pixels, 160); skippedLineFlag = 1; return;}
    line -= line>>3;}
  
  if(line != prevLine+1){
    while(nbSPI_isBusy());
    if (!realSaveStruct.forceRescale) myESPboy.tft.setAddrWindow(0, line-offset_yy, 128, 128);
    else  myESPboy.tft.setAddrWindow(0, line, 128, 128);}

    currentBuf = flipBuf?uiBuff1:uiBuff2;
    currentBuf2 = currentBuf;
    currentBuf3 = &currentBuf[128];

    if (!realSaveStruct.forceRescale) pix = (uint8_t *)&pixels[offset_xx];
    else pix = (uint8_t *)&pixels[0];

  if(skippedLineFlag){
    skippedLineFlag=0;  
    uint8_t *pixx = &uiBuff3[0];
    while (currentBuf != currentBuf3){  
      *currentBuf++ = paletteNN[((*pix)+(*pixx))>>1]; pix++; pixx++;
      *currentBuf++ = paletteNN[((*pix)+(*pixx))>>1]; pix++; pixx++;
      *currentBuf++ = paletteNN[((*pix)+(*pixx))>>1]; pix++; pixx++;
      if(!realSaveStruct.forceRescale) {*currentBuf++ = paletteNN[((*pix)+(*pixx))>>1]; pix++; pixx++;}
      else {*currentBuf++ = paletteNN[(pix[0]+pix[1]+pixx[0]+pixx[1])>>2]; pix+= 2; pixx+= 2;}
      *currentBuf++ = paletteNN[((*pix)+(*pixx))>>1]; pix++; pixx++;
      *currentBuf++ = paletteNN[((*pix)+(*pixx))>>1]; pix++; pixx++;
      *currentBuf++ = paletteNN[((*pix)+(*pixx))>>1]; pix++; pixx++;
      if(!realSaveStruct.forceRescale) {*currentBuf++ = paletteNN[((*pix)+(*pixx))>>1]; pix++; pixx++;}
      else {*currentBuf++ = paletteNN[(pix[0]+pix[1]+pixx[0]+pixx[1])>>2]; pix+= 2; pixx+= 2;}
    }
  }
  else{      
    while (currentBuf != currentBuf3){
      *currentBuf++ = paletteNN[(*pix++)];
      *currentBuf++ = paletteNN[(*pix++)];
      *currentBuf++ = paletteNN[(*pix++)];
      if(!realSaveStruct.forceRescale) *currentBuf++ = paletteNN[(*pix++)];
      else {*currentBuf++ = paletteNN[(pix[0]+pix[1])>>1]; pix+= 2;}
      *currentBuf++ = paletteNN[(*pix++)];
      *currentBuf++ = paletteNN[(*pix++)];
      *currentBuf++ = paletteNN[(*pix++)];
      if(!realSaveStruct.forceRescale) *currentBuf++ = paletteNN[(*pix++)];
      else {*currentBuf++ = paletteNN[(pix[0]+pix[1])>>1]; pix+= 2;}
    }
  }

    while(nbSPI_isBusy()); nbSPI_writeBytes((uint8_t*)currentBuf2, 256);   //unsafe but fast render method
    //myESPboy.tft.pushColors(currentBuf2, 128, false); //safe but slow render method 

    prevLine = line;
    flipBuf = !flipBuf;
}



void IRAM_ATTR sound_ISR(){
  if(realSaveStruct.soundFlag)
    sigmaDeltaWrite(0, audio_update());
}


bool loadparameters(){
  EEPROM.get(0, realSaveStruct);
  if(realSaveStruct.appMarker != APP_MARKER){
    EEPROM.put(0, defaultSaveStruct);
    EEPROM.commit();
    realSaveStruct = defaultSaveStruct;
    return(1);
  }
  return(0);
};


void saveparameters(){
  delay(100);
  realSaveStruct.soundFlag = previousSoundFlag;
  EEPROM.put(0, realSaveStruct);
  EEPROM.commit();
};


void setup() {
  //system_update_cpu_freq(SYS_CPU_160MHZ);
    
  //Serial.begin(115200);
  
  //Serial.println();
  //Serial.println(ESP.getFreeHeap());

  myESPboy.begin("GameBoy emu 2.4");

//Check OTA2
//  if (myESPboy.getKeys()&PAD_ACT || myESPboy.getKeys()&PAD_ESC) { 
//     terminalGUIobj = new ESPboyTerminalGUI(&myESPboy.tft, &myESPboy.mcp);
//     OTA2obj = new ESPboyOTA2(terminalGUIobj);
//  }


  WiFi.mode(WIFI_OFF);
  Wire.setClock(400000L);
  
  myESPboy.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  myESPboy.tft.fillScreen(TFT_BLACK);

//EEPROM init (for game cart RAM)  
  EEPROM.begin(sizeof(SaveStruct));

  // File system init
  cartSave = (uint8_t *)malloc (CART_SIZE+1);
    
  LittleFS.begin();
  if (loadparameters()){
    myESPboy.tft.drawString(F("Init file system..."), 0, 0);
    LittleFS.format();
    fle = LittleFS.open("/save.dat", "a");
    for(uint16_t i=0; i<CART_SIZE; i++) fle.write(0);
    fle.close();
    myESPboy.tft.fillScreen(TFT_BLACK);
    myESPboy.tft.drawString(F("<< press both side >>"), 0, 15);
    myESPboy.tft.drawString(F("buttons for settings"), 3, 30);
    delay(3000);
    myESPboy.tft.fillScreen(TFT_BLACK);
  }

  loadFS();

  paletteNN = (uint16_t *)paletteN[realSaveStruct.paletteNo];
  offset_xx = realSaveStruct.offset_x;
  offset_yy = realSaveStruct.offset_y;

// init game boy emulator
   gb = new gb_s;

   ret = gb_init(gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, NULL);

  //if(ret != GB_INIT_NO_ERROR){
    //Serial.print("Error: ");
    //Serial.println(ret);}

    gb_init_lcd(gb, &lcd_draw_line);
    gb->direct.interlace = 0;
    gb->direct.frame_skip = 1;
  
  sigmaDeltaSetup(0, F_CPU / 256);
  sigmaDeltaAttachPin(SOUNDPIN);
  sigmaDeltaEnable();
  noInterrupts();
  timer1_isr_init();
  timer1_attachInterrupt(sound_ISR);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
  timer1_write(80 * 1000000 / SAMPLING_RATE);//AUDIO_SAMPLE_RATE);
  interrupts();

  //Serial.println(ESP.getFreeHeap());

  myESPboy.tft.setAddrWindow(0, 0, 128, 128);
}



#define FRAME_TIME 16600
uint32_t nextScreen;
uint8_t passGetkey;

void loop() {
   nextScreen = micros() + FRAME_TIME ;
   
   if(passGetkey > 5){
      readkeys();
      divider = (maxout>>8)+1;
      passGetkey=0;}
   passGetkey++;
   
   gb_run_frame(gb);
 
  if (cartSaveFlag && millis() - timeToSave > WRITE_DELAY){
    previousSoundFlag = realSaveStruct.soundFlag;
    realSaveStruct.soundFlag=0;
    writeFS();
    realSaveStruct.soundFlag = previousSoundFlag;
    cartSaveFlag = 0;
  }

  while(nextScreen > micros());
};
