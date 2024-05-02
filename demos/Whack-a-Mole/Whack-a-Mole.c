/*
 *  Uzebox(tm) Whack-a-Mole
 *  Copyright (C) 2009  Alec Bourque
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Uzebox is a reserved trade mark
*/
#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <uzebox.h>
#include <gui.h>

#include "data/whacksong.inc"

#include "data/patches.inc"
#include "data/tiles.inc"
#include "data/sprites.inc"

#define BTN_GUN_TRIGGER	(1U<<12U)
#define BTN_GUN_SENSE	(1U<<13U)

#define FIELD_TOP 5
#define BUTTONS_COUNT 1
#define BUTTON_UNPRESSED 0
#define BUTTON_PRESSED 1

uint8_t calibrateGun();
uint8_t lightgunScan();
char createMyButton(unsigned char x,unsigned char y,const char *normalMapPtr,const char *pushedMapPtr);
void processMouseMovement(void);
void processHammer(void);
void doStart();
void processMyButtons();
void processMoles();
void WaitKey();
//void DetectControllers();
void menu();
void PrintSpeed();

//unsigned char vram_buffer[VRAM_SIZE];
void PushVRAM(){
//	for(uint16_t i=0;i<VRAM_SIZE;i++)
//		vram_buffer[i] = vram[i];
}

void PullVRAM(){
//	for(uint16_t i=0;i<VRAM_SIZE;i++)
//		vram[i] = vram_buffer[i];
}

const char strNotDetected[] PROGMEM ="MOUSE NOT DETECTED!";
const char strHighScore[] PROGMEM="HI-SCORE:";
const char strWhacked[] PROGMEM="MOLES WHACKED:";
const char strTime[] PROGMEM="TIME:";

const char strCopy[] PROGMEM="\\2009 UZE";
const char strStart[] PROGMEM="PRESS A BUTTON";
const char strWeb[] PROGMEM="HTTP://BELOGIC.COM/UZEBOX";

const char strSpeed[] PROGMEM="MOUSE SPD:";
const char strLo[] PROGMEM= "LOW ";
const char strMed[] PROGMEM="MED ";
const char strHi[] PROGMEM= "HIGH";

const char strError[] PROGMEM= "ERROR:";

#define MAX_GUN_LAG	10
uint8_t gunLag = 5;
uint8_t gunSenseBuf[MAX_GUN_LAG];

struct ButtonStruct
{
	unsigned char id;
	unsigned char x;
	unsigned char y;
	unsigned char width;
	unsigned char height;
	const char *normalMapPtr;
	const char *pushedMapPtr;
	unsigned char state;
	bool clicked;
};
struct ButtonStruct buttons[BUTTONS_COUNT];
unsigned char nextFreeButtonIndex=0;


struct MoleStruct
{
	bool active;
	unsigned char animFrame;
	unsigned char x;
	unsigned char y;
	bool whacked;
};
struct MoleStruct moles[4][4];


struct EepromBlockSaveGameStruct{
	//some unique block ID
	unsigned int id;
		
	unsigned char top1name[6];
	unsigned int top1Score;
	unsigned char top2name[6];	
	unsigned int top2Score;
	unsigned char top3name[6];
	unsigned int top3Score;

	unsigned char data[6];		
};
struct EepromBlockSaveGameStruct saveGameBlock;

extern unsigned char ram_tiles[];
extern unsigned char playDevice,playPort;
extern unsigned int actionButton;
int mx,my;
char dx=0,dy=0;
unsigned char highScore=0,molesWhacked=0,activeMoles=0,level,mouseSpeed=MOUSE_SENSITIVITY_MEDIUM;
unsigned int time=0,lastTime=0;
bool gameOver;

