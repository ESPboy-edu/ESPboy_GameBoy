/*
GameBoy emulator for ESPboy.

You are able to try Nintendo retro games like SuperMario, Zelda, Pokemon, etc! Pressing both side buttons, you can adjust view port (original GB screen resolution is 160х144 but it's only 128x128 window visible on ESPboy display).

Peanut-GB core by deltabeard
https://github.com/deltabeard/Peanut-GBhttps://github.com/deltabeard/Peanut-GB

RomanS has screwed it to ESPboy
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-funhttps://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun

MIT license
*/

#pragma GCC optimize ("-Ofast")
#pragma GCC push_options

#include "Arduino.h"
#include <ESP_EEPROM.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_MCP4725.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include "ESPboyLogo.h"
#include "sound.h"
#include "peanut_gb.c"
#include <sigma_delta.h>


#define SAVE_TOKEN 0xFC
#define SAVE_TOKEN_OFFSET 1
#define SAVE_SOUND_OFFSET 2
#define SAVE_PALETTE_OFFSET 3
#define SAVE_X_OFFSET 4
#define SAVE_Y_OFFSET 5


//#include "GAMES/rom_1.h"  //test rom
#include "GAMES/rom_2.h"  //super mario land
//#include "GAMES/rom_3.h"  //tetris
//#include "GAMES/rom_4.h"  //lemmings
//#include "GAMES/rom_5.h"  //kirby's dream land
//#include "GAMES/rom_6.h"  //mega man
//#include "GAMES/rom_7.h"  //zelda
//#include "GAMES/rom_8.h"  //prince of persia
//#include "GAMES/rom_9.h"  //contra
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
//#include "GAMES/rom_26.h"   //Mortal Combat
//#include "GAMES/rom_27.h"   //Mortal Combat 2


#define GB_ROM  rom

#define CART_MEM       4000
#define SAVECARTOFFSET 300//for donkey kong land 2600 for others 300
#define WRITE_DELAY    2000

int16_t cartMemOffset1;
uint8_t cartMemFlag;
uint8_t soundFlag = 1;
static uint8_t cartSaveFlag = 0;
static uint32_t timeEEPROMcommete;
static int8_t paletteNo = 2;
static int8_t paletteChangeFlag = 1;
uint16_t offset_x=16, offset_y=8;

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
Adafruit_MCP4725 dac;
TFT_eSPI tft = TFT_eSPI(); 

static struct gb_s gb;
enum gb_init_error_e ret;


uint8_t inline getKeys() __attribute__((always_inline));
uint8_t inline getKeys() { return (~mcp.readGPIOAB() & 255); }

void IRAM_ATTR readkeys(){
 static uint8_t nowkeys;
  nowkeys = getKeys();
  gb.direct.joypad_bits.a = (nowkeys&PAD_ACT)?0:1;
  gb.direct.joypad_bits.b = (nowkeys&PAD_ESC)?0:1;
  gb.direct.joypad_bits.up = (nowkeys&PAD_UP)?0:1;
  gb.direct.joypad_bits.down = (nowkeys&PAD_DOWN)?0:1;
  gb.direct.joypad_bits.left = (nowkeys&PAD_LEFT)?0:1;
  gb.direct.joypad_bits.right = (nowkeys&PAD_RIGHT)?0:1;
  gb.direct.joypad_bits.start = (nowkeys&PAD_LFT)?0:1;
  gb.direct.joypad_bits.select = (nowkeys&PAD_RGT)?0:1;
  if (nowkeys&PAD_LFT && nowkeys&PAD_RGT) adjustOffset();
}

void adjustOffset(){
  static uint8_t nowkeys;
  while(1){
    nowkeys = getKeys();
    if (nowkeys&PAD_UP && offset_y>0) {offset_y--; gb_run_frame(&gb);}
    if (nowkeys&PAD_DOWN && offset_y<16) {offset_y++; gb_run_frame(&gb);}
    if (nowkeys&PAD_LEFT && offset_x>0) {offset_x--; gb_run_frame(&gb);}
    if (nowkeys&PAD_RIGHT && offset_x<32) {offset_x++; gb_run_frame(&gb);}
    if (nowkeys&PAD_ACT) {soundFlag = !soundFlag; gb_run_frame(&gb); delay(100);}
    if (nowkeys&PAD_RGT) {paletteNo++; if(paletteNo>2)paletteNo=0; paletteChangeFlag=1; gb_run_frame(&gb); gb_run_frame(&gb);}
    if (nowkeys&PAD_LFT) {paletteNo--; if(paletteNo<0)paletteNo=2; paletteChangeFlag=1; gb_run_frame(&gb); gb_run_frame(&gb);}
    if (nowkeys&PAD_ESC) {saveparameters(); break;}
    tft.drawString(F("Adjusting LCD"), 24, 60);
    if (soundFlag) tft.drawString(F("Sound ON "), 0, 0);
    else tft.drawString(F("Sound OFF"), 0, 0);
    tft.drawString(F("Palette No  "), 0, 10);
    tft.drawString((String)paletteNo, 66, 10);
    delay(200);
  }
};


