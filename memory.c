/*
 * memory.c: memory and I/O functions for Atari Vector game simulator
 *
 * Copyright 1991, 1993, 1996, 2014 Hedley Rainnie and Eric Smith
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
 * $Header: /usr2/eric/vg/atari/vecsim/RCS/memory.c,v 1.23 1997/05/17 22:25:36 eric Exp $
 */
#include "stm32f4_discovery.h"
#include "stm32f4xx.h"
#include "memory.h"
#include "display.h"
#include "game.h"
#include "misc.h"
#include "sim6502.h"
#include "audio.h"
#include "data.h"
#include "board.h"

/*
 * This used to decrement the switch variable if it was non-zero, so that
 * they would automatically release.  This has been changed to increment
 * it if less than zero, so switches set by the debugger will release, but
 * to leave it alone if it is greater than zero, for keyboard handling.
 */
s32 
check_switch_decr(s32 *sw)
{
    if((*sw) < 0) {
	(*sw)++;
	if((*sw) == 0)
	    ;
    }
    return ((*sw) != 0);
}
u8 
MEMRD(u32 addr, s32 PC, u32 cyc)
{
    u8 tag;
    u8 result=0;
    
    if(!(tag=g_sys_mem[addr].tagr)) {
	return(g_sys_mem[addr].cell);
    }
    
    switch(tag & 0x3f) {
    case MEMORY:
	result = g_sys_mem[addr].cell;
	break;
    case MEMORY1:
	result = g_sys_sram[addr&0x3ff];
	break;
    case MEMORY_BB:
	for(;;);
    case VECRAM:
	result = g_sys_vram[addr&0x7ff];
	break;
    case COININ:
	result =
	    ((! check_switch_decr(&g_sys_cslot_right))) |
	    ((! check_switch_decr(&g_sys_cslot_left)) << 1) |
	    ((! check_switch_decr(&g_sys_cslot_util)) << 2) |
	    ((! check_switch_decr(&g_sys_slam)) << 3) |
	    ((! g_sys_self_test) << 4) |
	    (1 << 5) | /* signature analysis */
	    (vg_done(cyc) << 6) |
	    /* clock toggles at 3 KHz */
	    ((cyc >> 1) & 0x80);
	
	break;
    case EAROMRD:
	result = 0;
	break;
    case OPTSW1:
	result = g_sys_optionreg[0];
	break;
    case OPTSW2:
	result = g_sys_optionreg[1];
	break;
    case OPT1_2BIT:
	result = 0xfc | ((g_sys_optionreg[0] >> (2 * (3 - (addr & 0x3)))) & 0x3);
	break;
    case ASTEROIDS_SW1:
	g_soc_curr_switch = read_gpio();
	switch(addr & 0x07) {
	case 0: /* nothing */
	    result = 0;
	    break;
	case 1: /* 3 KHz */
	    /* clock toggles at 3 KHz */
	    result = ((cyc >> 2) & 0x80);
	    break;
	case 2: /* vector generator halt */
	    result = (!vg_done(cyc)) << 7;
	    break;
	case 3: /* hyperspace */
//	    result = switches [0].hyper << 7;
	    result = (g_soc_curr_switch & HYPER_PIN) ? 0x80 : 0;
	    break;
	case 4: /* fire */
//	    result = switches [0].fire << 7;
	    result = (g_soc_curr_switch & FIRE_PIN) ? 0x80 : 0;
	    break;
	case 5: /* diag step */
	    result = 0;
	    break;
	case 6: /* slam */
	    result = check_switch_decr(&g_sys_slam) << 7;
	    break;
	case 7: /* self test */
	    result = g_sys_self_test << 7;
//	    g_sys_self_test = result = (g_soc_curr_switch & SELF_TEST_PIN) ? 0x80 : 0;
	    break;
	}
	break;
    case ASTEROIDS_SW2:
	g_soc_curr_switch = read_gpio();
	switch(addr & 0x07) {
	case 0: /* left coin */
	    result = !(check_switch_decr(&g_sys_cslot_left) << 7);
	    break;
	case 1: /* center coin */
	    result = !(check_switch_decr(&g_sys_cslot_util) << 7);
	    break;
	case 2: /* right coin */
	    result = !(check_switch_decr(&g_sys_cslot_right) << 7);
	    break;
	case 3: /* 1P start */
//	    result = check_switch_decr(&g_sys_start1) << 7;
	    result = (g_soc_curr_switch & START1_PIN) ? 0x80 : 0;
	    break;
	case 4: /* 2P start */
//	    result = check_switch_decr(&g_sys_start2) << 7;
	    result = (g_soc_curr_switch & START2_PIN) ? 0x80 : 0;
	    break;
	case 5: /* thrust */
//	    result = switches [0].thrust << 7;
	    result = (g_soc_curr_switch & THRUST_PIN) ? 0x80 : 0;
	    break;
	case 6: /* rot right */
//	    result = switches [0].right << 7;
	    result = (g_soc_curr_switch & RIGHT_PIN) ? 0x80 : 0;
	    break;
	case 7: /* rot left */
//	    result = switches [0].left << 7;
	    result = (g_soc_curr_switch & LEFT_PIN) ? 0x80 : 0;
	    break;
	}
	break;
    case UNKNOWN:
	result = 0xff;
	break;
    default:
	result = 0xff;
	break;
    }
    if(tag & BREAKTAG) {
	;
    }
    return(result);
}
void 
MEMWR(u32 addr, s32 val, s32 PC, u32 cyc)
{
    u8                 tag;
    u32    aud_saucer_fire;
    s32            newbank;
    s32                  i;
    u8                temp;

    if(!(tag=g_sys_mem[addr].tagw)) {
	g_sys_mem[addr].cell = val;
    } else {
	switch(tag & 0x3f) {
	case MEMORY:
	    g_sys_mem[addr].cell = val;
	    break;
	case MEMORY1:
	    g_sys_sram[addr&0x3ff] = val;
	    break;
	case MEMORY_BB:
	    break;
	case VECRAM:
	    g_sys_vram[addr&0x7ff] = val;
	    break;
	case COINOUT:
	    newbank = (val>>2) & 1;
	    g_sys_bank = newbank;
	    break;
	case INTACK:
	    g_cpu_irq_cycle = cyc + 6144;
	    break;
	case WDCLR:
	case EAROMCON:
	case EAROMWR:
	    /* none of these are implemented yet, but they're OK. */
	    break;
	case VGRST:
	    vg_reset(cyc);
	    break;
	case VGO:
	    g_vctr_vg_count++;
	    while(0 == g_soc_sixty_hz);
	    g_soc_sixty_hz = 0;
	    vg_go(cyc);
	    break;
	case DMACNT:
	    break;
	case COLORRAM:
	    break;
	case TEMP_OUTPUTS:
	    break;
	case ASTEROIDS_OUT:
	    newbank = (val >> 2) & 1;
	    if(newbank != g_sys_bank) {
		for(i=0;i < 0x100;i++) {
		    temp = g_sys_sram[0x200+i];
		    g_sys_sram[0x200+i] = g_sys_sram[0x300+i];
		    g_sys_sram[0x300+i] = temp;
		}
		g_sys_bank = newbank;
	    }
	    write_gpio_leds(val);
	    break;
	case ASTEROIDS_EXP:
	    if(val & 0x3c) {
		s32 explosion = (val >> 6) & 3;
		if(explosion != g_aud_explosion) {
		    if(g_aud_curr_exp) {
			disable_sound(g_aud_curr_exp);
		    }
		    switch(explosion) {
		    case 0:
		    case 1:
			g_aud_curr_exp = EXPLODE1;
			break;
		    case 2:
			g_aud_curr_exp = EXPLODE2;
			break;
		    case 3:
			g_aud_curr_exp = EXPLODE3;
			break;
		    }
		    enable_sound(g_aud_curr_exp);
		}
		g_aud_explosion = explosion;
	    } else {
		g_aud_explosion = -1;
	    }
	    break;
	case ASTEROIDS_THUMP:
	    if(val & 0x10) {
		if(val & 0xf) { 
		    g_aud_curr_thump = THUMPLO;
		} else {
		    g_aud_curr_thump = THUMPHI;
		}
		enable_sound(g_aud_curr_thump);
	    }
	    break;
	case ASTEROIDS_SND:
	    switch(addr & 7) {
	    case 0: // saucer
		if(val & 0x80 && !(g_aud_last_saucer & 0x80)) {
		    enable_sound(g_aud_curr_saucer);
		}
		if(!(val & 0x80) && (g_aud_last_saucer & 0x80)) {
		    disable_sound(g_aud_curr_saucer);
		}
		g_aud_last_saucer = val;
		break;
	    case 1: // saucer fire
		aud_saucer_fire = val & 0x80;
		if(aud_saucer_fire != g_aud_saucer_fire) {
		    disable_sound(SFIRE);
		    if(aud_saucer_fire) {
			enable_sound(SFIRE);
		    }
		}
		aud_saucer_fire = g_aud_saucer_fire;
		break;
	    case 2: // saucer select
		if(val & 0x80) {
		    g_aud_curr_saucer = LSAUCER;
		} else {
		    g_aud_curr_saucer = SSAUCER;
		}
		break;
	    case 3: // thrust sound
		if(val & 0x80) {
		    enable_sound(THRUST);
		} else {
		    disable_sound(THRUST);
		}
		break;
	    case 4: // fire sound
		if(val & 0x80) {
		    enable_sound(FIRE);
		} else {
		    disable_sound(FIRE);
		}
		break;
	    case 5: // Life
		if(val & 0x80) {
		    enable_sound(LIFE);
		} else {
		    disable_sound(LIFE);
		}
		break;
	    default:
		for(;;);
	    }
	    break;
	case ASTEROIDS_SND_RST:
	    break;
	case IGNWRT:
	    break;
	case ROMWRT:
	    break;
	case UNKNOWN:
	    break;
	default:
	    break;
	}
    }
}
