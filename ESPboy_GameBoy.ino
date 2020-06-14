/*
# ESPboy_GameBoy
GameBoy emulator for ESPboy. 

You are able to try Nintendo retro games like SuperMario, Zelda, Pokemon, etc! 
Pressing both side buttons, you can adjust view port (original GB screen resolution is 160х144 but it's only 128x128 window visible on ESPboy display).

by RomanS for ESPboy project
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun

Peanut-GB core is used
https://github.com/deltabeard/Peanut-GB

MIT license
*/

#include "Arduino.h"
#include <Adafruit_MCP23017.h>
#include <Adafruit_MCP4725.h>
#include <TFT_eSPI.h>
#include <ESP8266WiFi.h>
#include "ESPboyLogo.h"
#include "rom.c"

#include "peanut_gb.h"

//#define GB_ROM rom1   //test rom
#define GB_ROM rom2   //super mario land
//#define GB_ROM rom3   //tetris
//#define GB_ROM rom4   //lemmings
//#define GB_ROM rom5   //kirby's dream land
//#define GB_ROM rom6   //mega man
//#define GB_ROM rom7   //zelda

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

uint8_t OFFSET_X=16, OFFSET_Y=8;

static struct gb_s gb;
enum gb_init_error_e ret;


uint8_t inline getKeys() { return (~mcp.readGPIOAB() & 255); }

void readkeys(){
 static uint8_t nowkeys;
  nowkeys = getKeys();
  if (nowkeys&PAD_LFT && nowkeys&PAD_RGT){
    adjustOffset();
  }
  else{
    gb.direct.joypad_bits.a = (nowkeys&PAD_ACT)?0:1;
    gb.direct.joypad_bits.b = (nowkeys&PAD_ESC)?0:1;
    gb.direct.joypad_bits.up = (nowkeys&PAD_UP)?0:1;
    gb.direct.joypad_bits.down = (nowkeys&PAD_DOWN)?0:1;
    gb.direct.joypad_bits.left = (nowkeys&PAD_LEFT)?0:1;
    gb.direct.joypad_bits.right = (nowkeys&PAD_RIGHT)?0:1;
    gb.direct.joypad_bits.start = (nowkeys&PAD_LFT)?0:1;
    gb.direct.joypad_bits.select = (nowkeys&PAD_RGT)?0:1;
  }
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

uint8_t gb_cart_ram_read(struct gb_s *gb, const uint32_t addr){}

void gb_cart_ram_write(struct gb_s *gb, const uint32_t addr, const uint8_t val){}

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
  
  if(line > OFFSET_Y && line < 128 + OFFSET_Y){
    for (x = 0; x < 128; ++x)
      uiBuff[x] = palette8[pixels[x+OFFSET_X]&3];
    tft.pushImage(0, line-OFFSET_Y, 128, 1, uiBuff, true);
  }
}


void setup() {
  system_update_cpu_freq(SYS_CPU_160MHZ);
  WiFi.mode(WIFI_OFF);
    
  Serial.begin(115200);
  delay(100);

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
    delay(10);
  }
  dac.setVoltage(4095, true);
  delay(1000);

// clear screen
  tft.fillScreen(TFT_BLACK);
  
  ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, NULL);

	if(ret != GB_INIT_NO_ERROR){
		Serial.print("Error: ");
		Serial.println(ret);
	}

	gb_init_lcd(&gb, &lcd_draw_line);
    gb.direct.interlace = 0;
    gb.direct.frame_skip = 1;
}


void loop() {
 //static uint32_t tme;
 //static String fps;
 //tme = millis();
 
   gb_run_frame(&gb);
   readkeys();
 
 //fps = 1000/(millis() - tme);
 //tft.drawString(fps + " ", 0, 120);
}