uint8_t IRAM_ATTR gb_rom_read(struct gb_s *gb, const uint32_t addr){
  return pgm_read_byte(GB_ROM+addr);
}

uint8_t gb_cart_ram_read(struct gb_s *gb, const uint32_t addr){

  if (cartMemFlag == 0) {
     cartMemOffset1 = addr - SAVECARTOFFSET; 
     cartMemFlag = 1;
   }

  //Serial.print("Read "); 
  //Serial.println(addr-cartMemOffset1); 
  tft.drawString("R", 0, 0);
  
  if (addr-cartMemOffset1 >0 && addr-cartMemOffset1<CART_MEM) return EEPROM.read(addr-cartMemOffset1);
  else {
    Serial.print(F("Error! Out of cart memory read ")); 
    Serial.println(addr-cartMemOffset1);
    tft.drawString("ER!", 0, 0);}
  return(0);
}


void gb_cart_ram_write(struct gb_s *gb, const uint32_t addr, const uint8_t val){
  if (cartMemFlag == 0) {
     cartMemOffset1 = addr - SAVECARTOFFSET;
     cartMemFlag = 1;}

//  Serial.print("Write "); 
//  Serial.println(addr-cartMemOffset1); 
  tft.drawString("W", 0, 0);
  
  if (addr-cartMemOffset1>0 && addr-cartMemOffset1<CART_MEM){
    EEPROM.write(addr-cartMemOffset1, val);
    cartSaveFlag = 1;
    timeEEPROMcommete = millis();
  }
  else {
    Serial.print(F("Error! Out of cart memory write "));
    Serial.println(addr-cartMemOffset1);
    tft.drawString("EW!", 0, 0);
    }
}


void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val){
	const char* gb_err_str[4] = {
		"UNKNOWN",
		"INVALID OPCODE",
		"INVALID READ",
		"INVALID WRITE"
	};

	Serial.print(F("Error "));
	Serial.print(gb_err);
	Serial.print(F(" occurred:  "));
	Serial.println(gb_err >= GB_INVALID_MAX ?gb_err_str[0] : gb_err_str[gb_err]);
	Serial.print(F("At Address "));
	Serial.println(val,HEX);
}


/*
void IRAM_ATTR lcd_draw_line(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line){
  static uint_fast8_t x;
  static uint_fast8_t pixels_x;
  static uint8_t uiBuff[128];
  
 // static const uint8_t palette8[] = {0x00, 0x52, 0xA5, 0xFF};
  static const uint8_t palette8[] = {0xFF, 0xA5, 0x52, 0x00};
  
  if(line >= offset_y && line < 128 + offset_y){
    pixels_x = offset_x;
    for (x = 0; x < 128; x++)
      uiBuff[x] = palette8[pixels[pixels_x++]&3];
    tft.pushImage(0, line-offset_y, 128, 1, uiBuff, true);
  }
}
*/



void IRAM_ATTR lcd_draw_line(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line){
  static uint_fast32_t x;
  static uint_fast32_t pixels_x;
  static uint16_t uiBuff[128];
  const static uint16_t palette0[4] = { 0x7FFF, 0x329F, 0x001F, 0x0000 }; // OBJ0
  const static uint16_t palette1[4] = { 0x7FFF, 0x3FE6, 0x0200, 0x0000 }; // OBJ1
  const static uint16_t palette2[4] = { 0x7FFF, 0x7EAC, 0x40C0, 0x0000 }; // BG
  const static uint16_t *paletteN[] = {palette0, palette1, palette2};
  static uint16_t *paletteNN;

  if (paletteChangeFlag) {paletteNN = (uint16_t *)paletteN[paletteNo]; paletteChangeFlag=0;}
  
  if(line >= offset_y && line < 128 + offset_y){
    pixels_x = offset_x;
    for (x = 0; x < 128; x++)
      uiBuff[x] = paletteNN[pixels[pixels_x++]&3];
    tft.setAddrWindow(0, line-offset_y, 128, 1);
    tft.pushPixels(uiBuff, 128);  
   }
}


