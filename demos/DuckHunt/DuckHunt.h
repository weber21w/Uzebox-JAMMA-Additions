//#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <uzebox.h>

#include "data/patches.inc"
#include "data/music-compressed.inc"
#include "data/title.inc"
#include "data/tiles.inc"
#include "data/sprites1.inc"
#include "data/sprites2.inc"
#include "data/sprite-partitioning.inc"
#include "data/compressed-font.inc"
const int8_t charlist[] PROGMEM = " !#,-.&=(){}`~$^0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

extern uint8_t ram_tiles[];
extern uint8_t free_tile_index;

#define DUCK_COLOR_BLACK_GREEN	0
#define DUCK_COLOR_BLUE_MAGENTA	1
#define DUCK_COLOR_RED_BLACK	2

#define DUCK_MIN_READY	15
#define DUCK_HIT_FRAMES		24//frames in the initial shot state
#define DUCK_SPIN_FRAMES	3//original was 5, but there were only 2 unique: 4*3 vs 2*5

#define SFX_SHOOT	4
#define SFX_LAUGH	5	
#define SFX_PAUSE	6
#define SFX_FALL	7
#define SFX_GROUND	8
#define SFX_SCORE	9
#define SFX_BARK	10
#define SFX_QUACK	11

uint8_t timer8;
uint8_t shotsRemaining;
uint32_t score;//this is 1/10th the displayed value...(extra 0)
uint32_t hiScore;
uint16_t scoreBits;//1 bit per duck(0=white, 1=red)
uint8_t targetsRemaining;//for entire round
uint8_t gameType;
uint8_t gameRound;

#define MAX_DUCKS			2

#define STATE_UNAVAILABLE	0
#define STATE_READY			1
#define STATE_ACTIVE		2
#define STATE_HIT			3
#define STATE_FALLING		4
#define STATE_KILLED		5
#define STATE_ESCAPED		6

#define MAX_ANGLES	32
const uint8_t sine_table[MAX_ANGLES] PROGMEM = { 0xff, 0xfd, 0xf5, 0xea, 0xda, 0xc6, 0xb0, 0x98,
0x80, 0x67, 0x4f, 0x39, 0x25, 0x15, 0xa, 0x2, 0x0, 0x2, 0xa, 0x15, 0x25, 0x39, 0x4f, 0x67, 0x80, 0x98, 0xb0, 0xc6, 0xda, 0xea, 0xf5, 0xfd };

uint8_t fast_sine(uint8_t a){
		a %= MAX_ANGLES;
		return pgm_read_byte(&sine_table[a]);
}

uint8_t fast_cosine(uint8_t a){
	a += (MAX_ANGLES/4);
	a %= MAX_ANGLES;
	return pgm_read_byte(&sine_table[a]);
}



uint8_t duck_type[MAX_DUCKS];
uint8_t duck_state[MAX_DUCKS];
uint16_t duck_x[MAX_DUCKS];
uint16_t duck_y[MAX_DUCKS];
uint8_t duck_angle[MAX_DUCKS];
uint8_t duck_vel[MAX_DUCKS];

uint8_t duck_frame[MAX_DUCKS];
uint8_t duck_timer[MAX_DUCKS];
uint8_t escape_timer;//time until all ducks will disappear upon hitting a border(except bottom)

#define DUCK_START_Y		140

#define GAME_CHAR_ZERO		0x8A

#define DUCK_SCORE_X		11
#define DUCK_SCORE_Y		23
#define DUCK_SCORE_WHITE	0x76
#define DUCK_SCORE_RED		0x94
#define DUCK_SCORE_BLANK	DUCK_SCORE_RED+1

#define SCORE_NUMS_X	23
#define SCORE_NUMS_Y	DUCK_SCORE_Y+1

#define DUCK_MIN_DIR_TIME	15
#define DUCK_MAX_DIR_CHANGE	MAX_ANGLES/2

#define AMMO_X		2
#define AMMO_Y		23
#define AMMO_TILE	0x71
#define AMMO_BLACK	DUCK_SCORE_BLANK

