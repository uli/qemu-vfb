/*
 *  Virtual Linux framebuffer console
 *
 *  Copyright (c) 2010 Ulrich Hecht
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "vfbcon.h"
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>
#include <asm/ioctls.h>

SDL_Surface *fb_screen = NULL;

int vkbd_fd = -1;
uint8_t *vkbd_sdlstate = NULL;
uint8_t key_buf[1024];
int key_buf_pos = 0;

vkbd_t vkbd = {
    .state = K_XLATE,
    .mode = KD_TEXT,
};

struct fb_var_screeninfo vfb_var = {
    .xres = 320,
    .yres = 240,
    .xres_virtual = 320,
    .yres_virtual = 240,
    .xoffset = 0,
    .yoffset = 0,
    .bits_per_pixel = 16,
    .grayscale = 0,
    .red = {0, 5, 0},
    .blue = {5, 6, 0},
    .green = {11, 5, 0},
    .nonstd = 0,
    .height = 50,
    .width = 100,
};
int fb_fd = -1;

static void dump_vscreen(void)
{
    fprintf(stderr, "xres %d yres %d xres_virtual %d yres_virtual %d\n", vfb_var.xres, vfb_var.yres, vfb_var.xres_virtual, vfb_var.yres_virtual);
    fprintf(stderr, "xoffset %d yoffset %d bpp %d grayscale %d\n", vfb_var.xoffset, vfb_var.yoffset, vfb_var.bits_per_pixel, vfb_var.grayscale);    
}

struct fb_fix_screeninfo vfb_fix = {
    .smem_start = 0xdead0000,
    .smem_len = 262144,
    .type = FB_TYPE_PACKED_PIXELS,
    .visual = FB_VISUAL_DIRECTCOLOR,
    .line_length = 640,
    .mmio_start = 0xcafe0000,
    .mmio_len = 0x42,
};

void vfb_update_screen(void)
{
    static uint64_t time = 0;
    uint64_t newtime;
    struct timeval tv;
    
    if (!fb_screen)
        return;
    
    gettimeofday(&tv, NULL);
    newtime = tv.tv_sec * 1000000 + tv.tv_usec;
    
    if (newtime - time > 1000000 / 60) {
        time = newtime;
        SDL_Flip(fb_screen);
    }
}

int do_virtual_fb_mmap(abi_ulong *ret, abi_ulong start, abi_ulong len,
                       int prot, int flags, int fd, abi_ulong offset)
{
    int kbd_state_size;
    uint8_t *kbdstate;
    if (fd > 0 && fd == fb_fd) {
        if (!fb_screen) {
            /* FIXME: Where's SDL_Init()? */
            fb_screen = SDL_SetVideoMode(vfb_var.xres, vfb_var.yres, vfb_var.bits_per_pixel, SDL_HWSURFACE);
            if (!fb_screen) {
                fprintf(stderr, "Could not set video mode: %s\n", SDL_GetError());
                abort();
            }
            SDL_WM_SetCaption("QEMU Virtual Framebuffer", NULL);
            kbdstate = SDL_GetKeyState(&kbd_state_size);
            vkbd_sdlstate = malloc(kbd_state_size); /* FIXME: When to release this? */
            memcpy(vkbd_sdlstate, kbdstate, kbd_state_size);
        }
        *ret = h2g(fb_screen->pixels + offset);
        return 1;
    } else {
        return 0;
    }
}