volatile uint8_t sound_dac;

void IRAM_ATTR sound_ISR(){

  if(soundFlag){
    sigmaDeltaWrite(0, sound_dac);
    sound_dac = audio_update();}
/*
  static float sound_output[2];
  audio_callback(NULL,(uint8_t*)&sound_output,sizeof(sound_output));
  sound_dac = (uint8_t)(((sound_output[0]+1.0f)+(sound_output[1]+1.0f))*63.0f);*/
}


void loadparameters(){
  if(EEPROM.read(CART_MEM + SAVE_TOKEN_OFFSET) != SAVE_TOKEN)
    saveparameters();
  else{
    soundFlag = EEPROM.read(CART_MEM + SAVE_SOUND_OFFSET);
    paletteNo = EEPROM.read(CART_MEM + SAVE_PALETTE_OFFSET);
    offset_x = EEPROM.read(CART_MEM + SAVE_X_OFFSET);
    offset_y = EEPROM.read(CART_MEM + SAVE_Y_OFFSET);
   }  
}


void saveparameters(){
  EEPROM.write(CART_MEM + SAVE_TOKEN_OFFSET, SAVE_TOKEN);
  EEPROM.write(CART_MEM + SAVE_SOUND_OFFSET, soundFlag);
  EEPROM.write(CART_MEM + SAVE_PALETTE_OFFSET, paletteNo);
  EEPROM.write(CART_MEM + SAVE_X_OFFSET, offset_x);
  EEPROM.write(CART_MEM + SAVE_Y_OFFSET, offset_y);
  EEPROM.commit();
}


void setup() {
  system_update_cpu_freq(SYS_CPU_160MHZ);
    
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println(ESP.getFreeHeap());

//EEPROM init (for game cart RAM)  
  EEPROM.begin(CART_MEM+5);
  
  cartMemFlag = 0;

//DAC init 
  dac.begin(0x60);
  delay (100);
  dac.setVoltage(0, false);

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
  /*tone(SOUNDPIN, 200, 100);
  delay(100);
  tone(SOUNDPIN, 100, 100);
  delay(100);
  noTone(SOUNDPIN);*/

// TFT init
  mcp.pinMode(CSTFTPIN, OUTPUT);
  mcp.digitalWrite(CSTFTPIN, LOW);
  tft.begin();
  delay(100);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

// draw ESPboylogo
  tft.drawXBitmap(30, 20, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(F("GameBoy emulator"), 20, 95);
  for (uint8_t i = 0; i<100; i++){
    dac.setVoltage(i* 10, false);
    delay(7);}
  dac.setVoltage(4095, true);
  delay(1000);

// clear screen
  tft.fillScreen(TFT_BLACK);

  WiFi.mode(WIFI_OFF);


// init game boy evulator
   ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, NULL);

	if(ret != GB_INIT_NO_ERROR){
		Serial.print("Error: ");
		Serial.println(ret);}

	  gb_init_lcd(&gb, &lcd_draw_line);
    gb.direct.interlace = 0;
    gb.direct.frame_skip = 1;

  //setup sound

  //audio_init();

  sigmaDeltaSetup(0, F_CPU / 256);
  sigmaDeltaAttachPin(SOUNDPIN);
  sigmaDeltaEnable();

  sound_dac = 0;

  noInterrupts();
  timer1_isr_init();
  timer1_attachInterrupt(sound_ISR);
  timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
  timer1_write(ESP.getCpuFreqMHz() * 1000000 / SAMPLING_RATE);//AUDIO_SAMPLE_RATE);
  interrupts();
  
  Serial.println(ESP.getFreeHeap());

  loadparameters();
}


void loop() {
   //static String fps;
   //uint32_t tme = millis();
 
   gb_run_frame(&gb);
   readkeys();
 
   //fps = 1000/(millis() - tme);
   //tft.drawString(fps, 0, 120);
 
  if (cartSaveFlag == 1 && millis() - timeEEPROMcommete > WRITE_DELAY){
    Serial.println("Commiting!");
    cartSaveFlag = 0;
    EEPROM.commit();
  }
}
