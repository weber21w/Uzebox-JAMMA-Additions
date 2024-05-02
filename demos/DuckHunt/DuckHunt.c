#include "DuckHunt.h"

void DHWaitVsync(uint8_t f){
	while(f){
		WaitVsync(1);
		f--;
	}
}

void FillVram(uint8_t t){
	for(uint16_t i=0;i<VRAM_SIZE;i++)
		vram[i] = t;
}

uint8_t GunSenseFrame(uint8_t frame){
	if(frame < 8)
		return (gunSenseBuf[2]>>(frame%8))&1;
	else if(frame < 16)
		return (gunSenseBuf[1]>>(frame%8))&1;
	return (gunSenseBuf[0]>>(frame%8))&1;
}

void ResetGunSenseBuffer(){
	gunSenseBuf[0] = gunSenseBuf[1] = gunSenseBuf[2] = 0;
}

void BufferGunSensor(){
	gunSenseBuf[0] <<= 1;
	if(gunSenseBuf[1] & 0b10000000)
		gunSenseBuf[0] |= 1;
	gunSenseBuf[1] <<= 1;
	if(gunSenseBuf[2] & 0b10000000)
		gunSenseBuf[1] |= 1;
	gunSenseBuf[2] <<= 1;
	if(joypad1_status_lo & LG_SENSE){
		gunSenseBuf[2] |= 1;
		//_emu_whisper(0,gunTimer);
	}
}

void PreVsyncRoutine(){
//	last_joypad1_status_lo = joypad1_status_lo;//store last frame state to test for new events
	//ReadButtons();//read first 16 bits(joypad+lightgun), we read the last 16 bits after vsync tasks
	//BufferGunSensor();
	//gunTimer++;
}

void PostVsyncRoutine(){//called by kernel after it has processed vsync items
	
	last_joypad1_status_lo = joypad1_status_lo;//store last frame state to test for new events
	ReadButtons();//read first 16 bits(joypad+lightgun), we read the last 16 bits after vsync tasks
	BufferGunSensor();
	globalFrame++;
	if(gunTimer < MAX_GUN_LAG)
		gunTimer++;
	if(gunCooldown)
		gunCooldown--;

	int xmag,ymag;
	/*if(joypad1_status_lo & MOUSE_SIGNATURE){
		ReadMouseExtendedData();

		xmag = (joypad1_status_hi>>0) & 0x7F;
		ymag = (joypad1_status_hi>>8) & 0x7F;
		if((joypad1_status_hi>>0) & 0x80)//left?
			xmag *= -1;
		if((joypad1_status_hi>>8) & 0x80)//up?
			ymag *= -1;

		if(joypad1_status_lo & BTN_DOWN)
			ymag = 1;
		else if(joypad1_status_lo & BTN_UP)
			ymag = 1;
		if(joypad1_status_lo & BTN_RIGHT)
			xmag = 1;
		else if(joypad1_status_lo & BTN_LEFT)
			xmag = 1;
		if(joypad1_status_lo & BTN_SL){
			xmag *= 2;
			ymag *= 2;
		}
		cursorX += xmag;
		cursorY += ymag;
	}
	if(cursorX < 0)
		cursorX = 0;
	else if((cursorX>>7) > ((SCREEN_TILES_H-2)*8))
		cursorX = ((SCREEN_TILES_H-2)*8)<<7;
	if(cursorY < 0)
		cursorY = 0;
	else if((cursorY>>7) > ((SCREEN_TILES_V-2)*8))
		cursorY = ((SCREEN_TILES_V-2)*8)<<7;
*/
	return;
}

