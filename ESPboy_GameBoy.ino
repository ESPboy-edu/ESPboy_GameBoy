/*
GameBoy emulator for ESPboy.

You are able to try Nintendo retro games like SuperMario, Zelda, Pokemon, etc! Pressing both side buttons, you can adjust view port (original GB screen resolution is 160х144 but it's only 128x128 window visible on ESPboy display).

Peanut-GB core by deltabeard
https://github.com/deltabeard/Peanut-GBhttps://github.com/deltabeard/Peanut-GB

RomanS has screwed it to ESPboy
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-funhttps://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun

MIT license
*/

#include "Arduino.h"
#include <ESP_EEPROM.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_MCP4725.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include "ESPboyLogo.h"
#include "peanut_gb.c"
#include "rom.h"
#include "ESPboyOTA.h"

//#define GB_ROM rom1   //test rom
#define GB_ROM rom2   //super mario land
//#define GB_ROM rom3   //tetris
//#define GB_ROM rom4   //lemmings
//#define GB_ROM rom5   //kirby's dream land
//#define GB_ROM rom6   //mega man
//#define GB_ROM rom7   //zelda
//#define GB_ROM rom8   //prince of persia


#define CART_MEM        2960 //16384
int16_t cartMemOffset1;
int16_t cartMemOffset2;
uint8_t cartMemFlag;
static uint8_t cartSaveFlag = 0;
static uint32_t timeEEPROMcommete;

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
ESPboyOTA* OTAobj = NULL;

uint8_t OFFSET_X=16, OFFSET_Y=8;

static struct gb_s gb;
enum gb_init_error_e ret;


uint8_t inline getKeys() { return (~mcp.readGPIOAB() & 255); }

void readkeys(){
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
    if (nowkeys&PAD_UP && OFFSET_Y>0) {OFFSET_Y--; gb_run_frame(&gb);}
    if (nowkeys&PAD_DOWN && OFFSET_Y<16) {OFFSET_Y++; gb_run_frame(&gb);}
    if (nowkeys&PAD_LEFT && OFFSET_X>0) {OFFSET_X--; gb_run_frame(&gb);}
    if (nowkeys&PAD_RIGHT && OFFSET_X<32) {OFFSET_X++; gb_run_frame(&gb);}
    if (nowkeys&PAD_ACT || nowkeys&PAD_ESC) break;
    tft.drawString(F("Adjusting LCD"), 24, 60);
    delay(30);
  }
};


uint8_t gb_rom_read(struct gb_s *gb, const uint32_t addr){
  return pgm_read_byte(GB_ROM+addr);
}

uint8_t gb_cart_ram_read(struct gb_s *gb, const uint32_t addr){

  if (cartMemFlag == 0) {
     cartMemOffset1 = addr - 100; 
     EEPROM.write(0, cartMemOffset1>>8);
     EEPROM.write(1, cartMemOffset1 & 255);
     EEPROM.write(CART_MEM-3, cartMemOffset1>>8);
     EEPROM.write(CART_MEM-2, cartMemOffset1&255);
     cartMemFlag = 1;
     cartSaveFlag = 1;
     timeEEPROMcommete = millis();
   }

  Serial.print("Read "); 
  Serial.println(addr-cartMemOffset1); 
  tft.drawString("R", 0, 0);
  delay(0);  
  
  if (addr-cartMemOffset1 >0 && addr-cartMemOffset1<CART_MEM) return EEPROM.read(addr-cartMemOffset1);
  else {
    Serial.print(F("Error! Out of cart memory read ")); 
    Serial.println(addr-cartMemOffset1);
    tft.drawString("ER!", 0, 0);}
}