int main(){
	unsigned char x,y,tmp;
    SetTileTable(tile_data);
    ClearVram();

	SetSpritesTileTable(sprite_data);
	EnableSnesMouse(0,map_cursor);
	DetectControllers();
	InitMusicPlayer(patches);
	//SetMouseSensitivity(mouseSpeed);

	
	menu();


	srand(TCNT1);

  //  SetTileTable(tile_data);
	SetFontTilesIndex(0);

    ClearVram();

    Print(2,1,strHighScore);
	PrintByte(13,1,highScore,true);

	PrintSpeed();


	mx=120;
	my=100;

	Print(12-5,3,strWhacked);
	PrintByte(28-5,3,molesWhacked,true);

	Print(2+10,4,strTime);
	PrintByte(9+10,4,(time/60),true);

	ClearVram();
	DrawMap2(2,FIELD_TOP,map_main);

	//create holes
	for(y=0;y<4;y++){
		for(x=0;x<4;x++){
			DrawMap2((x*6)+4,(y*4)+FIELD_TOP+3,map_hole);
			moles[y][x].whacked=false;
			moles[y][x].animFrame=0;
			moles[y][x].active=false;
			moles[y][x].x=(x*6)+4;
			moles[y][x].y=(y*4)+FIELD_TOP+3;
		}
	
	}

	while(1){

		MapSprite(0,map_cursor);


		activeMoles=0;
		level=2;	
		molesWhacked=0;
		time=60*60;
		gameOver=false;


		doStart();

		

		Print(12-5,3,strWhacked);
		PrintByte(28-5,3,molesWhacked,true);

		Print(2+10,4,strTime);
		PrintByte(9+10,4,(time/60),true);


		do{
	   		WaitVsync(1);
			processMouseMovement();
			processHammer();
			processMoles();
	
			time--;
			tmp=(time/60);
			PrintByte(28-5,3,molesWhacked,true);
			PrintByte(9+10,4,tmp,true);
			level=((60-tmp)/6)+3;
			
			if(tmp<=5 && tmp!=lastTime && tmp!=0){
				TriggerFx(8,0xa0,true);	
			}

			lastTime=tmp;
		}while(tmp>0);

		gameOver=true;
		
		TriggerFx(9,0xa0,true);	
	

		while(activeMoles>0){
			WaitVsync(1);
			processMouseMovement();
			processMoles();
		
		}

		//DrawMap2(11,FIELD_TOP+7+4,map_gameOver);
		MapSprite(9,map_gameOver);
		MoveSprite(9,(11)*8,(FIELD_TOP+7+4)*8,9,2);
		

		if(molesWhacked>highScore){
			highScore=molesWhacked;
			PrintByte(13,1,highScore,true);
		}
	}

} 

void PrintSpeed(){
	Print(15,1,strSpeed);
	if(mouseSpeed==MOUSE_SENSITIVITY_LOW){
		Print(25,1,strLo);
	}else if(mouseSpeed==MOUSE_SENSITIVITY_HIGH){
		Print(25,1,strHi);
	}else{
		Print(25,1,strMed);
	}
}

void menu(){

	unsigned int joy;
  //  SetTileTable(title_tileset);
//	SetFontTilesIndex(TITLE_TILESET_SIZE);
    ClearVram();

	DrawMap2(4,8,map_title);
	
	Print(8,12,strStart);
	Print(10,23,strCopy);
	Print(3,25,strWeb);
actionButton=BTN_B;
WaitVsync(60);calibrateGun();return;
	DetectControllers();
	if(playDevice==0)Print(6,17,strNotDetected);

	StartSong(song_Whacksong);

	while(1){
		joy=ReadJoypad(playPort);
		if(joy&actionButton){
			while(ReadJoypad(playPort)&actionButton);
			StopSong();
			return;
		}
	}
}

/*
void DetectControllers(){
	unsigned int joy;
	//wait a frame for mouse to settle
	WaitVsync(4);
	joy=ReadJoypad(0);
	if((joy&0x8000)!=0){
		//we have a mouse in player 1 port
		playDevice=1;
		playPort=0;
		actionButton=BTN_MOUSE_LEFT;
		return;
	}else{
		playDevice=0;
		playPort=0;
		actionButton=BTN_A;
	}

	joy=ReadJoypad(1);
	if((joy&0x8000)!=0){
		//we have a mouse in player 2 port
		playDevice=1;
		playPort=1;
		actionButton=BTN_MOUSE_LEFT;
	}


}*/


void SaveVRAM(){
	for(uint16_t i=VRAM_SIZE;i<VRAM_SIZE+(MAX_SPRITES);i++){
		ram_tiles[i] = sprites[i-VRAM_SIZE].flags;
		sprites[i-VRAM_SIZE].flags = SPRITE_OFF;
	}

	for(uint16_t i=0;i<VRAM_SIZE;i++)
		ram_tiles[i] = vram[i];
}


void RestoreVRAM(){
	for(uint16_t i=0;i<VRAM_SIZE;i++)
		vram[i] = ram_tiles[i];

	for(uint16_t i=VRAM_SIZE;i<VRAM_SIZE+MAX_SPRITES;i++)
		sprites[i-VRAM_SIZE].flags = ram_tiles[i];
}