void BootIntro(){
return;
	FadeIn(3,0);
	SetTileTable(title_data);
	FillVram(RAM_TILES_COUNT+TITLE_BLACK_TILE);
	duck_x[0] = 0;
	duck_y[0] = (156<<8UL);

	for(uint8_t i=0;i<8;i++)//prepare variations of bullet hole into ram_tiles[](H and V flipping combinations)
		BlitSprite(SPRITE_BANK1|(i%4), (SPRITE_DATA2_SIZE-2)+(i>3), (i*8), 0);
	FillVram(RAM_TILES_COUNT+TITLE_BLACK_TILE);//replace ram_tile[] indices in vram[]..
	WaitVsync(1);

	for(uint8_t i=0;i<6;i++){
		TriggerFx(SFX_SHOOT,255,1);
		uint16_t voff = pgm_read_word(&intro_bullet_offsets[i]);
		for(uint8_t y=0;y<5;y++){
			uint8_t t = pgm_read_byte(&intro_bullet_map[(y*6)+i]);
			for(uint8_t x=0;x<7;x++){
				if(t & 0b10000000)
					vram[voff+x] = (y+x+i)%8;//point to pre-rendered ram_tiles[]
				t <<= 1;
					//WaitVsync(120);
			}
			voff += VRAM_TILES_H;
		}
		uint8_t wait;
		if(i == 2)
			wait = 30;
		else if(i < 3)
			wait = 15;
		else
			wait = 10;
		while(wait--){
			WaitVsync(1);
			for(uint16_t i=0;i<VRAM_SIZE;i++){
				if(vram[i] > 7)//clean up any sprite ram_tiles indices from last frame(leave bullet holes alone)
					vram[i] = RAM_TILES_COUNT+TITLE_BLACK_TILE;
			}
			duck_state[0] = STATE_ACTIVE;
			duck_timer[0] = 0;
			if((duck_x[0]>>8) < ((SCREEN_TILES_H-5)*TILE_WIDTH)){
				duck_x[0] += 12<<6;
			//	if(duck_x[0] <  ((SCREEN_TILES_H-5)*TILE_WIDTH))
			//		duck_y[0] -= 6<<6;
			//	else
					duck_y[0] -= 3<<6;	
				if(wait & 4)
					duck_frame[0] = !duck_frame[0];
				UpdateDucks(8);
			}
		}
	}
	WaitVsync(40);
	FadeOut(5,1);
	rtl_RamifyFontEx(0,introcharmap,charlist,compressed_font,0x00,0xFF);
	FillVram(RAM_TILES_COUNT+TITLE_BLACK_TILE);//replace ram_tile[] indices in vram[]..
	rtl_Print(5,10,PSTR("NO ANIMALS WERE HARMED"));
	rtl_Print(5,12,PSTR("DURING THE DEVELOPMENT"));
	rtl_Print(5,14,PSTR("OF THIS GAME"));
	FadeIn(3,1);
	WaitVsync(40);
	FadeOut(5,1);
}

void BlackScreen(uint8_t t){
	if(t < RAM_TILES_COUNT){
		for(uint8_t i=0;i<64;i++)
			ram_tiles[(t*64)+i] = 0;
	}
	FillVram(t);
}

void TitleScreen(){
	//StartSong(mus_title);
	DDRC = 0;
	FadeIn(4,0);
	SetTileTable(title_data);
	rtl_RamifyFontEx(0,titlecharmap1,charlist,compressed_font,0xD8,0x00);
	rtl_RamifyFontEx(sizeof(titlecharmap1),titlecharmap2,charlist,compressed_font,0xD8,0xBB);
	rtl_Print(5,23,PSTR("TOP SCORE ="));
 //user_ram_tiles_c = sizeof(titlecharmap1)+sizeof(titlecharmap2);
	gameType = 255;
	gunTargets = 0;
uint8_t wait_frames = 0;
	while(1){
		WaitVsync(1);
		if(!(last_joypad1_status_lo & BTN_START) && (joypad1_status_lo & BTN_START))
			wait_frames++;
		if((last_joypad1_status_lo & LG_TRIGGER) && !(joypad1_status_lo & LG_TRIGGER)){//trigger just pulled(inverted)?
			if(gunLag == MAX_GUN_LAG){//use first trigger pull to determine display latency

				CalibrateGun();
			}
			gunTargets = 1;
			//see if a game selection square was shot
		//	if((joypad1_status_lo & LG_SENSE) || (last_joypad1_status_lo & LG_SENSE))
		//		continue;
			//ResetGunSenseBuffer();
			BlackScreen(RAM_TILES_COUNT+8);//title black tile
			WaitVsync(1);

			for(uint8_t i=0;i<2;i++){//draw 1 white box per frame, and see if it's detected
				for(uint8_t y=16;y<21;y++){
					for(uint8_t x=4;x<15;x++)
						SetTile(x+(i*14), y, 11);//title white tile
				}
				WaitVsync(1);
				if(i == 0){
					FillVram(RAM_TILES_COUNT+8);//refill with title black tile
					gunTimer = 0;//start counting up to gunLag
				}else{
					BlackScreen(RAM_TILES_COUNT+8);
					WaitVsync(1);
				}
			//	_emu_whisper(0,gunTimer);
			}
		}
		if(gunTargets){
			//_emu_whisper(0,gunTimer);
			if(gunTimer > gunLag+1){//both targets should be buffered by now
				if(GunSenseFrame(gunTimer-1) && !GunSenseFrame(gunTimer-0))
					gameType = 0;
				else if(GunSenseFrame(gunTimer-2) && !GunSenseFrame(gunTimer-1))
					gameType = 1;
				gunTargets = 0;
			}

			if(gameType != 255)
				break;
		}
	FillVram(RAM_TILES_COUNT+0);
	DrawMap(0,0,title_map);
rtl_RamifyFontEx(0,titlecharmap1,charlist,compressed_font,0xD8,0x00);
	rtl_Print(4,16,PSTR("($$$$$$$$$)   ($$$$$$$$$)"));
	rtl_Print(4,17,PSTR("`         ~   `         ~"));
	rtl_Print(4,18,PSTR("` 1 DUCK  ~   ` 2 DUCKS ~"));
	rtl_Print(4,19,PSTR("`         ~   `         ~"));
	rtl_Print(4,20,PSTR("{^^^^^^^^^}   {^^^^^^^^^}"));

		//WaitVsync(1);
//if(cursorX != 0xFFFF)
//	BlitSprite(0, 3, cursorX>>7, cursorY>>7);

		uint16_t map_pos = (uint16_t)(2+(7*7*6));
		uint8_t t;
		free_tile_index = 0;
		uint8_t dx = 80;
	//	for(uint8_t j=0; j<5; j++){
	//		for(uint8_t i=0; i<6; i++){
	//			t = pgm_read_byte(&duck_map[map_pos++]);
	//			if(t)
	//				BlitSprite(0, SPRITE_DATA2_SIZE-3, dx+(i*8), 80+(j*8));
	//		}
	//	}
	}

	//return;
	if(gameType == 0)
		StartSong(mus_got_duck);
	else
		StartSong(mus_perfect);

}