void gb_cart_ram_write(struct gb_s *gb, const uint32_t addr, const uint8_t val){
  if (cartMemFlag == 0) {
     cartMemOffset1 = addr - 100; 
     EEPROM.write(0, cartMemOffset1>>8);
     EEPROM.write(1, cartMemOffset1 & 255);
     EEPROM.write(CART_MEM-3, cartMemOffset1>>8);
     EEPROM.write(CART_MEM-2, cartMemOffset1&255);
     cartMemFlag = 1;}

  Serial.print("Write "); 
  Serial.println(addr-cartMemOffset1); 
  tft.drawString("W", 0, 0);
  delay(0);
  
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
void lcd_draw_line(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line){
  static uint8_t x;
  static uint16_t uiBuff[128];
  static uint_fast8_t colNo, colTp;
  const static uint16_t palette[3][4] ={
      { 0x7FFF, 0x329F, 0x001F, 0x0000 }, /. OBJ0
      { 0x7FFF, 0x3FE6, 0x0200, 0x0000 }, // OBJ1
      { 0x7FFF, 0x7EAC, 0x40C0, 0x0000 }  // BG
    };
  
  if(line > OFFSET_Y && line < 128 + OFFSET_Y){
    for (x = 0; x < 128; ++x){
      colNo = pixels[x+OFFSET_X];
      colTp = (colNo>>3)&3;
      colNo &= 3;
      uiBuff[x] = palette[colTp][colNo];
    }
    tft.pushImage(0, line-OFFSET_Y, 128, 1, uiBuff);
  }
}
*/


void lcd_draw_line(struct gb_s *gb, const uint8_t *pixels, const uint_fast8_t line){
  static uint8_t x;
  static uint8_t uiBuff[128];
 // static const uint8_t palette8[] = {0x00, 0x52, 0xA5, 0xFF};
  static const uint8_t palette8[] = {0xFF, 0xA5, 0x52, 0x00};
  
  if(line >= OFFSET_Y && line < 128 + OFFSET_Y){
    for (x = 0; x < 128; x++)
      uiBuff[x] = palette8[pixels[x+OFFSET_X]&3];
    tft.pushImage(0, line-OFFSET_Y, 128, 1, uiBuff, true);
  }
}



void setup() {
  system_update_cpu_freq(SYS_CPU_160MHZ);
    
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println(ESP.getFreeHeap());

//EEPROM init (for game cart RAM)  
  EEPROM.begin(CART_MEM);
/*
  EEPROM.write(100,1);
  EEPROM.write(101,2);
  EEPROM.write(102,3);
  if(EEPROM.read(100) + EEPROM.read(101) + EEPROM.read(102) == 6)
    Serial.println(F("\n\nEEPROM init OK"));
  else Serial.println(F("\n\nEEPROM init FAIL"));
*/
  cartMemOffset1 = (EEPROM.read(0)<<8) + EEPROM.read(1);
  cartMemOffset2 = (EEPROM.read(CART_MEM-3)<<8) + EEPROM.read(CART_MEM-2);
  if (cartMemOffset1 == cartMemOffset2 && cartMemOffset1 != 0) cartMemFlag = 1;
  else cartMemFlag = 0;

  Serial.print("Offset1: ");  Serial.println(cartMemOffset1);
  Serial.print("Offset2: ");  Serial.println(cartMemOffset2);
  
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
  tone(SOUNDPIN, 200, 100);
  delay(100);
  tone(SOUNDPIN, 100, 100);
  delay(100);
  noTone(SOUNDPIN);

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

// init game boy evulator
   ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, NULL);

	if(ret != GB_INIT_NO_ERROR){
		Serial.print("Error: ");
		Serial.println(ret);}

	  gb_init_lcd(&gb, &lcd_draw_line);
    gb.direct.interlace = 0;
    gb.direct.frame_skip = 1;

  Serial.println(ESP.getFreeHeap());

//check OTA and if No than WiFi OFF
  delay(500);
  if (getKeys()&PAD_ACT || getKeys()&PAD_ESC) OTAobj = new ESPboyOTA(&tft, &mcp);
  WiFi.mode(WIFI_OFF);

}


void loop() {
 //static String fps;
 //uint32_t tme = millis();
 
   gb_run_frame(&gb);
   readkeys();
 
 //fps = 1000/(millis() - tme);
 //tft.drawString(fps + " ", 0, 120);
 
  if (cartSaveFlag == 1 && millis() - timeEEPROMcommete > 5000){
    Serial.println("Commiting!");
    cartSaveFlag = 0;
    EEPROM.commit();
  }
}
