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
        if (fb_screen)
            SDL_Flip(fb_screen);
        fprintf(stderr, "keyboard select!!\n");
        int sz, i;
        SDL_PumpEvents();
        uint8_t *cstate = SDL_GetKeyState(&sz);
        uint8_t key;
        for (i = 0; i < sz; i++) {
            if (cstate[i] != vkbd_sdlstate[i]) {
                switch (i) {
                case SDLK_ESCAPE: key = 1; break;
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
                case SDLK_LSHIFT: key = 42; break;
                case SDLK_z: key = 44; break;
                case SDLK_x: key = 45; break;
                case SDLK_c: key = 46; break;
                case SDLK_v: key = 47; break;
                case SDLK_b: key = 48; break;
                case SDLK_n: key = 49; break;
                case SDLK_m: key = 50; break;
                case SDLK_LALT: key = 56; break;
                case SDLK_SPACE: key = 57; break;
                case SDLK_UP: key = 103; break;
                case SDLK_RIGHT: key = 106; break;
                case SDLK_DOWN: key = 108; break;
                case SDLK_LEFT: key = 105; break;
                case SDLK_MENU: key = 127; break;
                default: key = i; break;
                }
                if (cstate[i] == 0 && vkbd_sdlstate[i] == 1) {
                    key |= 0x80;	/* released */
                }
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
            fprintf(stderr, "table %d index %d value %d\n", kbe->kb_table, kbe->kb_index, kbe->kb_value);
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