void DrawForeground(uint8_t x, uint8_t y, uint8_t w, uint8_t h){
return;
	//this is an alternative method, normally you would want to use ram_tiles_restore[] instead...
	uint16_t voff = (y*VRAM_TILES_H)+x;
	for(uint8_t j=0; j<h; j++){
		for(uint8_t i=0; i<w; i++){
			uint8_t vt = vram[voff++];
			if(vt < RAM_TILES_COUNT){//have a ram_tile[]? check if it was blit over a foreground tile...
				uint8_t mt = pgm_read_byte(&map_main[voff+1]);
				if(mt){//not blue sky?
					uint16_t roff = vt*TILE_WIDTH*TILE_HEIGHT;
					uint16_t toff = mt*TILE_WIDTH*TILE_HEIGHT;
					for(uint8_t k=0; k<TILE_HEIGHT*TILE_WIDTH; k+=8){
						uint8_t p = pgm_read_byte(&tile_data[toff++]);
						if(p != 0xE2)//not blue sky?
							ram_tiles[roff+k+0] = p;
						p = pgm_read_byte(&tile_data[toff++]);
						if(p != 0xE2)//not blue sky?
							ram_tiles[roff+k+1] = p;
						p = pgm_read_byte(&tile_data[toff++]);
						if(p != 0xE2)//not blue sky?
							ram_tiles[roff+k+2] = p;
						p = pgm_read_byte(&tile_data[toff++]);
						if(p != 0xE2)//not blue sky?
							ram_tiles[roff+k+3] = p;
						p = pgm_read_byte(&tile_data[toff++]);
						if(p != 0xE2)//not blue sky?
							ram_tiles[roff+k+4] = p;
						p = pgm_read_byte(&tile_data[toff++]);
						if(p != 0xE2)//not blue sky?
							ram_tiles[roff+k+5] = p;
						p = pgm_read_byte(&tile_data[toff++]);
						if(p != 0xE2)//not blue sky?
							ram_tiles[roff+k+6] = p;
						p = pgm_read_byte(&tile_data[toff++]);
						if(p != 0xE2)//not blue sky?
							ram_tiles[roff+k+7] = p;
					}
				}
			}
		}
		voff += (VRAM_TILES_H-w);
	}
}

void DogIntro(){//full dog walk at beginning of game, or shorter one at beginning of round
	SetTileTable(tile_data);
return;
	if(gameRound == 1)
		StartSong(mus_intro);
	uint8_t dx = 0;
	uint8_t dwait = 0;
	uint8_t dframe;
	uint8_t dpos = 0;
	while(1){
		DrawMap2(0, 0, map_main);//we save RAM by not using ram_tiles_restore[], so we must always redraw
		RedrawHUD();
		if(dwait)
			dwait--;
		else{
			dwait = pgm_read_byte(&dog_intro_pattern[dpos++]);
			if(dwait == 255)
				break;
			dframe = pgm_read_byte(&dog_intro_pattern[dpos++]);
			if(dframe < 4)
				dx++;
		}
		uint16_t map_pos = (uint16_t)(2+(dframe*7*6));
		uint8_t t;
		free_tile_index = 0;
		for(uint8_t j=0; j<6; j++){
			for(uint8_t i=0; i<7; i++){
				t = pgm_read_byte(&dog_map[map_pos++]);
				if(t)
					BlitSprite(0, t, dx+(i*8), DOG_GROUND_Y+(j*8));
			}
		}
		DrawForeground(dx/8, DOG_GROUND_Y/8, 7, 6);
		WaitVsync(1);
	}
}

