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

#include <SDL/SDL.h>
#include "qemu.h"

extern SDL_Surface *fb_screen;
extern int vkbd_fd;
extern uint8_t *vkbd_sdlstate;
extern uint8_t key_buf[1024];
extern int key_buf_pos;

typedef struct {
    abi_long state;
    abi_long mode;
} vkbd_t;

extern vkbd_t vkbd;

int do_virtual_tty_select(int *ret, int n, fd_set *rfds_ptr, fd_set *wfds_ptr,
                          fd_set *efds_ptr, struct timeval *tv_ptr);

int vfbcon_ioctl_null(int *ret, int fd, int request);
int vfbcon_ioctl_int(int *ret, int fd, int request, abi_long arg);
int vfbcon_ioctl_ptr(int access, int *ret, int fd, int request, void *argptr);