uint8_t calibrateGun(){//returns gun latency frames
	SaveVRAM();

GUN_CALIBRATE_TOP:
	ClearVram();//DDRC=0;
	while(ReadJoypad(0) & BTN_GUN_TRIGGER)//wait for trigger to be released
		WaitVsync(1);

	Print((SCREEN_TILES_H/2)-12,SCREEN_TILES_V-2,PSTR("SHOOT CENTER TO CALIBRATE"));
	while(ReadJoypad(0) & BTN_GUN_SENSE)//wait until we don't sense light on the black screen
		WaitVsync(1);

	while(1){
		WaitVsync(1);
		if(ReadJoypad(0) & BTN_GUN_SENSE)//detected light when we shouldn't have?
			goto GUN_CALIBRATE_TOP;
		if(ReadJoypad(0) & BTN_GUN_TRIGGER)
			break;
	}

	for(uint16_t j=VRAM_SIZE;j<VRAM_SIZE+64;j++){ram_tiles[j] = 0xFF;}//create a white ram_tile if necessary
	for(uint16_t j=0;j<VRAM_SIZE;j++){vram[j] = VRAM_SIZE/64;}//SetTile(j%VRAM_TILES_H,j/VRAM_TILES_H,WHITE_TILE);

	uint8_t k;
	for(k=0;k<MAX_GUN_LAG;k++){//display the white frame until it's detected or timed out
		WaitVsync(1);
		if(ReadJoypad(0) & BTN_GUN_SENSE){
			goto GUN_CALIBRATE_END;		}
	}
	goto GUN_CALIBRATE_TOP;

GUN_CALIBRATE_END:
	RestoreVRAM();
	return k;
}


uint16_t drawTargetMask(uint16_t mask){//draws (active)moles as targets, according to the bitmask and returns the Joypad state
	ClearVram();
	for(uint8_t y=0;y<4;y++){
		for(uint8_t x=0;x<4;x++){
			if(mask & 0b1000000000000000){
				if(moles[y][x].active)
					DrawMap((x*6)+4,(y*4)+FIELD_TOP+3,map_target);
			}
			mask <<= 1;
		}
	}
	WaitVsync(1);
	return ReadJoypad(0);
}


uint8_t lightgunScan(){//performs screen black/white checks to return ((hitColumn<<4)|(hitRow&0x0F)) or 255 for no hit
	uint8_t gunSensePos = 0;
	gunSenseBuf[gunSensePos++] = drawTargetMask(0);//black screen, we shouldn't detect light this frame...

	//filter column(0/1 if sensed, otherwise 2/3)
	gunSenseBuf[gunSensePos++] = drawTargetMask(0b\
1100\
1100\
1100\
1100\
);
	//determine column
	gunSenseBuf[gunSensePos++] = drawTargetMask(0b\
0110\
0110\
0110\
0110\
);


	//verify hit(verifies timing, covers row 3, column 3 case not covered elsewhere)
	gunSenseBuf[gunSensePos++] = drawTargetMask(0b\
1111\
1111\
1111\
1111\
);

	//filter row(0/1 if sensed, otherwise 2/3)
	gunSenseBuf[gunSensePos++] = drawTargetMask(0b\
1111\
1111\
0000\
0000\
);
	//determine row
	gunSenseBuf[gunSensePos++] = drawTargetMask(0b\
0000\
1111\
1111\
0000\
);
	for(uint8_t i=0;i<gunLag;i++){//make sure gun has had time to see entire sequence
		WaitVsync(1);
		gunSenseBuf[gunSensePos++] = ReadJoypad(0);
	}
	for(uint8_t i=0;i<gunLag;i++)//compensate the guns signal pattern for measured lag frames
		gunSenseBuf[i] = gunSenseBuf[i+gunLag];

	uint8_t hitColumn = 255;
	uint8_t hitRow = 255;

	if(!gunSenseBuf[0] && gunSenseBuf[3]){//saw nothing on blank frame and something on all light frame?
		if(gunSenseBuf[1])//saw something on first column filter? then column is 0 or 1
			hitColumn = (gunSenseBuf[2]?1:0);
		else//otherwise column is 2 or 3
			hitColumn = (gunSenseBuf[2]?2:3);

		if(gunSenseBuf[4])//saw something on first row filter? then row is 0 or 1
			hitRow = (gunSenseBuf[5]?1:0);
		else//otherwise row is 2 or 3
			hitRow = (gunSenseBuf[5]?2:3);
	}//otherwise they are pointed at a lightbulb :)

	return (uint8_t)((hitColumn<<4)|(hitRow&0x0F));
}