void DogRetrieve(){
	uint8_t numHits = 0;
	RedrawHUD();
	WaitVsync(68);
	for(uint8_t i=0;i<MAX_DUCKS;i++){
		if(duck_state[i] == STATE_KILLED)
			numHits++;
	}
	uint8_t dx = 255;
	uint8_t dy = 140;
	if(numHits == 0){
		TriggerFx(SFX_LAUGH,255,1);
		dx = (((SCREEN_TILES_H*8)/2)-24);
		//while(dy < 
	}else{
		StartSong(mus_got_duck);
		WaitVsync(120);
	}
}

void LaunchDuck(uint8_t d){
	//based on statistical analysis, this is an approximation that avoids a huge table
	uint8_t t;
	if(GetPrngNumber(0) < 154){//~60 chance to come from the right area(see graph under research dir)
		t = 144-(GetPrngNumber(0)%27);//min 116
		t += (GetPrngNumber(0)%34);//max 176
		if(GetPrngNumber(0) > 127)//~50% to move to right peak(second peak on right side roughly)
			t += 48;
	}else{//otherwise come from left area(~22-95), bell curve with 65(22+43) being peak
		t = 64-(GetPrngNumber(0)%43);//min 22
		t += (GetPrngNumber(0)%33);//max 96
	}
	duck_y[d] = (int16_t)(DUCK_START_Y*256UL);
	duck_x[d] = (int16_t)(t*256UL);
	duck_timer[d] = 16+(GetPrngNumber(0)%33);
	//t = (GetPrngNumber(0)%3);//get duck type
	//duck_state[d] = (t<<6);
	duck_state[d] = STATE_ACTIVE;
	t = (gameRound < 20)?gameRound:t;//flyaway times averages max at round 1(~690 frames), and min at round 20(306 frames)
	escape_timer = (690-(t*20));//~20 frames less time each round
	escape_timer += (GetPrngNumber(0)%60);//this variation seems roughly true through all rounds...
	escape_timer -= (GetPrngNumber(0)%30);

}

uint8_t DuckHitboxSize(){
	if(gameRound < 23)
		return 32;
	if(gameRound < 27)
		return 28-(gameRound-23);
	return 23;
}

void DogPopUp(){
	uint8_t s = 0;//popping up
	
	while(1){

	}
}

void DrawHitbox(uint8_t d){
	SetTileTable(title_data);
	FillVram(RAM_TILES_COUNT+8);//black screen

	uint8_t p;
	if(gameRound < 23)//rough approximation of shrinking hitbox size..
		p = 32;
	else if(gameRound < 27)
		p = 28-(gameRound-23);
	p = 23;

	uint8_t dx = duck_x[d]>>8;
	uint8_t dy = duck_y[d]>>8;
	//TODO GET OFFSET FOR FRAME
	uint8_t ft = p/8;
	p %= 8;
	p = 8-p;

dx=10;dy=10;
	//free_tile_index = 0;
	for(uint8_t y=0;y<ft;y++){
		for(uint8_t x=0;x<ft;x++){
			SetTile((dx/8)+x, (dy/8)+y, 11);//BlitSprite(SPRITE_BANK1, SPRITE_DATA2_SIZE-5, dx+(x*8), dy+(y*8));	
		}
	}
	free_tile_index = 0;//we only draw a single one per frame
	for(uint8_t y=0;y<ft;y++)
		BlitSprite(SPRITE_BANK1, SPRITE_DATA2_SIZE-5, (dx+(ft*8))-p, (dy+(y*8)));
	for(uint8_t x=0;x<ft;x++)
		BlitSprite(SPRITE_BANK1, SPRITE_DATA2_SIZE-5, (dx+(x*8)), (dy+(ft*8))-p);

	BlitSprite(SPRITE_BANK1, SPRITE_DATA2_SIZE-5, (dx+(ft*8))-p, dy+((ft*8))-p);

	WaitVsync(1);
	SetTileTable(tile_data);
	DrawMap2(0, 0, map_main);
}

void InitDucks(){
	for(uint8_t i=0;i<MAX_DUCKS;i++){
		if(i > gameType)
			break;
		duck_state[i] = STATE_READY;
		duck_timer[i] = DUCK_MIN_READY+GetPrngNumber(60);
	}
}