const uint8_t dog_intro_pattern[] PROGMEM = {
7,0, 7,1, 7,2, 7,3,//4*4 steps...
7,0, 7,1, 7,2, 7,3,
7,0, 7,1, 7,2, 7,3,
7,0, 7,1, 7,2, 7,3,

14,0, 9,4, 9,0, 9,4, 9,0, 10,4,//sniff event

7,0, 7,1, 7,2, 7,3,//4*4 steps
7,0, 7,1, 7,2, 7,3,
7,0, 7,1, 7,2, 7,3,
7,0, 7,1, 7,2, 7,3,

14,0, 9,4, 9,0, 9,4, 9,0, 10,4,//sniff event

7,0, 1,1, 18,5, 17,6, 32,7,//step, alarm jump, glide...
255,
};

#define DOG_GROUND_Y 120

#define INTRO_BULLET_HOLES	61
const uint8_t intro_bullet_map[] PROGMEM = {
0b10001000,0b11111000,0b11111000,0b11110000,0b01110000,0b10001000,
0b10001000,0b00010000,0b10000000,0b10001000,0b10001000,0b01010000,
0b10001000,0b00100000,0b11110000,0b11110000,0b10001000,0b00100000,
0b10001000,0b01000000,0b10000000,0b10001000,0b10001000,0b01010000,
0b01110000,0b11111000,0b11111000,0b11110000,0b01110000,0b10001000,
};

const uint16_t intro_bullet_offsets[] PROGMEM = {
(4*VRAM_TILES_H)+2,(14*VRAM_TILES_H)+4,(8*VRAM_TILES_H)+11,(18*VRAM_TILES_H)+15,(3*VRAM_TILES_H)+19,(11*VRAM_TILES_H)+26,
};

#define TITLE_BLACK_TILE	8

#define BTN_GUN_TRIGGER	(1U<<12U)
#define BTN_GUN_SENSE	(1U<<13U)

#define FIELD_TOP 5
#define BUTTONS_COUNT 1
#define BUTTON_UNPRESSED 0
#define BUTTON_PRESSED 1

uint8_t CalibrateGun();

extern uint16_t joypad1_status_lo,joypad2_status_lo;
extern uint16_t joypad1_status_hi,joypad2_status_hi;
uint16_t last_joypad1_status_lo;
uint8_t globalFrame = 0;

uint16_t cursorX,cursorY;//9:7 fixed point
uint8_t curosrSensitivity;//scaling factor to apply to mouse deltas
extern bool snesMouseEnabled;
extern uint8_t user_ram_tiles_c;

#define MAX_GUN_LAG	10
#define GUN_COOLDOWN	MAX_GUN_LAG

uint8_t gunTargets;
uint8_t gunLag = MAX_GUN_LAG;
uint8_t gunSenseBuf[MAX_GUN_LAG+1];
uint8_t gunTimer;
uint8_t gunCooldown;

void TitleScreen();
void DogIntro();
void InitDucks();
uint8_t UpdateDucks(uint8_t rt_start);
void DogRetrieve();
uint8_t TallyScore();
void RedrawAmmo();
void RedrawScore();
void RedrawHUD();
void RoundClear();
void GameOver();

const int8_t introcharmap[] PROGMEM = "NOAIMLSWERHDUGTVPF";
const int8_t titlecharmap1[] PROGMEM = "12DUCKS(){}`~$^";
const int8_t titlecharmap2[] PROGMEM = "TOP=0123456789";

void rtl_RamifyFontEx(uint16_t rtoff, const int8_t *cmap, const int8_t *chrlst, const uint8_t *ftiles, uint8_t backcolor, uint8_t fontcolor);
void rtl_PrintRamEx(uint8_t x, uint8_t y,char *string, const int8_t *cmap, uint8_t offset, uint8_t flags);
void rtl_Print(uint8_t x, uint8_t y, const char *string);

static void _emu_whisper(int port, unsigned char val){
	if(port == 0){ u8 volatile * const _whisper_pointer1 = (u8 *) 0x39; *_whisper_pointer1 = val; }
	if(port == 1){ u8 volatile * const _whisper_pointer2 = (u8 *) 0x3A; *_whisper_pointer2 = val; }
}