unsigned char cnt=0;
void processMoles(){
	unsigned char x,y,rdx,rdy;
	const char *ptr=NULL;

	//randomize moles
	if(activeMoles<level && !gameOver){

		rdx=random()%256;
		if(rdx<(16+(level*2))){

			rdx=rand()%4;		
			rdy=rand()%4;
			cnt++;
			if(moles[rdy][rdx].active==false){

				moles[rdy][rdx].active=true;
				moles[rdy][rdx].whacked=false;
				moles[rdy][rdx].animFrame=0;
				activeMoles++;

			}
		}

	}

	//animate moles
	for(y=0;y<4;y++){
		for(x=0;x<4;x++){
			if(moles[y][x].active){
				ptr=NULL;

				if(moles[y][x].whacked){

					switch(moles[y][x].animFrame){
						case 0:
						case 8:
						case 16:
							ptr=map_mole6;
							break;
				
						case 4:
						case 12:
						case 20:
							ptr=map_mole7;
							break;				
																										
						case 24:
							ptr=map_hole;
							break;

						case 28:
							moles[y][x].active=false;
							activeMoles--;
							break;
					}

				}else{
					switch(moles[y][x].animFrame){
						case 0:
						case 52+10:
							ptr=map_mole1;
							break;
				
						case 6:
						case 46+10:
							ptr=map_mole2;
							break;				
				
				
						case 12:
						case 40+10:
							ptr=map_mole3;
							break;
						
						case 18:
						case 36+10:
							ptr=map_mole4;
							break;
						
						case 24:
							ptr=map_mole5;
							break;
																						
						case 58+10:
							ptr=map_hole;
							break;

						case 64+10:
							moles[y][x].active=false;
							activeMoles--;
							break;
					}
				}

				if(ptr!=NULL){				
					DrawMap2((x*6)+4,(y*4)+FIELD_TOP+3,ptr);
				}

				moles[y][x].animFrame++;
				
			}

	
			
		}		
	}

	
}

void WaitKey(){
	while((ReadJoypad(playPort)&BTN_MOUSE_LEFT|BTN_GUN_TRIGGER)==0);
	while((ReadJoypad(playPort)&BTN_MOUSE_LEFT|BTN_GUN_TRIGGER)!=0);
}


void doStart(){
	unsigned char btnId;
	unsigned int frame=0;


	nextFreeButtonIndex=0;
	btnId=createMyButton(13,FIELD_TOP+3,map_startBtnUp,map_startBtnDown);

	while(1){
   		WaitVsync(1);
		processMouseMovement();
		processMyButtons();

		//check if we have pressed the start button
		if(buttons[btnId].clicked){
			DrawMap2(10,FIELD_TOP+3,map_hole);
			DrawMap2(16,FIELD_TOP+3,map_hole);
			MapSprite(0,map_hammerUp);

			DrawMap2(12,FIELD_TOP+7+4,map_ready);



			while(frame<120){
				WaitVsync(1);
				processMouseMovement();
				processHammer();

				if(frame==40 || frame==110){
					DrawMap2(10,FIELD_TOP+3+4+4,map_hole);
					DrawMap2(16,FIELD_TOP+3+4+4,map_hole);
				}else if(frame==50){
					DrawMap2(12,FIELD_TOP+7+4,map_whack);
				}

				frame++;
			}
			return;
		}


	}
}

void processMyButtons(){
	unsigned char i,tx,ty;
	unsigned int joy=ReadJoypad(playPort);
	static unsigned int lastButtons;
lightgunScan();
	tx=(mx+6)>>3;
	ty=my>>3;
actionButton |= BTN_GUN_TRIGGER;

	for(i=0;i<nextFreeButtonIndex;i++){
		if(tx>=buttons[i].x && tx<(buttons[i].x+buttons[i].width) && ty>=buttons[i].y && ty<(buttons[i].y+buttons[i].height)){
			if(joy&actionButton && buttons[i].state==BUTTON_UNPRESSED){
				DrawMap2(buttons[i].x,buttons[i].y,buttons[i].pushedMapPtr);
				buttons[i].state=BUTTON_PRESSED;
			}
			
			if((joy&actionButton)==0 && buttons[i].state==BUTTON_PRESSED){
				//button clicked!
				buttons[i].state=BUTTON_UNPRESSED;
				buttons[i].clicked=true;
				DrawMap2(buttons[i].x,buttons[i].y,buttons[i].normalMapPtr);
			}			
		}else{
		
			if((joy&actionButton)==0 && buttons[i].state==BUTTON_PRESSED){
				//button clicked!
				buttons[i].state=BUTTON_UNPRESSED;
				buttons[i].clicked=false;
				DrawMap2(buttons[i].x,buttons[i].y,buttons[i].normalMapPtr);
			}

		}


	}

	if((joy&BTN_MOUSE_RIGHT) && ((lastButtons&BTN_MOUSE_RIGHT)==0)){
		TriggerFx(8,0xff,true);
		mouseSpeed++;
		if(mouseSpeed==3)mouseSpeed=0;
		//SetMouseSensitivity(mouseSpeed);
		PrintSpeed();
	}

	lastButtons=joy;
}