uint8_t UpdateDucks(uint8_t rt_start){
	uint8_t numActive = 0;
	free_tile_index = rt_start;
	for(uint8_t i=0; i<MAX_DUCKS; i++){
		uint8_t d;
		if(timer8 & 1)//alternate drawing across frames in case ram_tiles[] run out...
			d = (MAX_DUCKS-1)-i;
		else
			d = i;

		if(duck_state[d] == STATE_UNAVAILABLE || duck_state[d] == STATE_KILLED)
			continue;

		numActive++;
		if(duck_timer[d])
			duck_timer[d]--;
		uint8_t dframe = 0;
		uint8_t dmirror = 0;

		if(duck_state[d] == STATE_READY){//can change to: STATE_ACTIVE
			dframe = 255;//no draw
			if(!duck_timer[d])//ready to launch?
				LaunchDuck(d);//this will set escape_timer for this iteration(or multiple times, by design)
			continue;//no draw
		}

		if(duck_state[d] == STATE_ACTIVE){//can change to: STATE_HIT or STATE_ESCAPED
			if(!duck_timer[d]){//time to change directions?
				duck_timer[d] = DUCK_MIN_DIR_TIME+GetPrngNumber((255-DUCK_MIN_DIR_TIME)-gameRound);
//	duck_timer[d] = 120;
	duck_vel[d] = 2;
				uint8_t t1 = GetPrngNumber(DUCK_MAX_DIR_CHANGE/2);
				uint8_t t2 = GetPrngNumber(DUCK_MAX_DIR_CHANGE/2);
				duck_angle[d] += t1;
				duck_angle[d] -= t2;
				duck_angle[d] %= 16;
			}else{//update duck movement
				duck_frame[d] = ((duck_timer[d]&0b00111000)>>3)%3;
				int16_t nx = duck_x[d] + (int16_t)(duck_vel[d]*(fast_cosine(duck_angle[d])));
				int16_t ny = duck_y[d] + (int16_t)(duck_vel[d]*(fast_sine(duck_angle[d])));
				uint8_t w = 0;
				if(nx < (int16_t)(4UL*256UL))//hit left edge?
					w = 1;
				else if(nx >= (int16_t)(((SCREEN_TILES_H*TILE_WIDTH)-44UL)*256UL))//right edge?
					w = 3;
				else if(ny < 4UL*256UL)//top edge?
					w = 2;
				else if(ny >= (int16_t)(((20UL*TILE_HEIGHT)-44UL))*256UL)//bottom edge?
					w = 4;

				if(w && !escape_timer){
					duck_state[d] = STATE_ESCAPED;
				}else if(w){
					duck_angle[d] += (3*(MAX_ANGLES/4));
					duck_angle[d] %= MAX_ANGLES;
				}else{//normal movement
					duck_x[d] = nx;
					duck_y[d] = ny;
				}
				if(duck_angle[d] >= (MAX_ANGLES/2))
					dmirror = 1;
				else
					dmirror = 0;

				if(duck_angle[d] <= (MAX_ANGLES/8)){
					dframe = 3;
				}else if(duck_angle[d] <= 5*(MAX_ANGLES/8)){
					dframe = 0;
				}else if(duck_angle[d] > 7*(MAX_ANGLES/8)){
					dframe = 3;
				}
				dframe += duck_frame[d];
			}
			//periodically blank out duck icon for active ducks
			SetTile((DUCK_SCORE_X+10)-targetsRemaining+d, DUCK_SCORE_Y, ((globalFrame>>4)&1)?DUCK_SCORE_BLANK:DUCK_SCORE_WHITE);
		}

		if(duck_state[d] == STATE_HIT){//can change to: STATE_FALLING
			//this state is set directly by CheckDuckShot()
			duck_frame[d] = 6;
			dframe = 6;
			if(!duck_timer[d])
				duck_state[d] = STATE_FALLING;
		}

		if(duck_state[d] == STATE_FALLING){//can change to STATE_KILLED(landed after fall)
			duck_y[d] += (1<<9);//move down 1 full pixel
			if(!duck_timer[d]){
				duck_timer[d] = DUCK_SPIN_FRAMES;
				duck_frame[d]++;//will happen first update after STATE_HIT to make frame 7
				if(duck_frame[d] > 10)
					duck_frame[d] = 7;
			}
			if((duck_y[d]>>8) >= DUCK_START_Y){
				duck_state[d] = STATE_KILLED;
				TriggerFx(SFX_GROUND,255,1);
				SetTile((DUCK_SCORE_X+10)-(targetsRemaining+d), DUCK_SCORE_Y, DUCK_SCORE_RED);
				continue;//no draw
			}
			if(duck_frame[d] == 9){//frame 9 is a mirrored frame 7...
				dframe = 7;
				dmirror = 1;
			}else if(duck_frame[d] == 10){
				dframe = 9;
			}else
				dframe = duck_frame[d];
		}

		uint16_t map_pos = (uint16_t)(2+(dframe*5*5));
		uint8_t t;
		uint8_t dx = (duck_x[d]/256UL);
		uint8_t dy = (duck_y[d]/256UL);
		if(!dmirror){
			for(uint8_t i=0; i<5*5; i++){
				t = pgm_read_byte(&duck_map[map_pos]);
				int8_t xo = pgm_read_byte(&spart_x_offsets[map_pos]);
				int8_t yo = pgm_read_byte(&spart_y_offsets[map_pos++]);
				if(t)
					BlitSprite(SPRITE_BANK1, t, dx+xo, dy+yo);
			}
		}else{
			for(uint8_t i=0; i<5*5; i++){
				t = pgm_read_byte(&duck_map[map_pos]);
				int8_t xo = 40-pgm_read_byte(&spart_x_offsets[map_pos]);
				int8_t yo = pgm_read_byte(&spart_y_offsets[map_pos++]);
				if(t)
					BlitSprite(SPRITE_BANK1|SPRITE_FLIP_X, t, dx+xo, dy+yo);
			}
		}
	}
	return numActive;
}

