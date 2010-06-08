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

SDL_Surface *fb_screen = NULL;

int vkbd_fd = -1;
uint8_t *vkbd_sdlstate = NULL;
uint8_t key_buf[1024];
int key_buf_pos = 0;

vkbd_t vkbd = {
    .state = K_XLATE,
    .mode = KD_TEXT,
};


int do_virtual_tty_select(int *ret, int n, fd_set *rfds_ptr, fd_set *wfds_ptr,
                          fd_set *efds_ptr, struct timeval *tv_ptr)
{
    if (vkbd_fd != -1 && n > vkbd_fd && FD_ISSET(vkbd_fd, rfds_ptr)) {
        fprintf(stderr, "keyboard select!!\n");
        int sz, i;
        SDL_PumpEvents();
        uint8_t *cstate = SDL_GetKeyState(&sz);
        uint8_t key;
        for (i = 0; i < sz; i++) {
            if (cstate[i] != vkbd_sdlstate[i]) {
                switch (i) {
                case SDLK_UP: key = 103; break;
                case SDLK_RIGHT: key = 106; break;
                case SDLK_DOWN: key = 108; break;
                case SDLK_LEFT: key = 105; break;
                case SDLK_LALT: key = 56; break;
                case SDLK_RETURN: key = 28; break;
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
    return 0;
}

int vfbcon_ioctl_ptr(int access, int *ret, int fd, int request, void *argptr)
{
    switch (request) {
        case KDGKBMODE:
            *(abi_long *)argptr = vkbd.state;
            *ret = 0;
            break;
        default:
            return 0;
    };
    return 1;
}
