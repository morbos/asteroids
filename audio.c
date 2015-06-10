/*
 * audio.c: Asteroids audio support
 *
 * Copyright 2014 Hedley Rainnie
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Header$
 */
#include "stm32f4_discovery.h"
#include "memory.h"
#include "data.h"
#include "audio.h"

const s16 explode1[]={
#include "explode1_48.hex"
};
const s16 explode2[]={
#include "explode2_48.hex"
};
const s16 explode3[]={
#include "explode3_48.hex"
};
const s16 life[]={
#include "life48.hex"
};
const s16 fire[]={
#include "fire48.hex"
};
const s16 sfire[]={
#include "sfire48.hex"
};
const s16 thumplo[]={
#include "thumplo48.hex"
};
const s16 thumphi[]={
#include "thumphi48.hex"
};
const s16 lsaucer[]={
#include "lsaucer48.hex"
};
const s16 ssaucer[]={
#include "ssaucer48.hex"
};
const s16 thrust[]={
#include "thrust48.hex"
};

// Add this to the mix

typedef struct _sound_rec {
    const s16 *ptr;
    u32 len; // Len of the sample after processing
    u32 idx; // Curr index into the sample. 
    u32 oneshot; // When 1 don't repeat
} sound_rec;

sound_rec sounds[]={
    { 0, 0, 0, 0 }, 
    { explode1, 0, 0, 1}, 
    { explode2, 0, 0, 1 }, 
    { explode3, 0, 0, 1 }, 
    { life, 0, 0, 1 }, 
    { fire, 0, 0, 1 }, 
    { sfire, 0, 0, 1 }, 
    { thumplo, 0, 0, 1 },
    { thumphi, 0, 0, 1 }, 
    { lsaucer, 0, 0, 0 }, 
    { ssaucer, 0, 0, 0 }, 
    { thrust, 0, 0, 0 }
};
void
add_sounds()
{
    int i;
    u32 len;
    for(i=0;i < sizeof(sounds)/sizeof(sound_rec);i++) {
	u32 p = (u32)sounds[i].ptr;
	if(p) {
	    len = REG32(p+0x28);
	    len /= 2; // Len in 16bit samples
	    len += AUDACITY_WAV_HDR_OFF;
	    sounds[i].len = len;
	    sounds[i].idx = AUDACITY_WAV_HDR_OFF;
	}
    }
}
void
enable_sound(u32 idx)
{
    g_aud_smask |= bit(idx);
}
void
disable_sound(u32 idx)
{
    g_aud_smask &= ~bit(idx);
}
s16
get_sample()
{
    u32 i;
    s16 worklist[16];
    u16 idx=0;
    s16 mixer(s16 *dat, u16 factor, u32 n);
    for(i=1;i < sizeof(sounds)/sizeof(sound_rec);i++) {
	if(g_aud_smask & bit(i)) {
	    worklist[idx++] = sounds[i].ptr[sounds[i].idx];
	    sounds[i].idx++;
	    if(sounds[i].idx == sounds[i].len) {
		sounds[i].idx = AUDACITY_WAV_HDR_OFF;
		if(sounds[i].oneshot) {
		    g_aud_smask &= ~bit(i);
		}
	    }
	}
    }
    if(idx == 1) {
	return worklist[0];
    } else {
	return mixer(worklist, 32768 / idx, idx<<1);
    }
}