uint8_t MinRoundScore(){
		switch(gameRound){
			case 0 ... 10: return 6;
			case 11 ... 12: return 7;
			case 13 ... 14: return 8;
			case 15 ... 19: return 9;
			default: return 10;
		}
}

uint8_t TallyScore(){
	uint8_t did_move = 1;
	while(did_move){
		did_move = 0;
		for(uint8_t i=DUCK_SCORE_X+9;i>DUCK_SCORE_X;i--){
			uint8_t t = vram[(DUCK_SCORE_Y*VRAM_TILES_H)+i];
			uint8_t t2 = vram[(DUCK_SCORE_Y*VRAM_TILES_H)+i-1];
			if(t > t2){
				vram[(DUCK_SCORE_Y*VRAM_TILES_H)+i-1] = t;
				vram[(DUCK_SCORE_Y*VRAM_TILES_H)+i] = t2;
				did_move = 1;
				TriggerFx(SFX_SCORE,255,1);
				WaitVsync(15);
			}
		}
	}
	uint8_t s;
	for(s=0;s<12;s++){
		if(vram[(DUCK_SCORE_Y*VRAM_TILES_H)+DUCK_SCORE_X+s] == DUCK_SCORE_RED)
			continue;
		break;
	}
	return (s < MinRoundScore()) ? 0:1;
}

void RoundClear(){
		StartSong(mus_round_clear);
		WaitVsync(180);
}

void GameOver(){
	StartSong(mus_game_over);
	WaitVsync(180);
}

void RedrawAmmo(){
	uint8_t t = AMMO_X+shotsRemaining;
	for(uint8_t i=AMMO_X+2;i>=t;i--){
		SetTile(i, AMMO_Y, AMMO_BLACK);
	}
}

void Shoot(){//hits are detected later, due to variable input latency
	if(!shotsRemaining || gunCooldown)
		return;

	TriggerFx(SFX_SHOOT,255,1);
	shotsRemaining--;
	//RedrawAmmo();
	gunCooldown = GUN_COOLDOWN;
	gunTargets = 0;
	for(uint8_t i=0;i<MAX_DUCKS;i++){
		if(duck_state[i] == STATE_ACTIVE)
			gunTargets++;
	}
	if(!gunTargets)
		return;

	BlackScreen(0);
	WaitVsync(10);
	gunTimer = 1;
	gunTargets = 0;
	for(uint8_t i=0;i<MAX_DUCKS;i++){
		if(duck_state[i] == STATE_ACTIVE){
			gunTargets++;
			DrawHitbox(i);
			//WaitVsync(60);
		}
	}
	//after gunLag+gunTargets frames, we check the buffer for specific hits...
}

void RedrawScore(){

	SetTile(SCORE_NUMS_X+6, SCORE_NUMS_Y, GAME_CHAR_ZERO);
	uint32_t val = score;
	for(uint8_t i=SCORE_NUMS_X+5;i>=SCORE_NUMS_X;i--){
		uint8_t c = val%10;
		SetFont(i,SCORE_NUMS_Y,c+GAME_CHAR_ZERO);
		val=val/10;
	}
	if(score < hiScore){//not the "HISCORE"? then blank out "HI"
		SetTile(SCORE_NUMS_X+0, SCORE_NUMS_Y-1, DUCK_SCORE_BLANK);
		SetTile(SCORE_NUMS_X+1, SCORE_NUMS_Y-1, DUCK_SCORE_BLANK);
	}
}