char createMyButton(unsigned char x,unsigned char y,const char *normalMapPtr,const char *pushedMapPtr){
	unsigned char id;

	if(nextFreeButtonIndex<BUTTONS_COUNT){
		buttons[nextFreeButtonIndex].x=x;
		buttons[nextFreeButtonIndex].y=y;
		buttons[nextFreeButtonIndex].normalMapPtr=normalMapPtr;
		buttons[nextFreeButtonIndex].pushedMapPtr=pushedMapPtr;
		buttons[nextFreeButtonIndex].state=0;
		buttons[nextFreeButtonIndex].width=pgm_read_byte(&(normalMapPtr[0]));
		buttons[nextFreeButtonIndex].height=pgm_read_byte(&(normalMapPtr[1]));
		buttons[nextFreeButtonIndex].clicked=false;
		DrawMap2(x,y,normalMapPtr);
		id=nextFreeButtonIndex;
		nextFreeButtonIndex++;
	}else{
		id=-1;
	}

	return id;
}



void processMouseMovement(void){
	unsigned int joy;
	


	//check in case its a SNES pad or Lightgun

	if(playDevice==0){
		joy=ReadJoypad(playPort);

		if(joy&BTN_LEFT){
			mx-=2;
			if(mx<5) mx=5; 
		}
		if(joy&BTN_RIGHT){
			mx+=2;
			if(mx>227) mx=227;
		}
		if(joy&BTN_UP){
			my-=2;
			if(my<0)my=0;
		}
		if(joy&BTN_DOWN){
			my+=2;
			if(my>218)my=218;
		}

	}else{
	
		joy=ReadJoypadExt(playPort);

		if(joy&0x80){
			mx-=(joy&0x7f);
			if(mx<5) mx=5; 
		}else{
			mx+=(joy&0x7f);
			if(mx>227) mx=227;
		}
	
		if(joy&0x8000){
			my-=((joy>>8)&0x7f);
			if(my<0)my=0;
		}else{
			my+=((joy>>8)&0x7f);
			if(my>218)my=218;
		}
	
	}

	MoveSprite(0,mx,my,3,3);
}

void processHammer(void){
	static int lastbuttons=0;
	static unsigned char hammerState=0,hammerFrame=0;
	static bool mouseBtnReleased=true;
	unsigned int joy;
	unsigned char x,y,dx,dy;

	
	joy=ReadJoypad(playPort);

	if((joy&actionButton) && hammerState==0 && mouseBtnReleased){

		MapSprite(0,map_hammerDown);
		hammerState=1;
		hammerFrame=0;
		dy=5;
		dx=-5;
		mouseBtnReleased=false;
		srand(TCNT1);
		TriggerFx(6,0xa0,true);

		if(!gameOver){
			//check if a mole was whacked!

			dx=((mx+6)>>3);
			dy=((my+8)>>3);


			for(y=0;y<4;y++){
				for(x=0;x<4;x++){
					if(moles[y][x].active && !moles[y][x].whacked && moles[y][x].animFrame>=12 && moles[y][x].animFrame<=60){
						if(dx>=moles[y][x].x && dx<(moles[y][x].x+5) &&
						   dy>=moles[y][x].y && dy<(moles[y][x].y+4)){
						
							//PrintHexByte(10,4,moles[y][x].active);
							//PrintHexByte(13,4,moles[y][x].animFrame);
						
							moles[y][x].whacked=true;
							moles[y][x].animFrame=0;
							molesWhacked++;
							TriggerFx(7,0xff,true);

						}
					}
				}	
			}
				
		}

	}

	if(hammerState==1 && mouseBtnReleased){
		MapSprite(0,map_hammerUp);
		dy=0;
		dx=0;
		hammerState=0;
	}
	
	if((joy&actionButton)==0)mouseBtnReleased=true;
	


	hammerFrame++;

	lastbuttons=joy;
}