int do_virtual_tty_select(int *ret, int n, fd_set *rfds_ptr, fd_set *wfds_ptr,
                          fd_set *efds_ptr, struct timeval *tv_ptr)
{
    if (vkbd_fd != -1 && n > vkbd_fd && FD_ISSET(vkbd_fd, rfds_ptr)) {
        int sz, i;
        SDL_PumpEvents();
        uint8_t *cstate = SDL_GetKeyState(&sz);
        uint8_t key;
        for (i = 0; i < sz; i++) {
            if (cstate[i] != vkbd_sdlstate[i]) {
                switch (i) {
                case SDLK_ESCAPE: key = 1; break;
                case SDLK_1: key = 2; break;
                case SDLK_2: key = 3; break;
                case SDLK_3: key = 4; break;
                case SDLK_4: key = 5; break;
                case SDLK_5: key = 6; break;
                case SDLK_6: key = 7; break;
                case SDLK_7: key = 8; break;
                case SDLK_8: key = 9; break;
                case SDLK_9: key = 10; break;
                case SDLK_0: key = 11; break;
                case SDLK_MINUS: key = 12; break;
                case SDLK_EQUALS: key = 13; break;
                case SDLK_BACKSPACE: key = 14; break;
                case SDLK_TAB: key = 15; break;
                case SDLK_q: key = 16; break;
                case SDLK_w: key = 17; break;
                case SDLK_e: key = 18; break;
                case SDLK_r: key = 19; break;
                case SDLK_t: key = 20; break;
                case SDLK_y: key = 21; break;
                case SDLK_u: key = 22; break;
                case SDLK_i: key = 23; break;
                case SDLK_o: key = 24; break;
                case SDLK_p: key = 25; break;
                case SDLK_LEFTBRACKET: key = 26; break;
                case SDLK_RIGHTBRACKET: key = 27; break;
                case SDLK_RETURN: key = 28; break;
                case SDLK_LCTRL: key = 29; break;
                case SDLK_a: key = 30; break;
                case SDLK_s: key = 31; break;
                case SDLK_d: key = 32; break;
                case SDLK_f: key = 33; break;
                case SDLK_g: key = 34; break;
                case SDLK_h: key = 35; break;
                case SDLK_j: key = 36; break;
                case SDLK_k: key = 37; break;
                case SDLK_l: key = 38; break;
                case SDLK_SEMICOLON: key = 39; break;
                case SDLK_QUOTE: key = 40; break;
                case SDLK_BACKQUOTE: key = 41; break;
                case SDLK_LSHIFT: key = 42; break;
                case SDLK_BACKSLASH: key = 43; break;
                case SDLK_z: key = 44; break;
                case SDLK_x: key = 45; break;
                case SDLK_c: key = 46; break;
                case SDLK_v: key = 47; break;
                case SDLK_b: key = 48; break;
                case SDLK_n: key = 49; break;
                case SDLK_m: key = 50; break;
                case SDLK_COMMA: key = 51; break;
                case SDLK_PERIOD: key = 52; break;
                case SDLK_SLASH: key = 53; break;
                case SDLK_RSHIFT: key = 54; break;
                case SDLK_KP_MULTIPLY: key = 55; break;
                case SDLK_LALT: key = 56; break;
                case SDLK_SPACE: key = 57; break;
                case SDLK_F1: key = 59; break;
                case SDLK_F2: key = 60; break;
                case SDLK_F3: key = 61; break;
                case SDLK_F4: key = 62; break;
                case SDLK_F5: key = 63; break;
                case SDLK_F6: key = 64; break;
                case SDLK_F7: key = 65; break;
                case SDLK_F8: key = 66; break;
                case SDLK_F9: key = 67; break;
                case SDLK_F10: key = 68; break;
                case SDLK_NUMLOCK: key = 69; break;
                case SDLK_SCROLLOCK: key = 70; break;
                case SDLK_KP7: key = 71; break;
                case SDLK_KP8: key = 72; break;
                case SDLK_KP9: key = 73; break;
                case SDLK_KP_MINUS: key = 74; break;
                case SDLK_KP4: key = 75; break;
                case SDLK_KP5: key = 76; break;
                case SDLK_KP6: key = 77; break;
                case SDLK_KP_PLUS: key = 78; break;
                case SDLK_KP1: key = 79; break;
                case SDLK_KP2: key = 80; break;
                case SDLK_KP3: key = 81; break;
                case SDLK_KP0: key = 82; break;
                case SDLK_KP_PERIOD: key = 83; break;
                case SDLK_F11: key = 87; break;
                case SDLK_F12: key = 88; break;
                case SDLK_KP_ENTER: key = 96; break;
                case SDLK_KP_DIVIDE: key = 98; break;
                case SDLK_SYSREQ: key = 99; break; /* or is it SDLK_PRINT? */
                case SDLK_RALT: key = 100; break;
                case SDLK_HOME: key = 102; break;
                case SDLK_UP: key = 103; break;
                case SDLK_PAGEUP: key = 104; break;
                case SDLK_LEFT: key = 105; break;
                case SDLK_RIGHT: key = 106; break;
                case SDLK_END: key = 107; break;
                case SDLK_DOWN: key = 108; break;
                case SDLK_PAGEDOWN: key = 109; break;
                case SDLK_INSERT: key = 110; break;
                case SDLK_DELETE: key = 111; break;
                case SDLK_POWER: key = 116; break;
                case SDLK_BREAK: key = 119; break; /* or is it SDLK_PAUSE? */
                case SDLK_LSUPER: key = 125; break;
                case SDLK_RSUPER: key = 126; break;
                case SDLK_MENU: key = 127; break;
                default:
                    fprintf(stderr, "Unmapped key %s\n", SDL_GetKeyName(i));
                    key = 255;
                    break;
                }
                if (cstate[i] == 0 && vkbd_sdlstate[i] == 1) {
                    key |= 0x80;	/* released */
                }
                if (key != 255)
                    key_buf[key_buf_pos++] = key;
            }
        }
        memcpy(vkbd_sdlstate, cstate, sz);
        if (key_buf_pos > 0)
            *ret = 1;
        else
            *ret = 0;
        return 1;
    }
    return 0;
}

int vfbcon_ioctl_null(int *ret, int fd, int request)
{
    return 0;
}

int vfbcon_ioctl_int(int *ret, int fd, int request, abi_long arg)
{
    switch (request) {
        case KDSETMODE:
            vkbd.mode = arg;
            *ret = 0;
            break;
        case KDSKBMODE:
            vkbd.state = arg;
            *ret = 0;
            break;
        default:
            return 0;
    };
    return 1;
}

#include "defkeymap.c_shipped"

int vfbcon_ioctl_ptr(int access, int *ret, int fd, int request, void *argptr)
{
    struct kbentry *kbe;
    switch (request) {
        case FBIOGET_FSCREENINFO:
            *(struct fb_fix_screeninfo *)argptr = vfb_fix;
            *ret = 0;
            break;
        case FBIOGET_VSCREENINFO:
            *(struct fb_var_screeninfo *)argptr = vfb_var;
            *ret = 0;
            break;
        case FBIOPUT_VSCREENINFO:
            vfb_var = *(struct fb_var_screeninfo *)argptr;
            dump_vscreen();
            *ret = 0;
            break;
        case KDGKBENT:
            kbe = (struct kbentry *)argptr;
            u_short *map = key_maps[kbe->kb_table];
            if (map) {
                kbe->kb_value = map[kbe->kb_index] & 0xfff;
            } else {
                if (kbe->kb_index)
                    kbe->kb_value = K_HOLE;
                else
                    kbe->kb_value = K_NOSUCHMAP;
            }
            *ret = 0;
            break;
        case KDGKBMODE:
            *(abi_long *)argptr = vkbd.state;
            *ret = 0;
            break;
        case TCGETS:
        case TCSETSF:
            *ret = 0;
            break;
        case VT_GETSTATE:
        case VT_LOCKSWITCH:
        case VT_UNLOCKSWITCH:
            *ret = -1;
            break;
        default:
            return 0;
    };
    return 1;
}