void RedrawHUD(){//because we do not use ram_tiles_restore[], we must redraw everything each frame
	if(GetVsyncFlag())//falling behind drawing sprites, bail out
		return;
	RedrawScore();//also called when a duck is hit
	
	uint16_t t = scoreBits;//redraw white/red duck icons(active flashing is handled by UpdateDucks())
	for(uint8_t i=0;i<10;i++){
		SetTile(DUCK_SCORE_X+i, DUCK_SCORE_Y, (t&1)?DUCK_SCORE_RED:DUCK_SCORE_WHITE);
		t >>= 1;
	}
	uint8_t m = MinRoundScore()+DUCK_SCORE_X;//cover over any targets more than round minimum
	for(uint8_t i=DUCK_SCORE_X+9;i>=m;i--)
		SetTile(i, DUCK_SCORE_Y+1, 0x86);

	SetTile(DUCK_SCORE_X-2, DUCK_SCORE_Y+1, (gameRound/10)+GAME_CHAR_ZERO);
	SetTile(DUCK_SCORE_X-1, DUCK_SCORE_Y+1, (gameRound%10)+GAME_CHAR_ZERO);
	
	RedrawAmmo();
}

void CheckDuckShot(){
	if(gunTimer <= gunLag+gunTargets || gunTimer >= MAX_GUN_LAG)
		return;
	uint8_t tnum = 0;
	for(uint8_t i=0;i<MAX_DUCKS;i++){
		if(duck_state[i] != STATE_ACTIVE)
			continue;
		if(GunSenseFrame(gunLag+tnum+1) && !GunSenseFrame(gunLag+tnum+0)){
			duck_state[i] = STATE_HIT;
			duck_timer[i] = DUCK_HIT_FRAMES;
			scoreBits |= (1<<(10-targetsRemaining)+i);//red duck icon...
			if(gameRound < 11)//as per original, duck types are worth different points...but all have the same behavior/difficulty!
				score += duck_type[i]*8;//800
			else
				score += duck_type[i]*10;//1000
			if(score > hiScore){
				hiScore = score;
				//TriggerFx(SFX_HISCORE,255,1);
			}
			RedrawScore();
		}
		tnum++;
	}
}

int main(){
	GetPrngNumber(GetTrueRandomSeed());
	if(GetPrngNumber(0) == 0)
		GetPrngNumber(0xACE);

	InitMusicPlayer(patches);
	SetMasterVolume(224);

	SetSpritesTileBank(0, sprite_data1);
	SetSpritesTileBank(1, sprite_data2);
	SetFontTilesIndex(0);
	snesMouseEnabled = 1;
	SetUserPostVsyncCallback(PostVsyncRoutine);
	SetUserPreVsyncCallback(PreVsyncRoutine);
gunLag = MAX_GUN_LAG;//force a calibration if LG is detected

	BootIntro();
	hiScore = 1200;
MAIN_TOP:
	TitleScreen();
	gameRound = 1;
ROUND_TOP:
	targetsRemaining = 10;
	shotsRemaining = 3;
	scoreBits = 0;
	RedrawHUD();
	DogIntro();
	InitDucks();
	while(targetsRemaining){//still have targets left this round?
		WaitVsync(1);
		if(!(joypad1_status_lo & LG_TRIGGER) && (last_joypad1_status_lo & LG_TRIGGER))
			Shoot();
		else
			DrawMap2(0, 0, map_main);

		CheckDuckShot();//directly sets duck state if a shot is detected...
		if(!UpdateDucks(0)){
			targetsRemaining -= (1+gameType);
			DogRetrieve();
			InitDucks();
			shotsRemaining = 3;
		}
		RedrawHUD();
	}
	if(TallyScore()){//enough to make next round?
		RoundClear();
		gameRound++;
		goto ROUND_TOP;
	}//else game over
	GameOver();
	goto MAIN_TOP;
	return 0;
} 


uint8_t CalibrateGun(){//determines the input latency, returns 0 if no black to white sequence was found
	for(uint8_t i=0;i<128;i++)
		ram_tiles[i] = (i<64)?0x00:0xFF;
	FillVram(0);//cover the screen with black
	WaitVsync(MAX_GUN_LAG+1);//make sure the gun should see black
	gunLag = 1;
	while(1){
		FillVram(1);
		WaitVsync(1);
		if(!(last_joypad1_status_lo & LG_SENSE) && (joypad1_status_lo & LG_SENSE))
			break;

		gunLag++;
		if(gunLag == MAX_GUN_LAG)
			break;//return MAX_GUN_LAG;
	}
	//ClearVram();
	FillVram(0);//not necessary? a gun that reports sense more than 1 frame will still work?!
	WaitVsync(gunLag);
	//_emu_whisper(0,gunLag);
	ResetGunSenseBuffer();
	return MAX_GUN_LAG;
}


uint8_t rtl_FontStartOffset;
int8_t *rtl_CharMap;

inline void rtl_SetVram(uint8_t x, uint8_t y, uint8_t t){
	vram[(y*VRAM_TILES_H)+x] = t;
}

uint8_t rtl_FindChar(int8_t ch, const int8_t *fmap){
	uint8_t pos = 0;

	int8_t t;
	while(true){
		t = pgm_read_byte(&fmap[pos]);

		if(t == 0 || pos == 255)
			return 255;

		if(t != ch){
			pos++;
			continue;
		}
		return pos+rtl_FontStartOffset;
	}
}


uint8_t rtl_SearchCharList(int8_t ch, const int8_t *chrlst){
	int8_t pos=0,t;
	while(true){
		t = pgm_read_byte(&chrlst[pos]);
		if(t == 0 || t == 255)
			return 255;
		if(t == ch)
			return pos;
		pos++;
	}
}

void rtl_RamifyFontEx(uint16_t rtoff, const int8_t *cmap, const int8_t *chrlst, const uint8_t *ftiles, uint8_t backcolor, uint8_t fontcolor){
//WaitVsync(1);
	uint8_t moff = 0;
	uint8_t t,t2;
	uint8_t c;
	rtl_FontStartOffset = rtoff;
	rtl_CharMap = (int8_t *)cmap;
	rtoff *= 64;
	uint8_t iteration = 0;

	while(rtoff < (RAM_TILES_COUNT*64)){
		c = pgm_read_byte(&cmap[moff++]);
		if(c == 0)
			return;
		

		t2 = rtl_SearchCharList(c,chrlst);

		if(t2 == 255){
			return;	
		}

		for(uint16_t i=(t2*8);i<(t2*8)+8;i++){
			t = pgm_read_byte(&ftiles[i]);
			for(u8 j=0;j<8;j++){
				if(t & (128>>j)){
						ram_tiles[rtoff] = fontcolor;
				}
				else
					ram_tiles[rtoff] = backcolor;
				rtoff++;
			}
		}
//	if(++iteration > 1)
//		WaitVsync(1);	
	}
}


void rtl_PrintEx(uint8_t x, uint8_t y, const char *string, const int8_t *cmap, uint8_t offset, uint8_t flags){
	uint8_t t,i=0;
	int8_t c;

	while(true){
		c = pgm_read_byte(&string[i++]);
		
		if(c == 0)
			return;

		if(x > VRAM_TILES_H-1){
			x++;
			continue;
		}		
		else if(c == ' '){
			//if(blank != 255)
		//	rtl_SetVram(x,y,blank);
			x++;
			continue;
		}

		t = rtl_FindChar(c,cmap);

		if(t > RAM_TILES_COUNT){
			x++;
			continue;
		}

		rtl_SetVram(x++,y,t);
	}
}



void rtl_PrintRamEx(uint8_t x, uint8_t y,char *string, const int8_t *cmap, uint8_t offset, uint8_t flags){
	uint8_t t,i=0;
	int8_t c;

	while(true){
		c = string[i++];
		
		if(c == 0)
			return;

		if(x > VRAM_TILES_H-1){
			x++;
			continue;
		}		
		else if(c == ' '){
			//if(blank != 255)
		//	rtl_SetVram(x,y,blank);
			x++;
			continue;
		}

		t = rtl_FindChar(c,cmap);

		if(t > RAM_TILES_COUNT){
			x++;
			continue;
		}

		rtl_SetVram(x++,y,t);
	}
}


void rtl_Print(uint8_t x, uint8_t y, const char *string){
	rtl_PrintEx(x,y,string,rtl_CharMap,rtl_FontStartOffset,0);
}

void rtl_PrintRam(uint8_t x, uint8_t y,char *string){
	rtl_PrintRamEx(x,y,string,rtl_CharMap,rtl_FontStartOffset,0);
}

inline void rtl_Print1num(uint8_t x, uint8_t y, uint8_t val){
	vram[(y*VRAM_TILES_H)+x] = 12+val;
}

void rtl_Print2num(uint8_t x, uint8_t y, uint16_t val){
	if(val > 99)
		val = 99;

	rtl_Print1num(x--,y,(val%10));
	rtl_Print1num(x,y,(val/10));	
}

void rtl_Print3num(uint8_t x, uint8_t y, uint16_t val){
	if(val > 999)
		val = 999;

	rtl_Print1num(x--,y,(val%10));
	rtl_Print1num(x--,y,(val%100)/10);
	rtl_Print1num(x,y,(val/100));
}

void rtl_Print4num(int x,int y, unsigned long val){
	unsigned char c,i;

	for(i=0;i<4;i++){
		c=val%10;
		if(val>0 || i==0){
			rtl_Print1num(x--,y,c);
		}else{
			rtl_Print1num(x--,y,0);
		}
		val=val/10;
	}
		
}