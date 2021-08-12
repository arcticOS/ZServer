/*
 * ZServer
 * Copyright (c) 2021 Johnny Stene <jhonnystene@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * This software contains code from Waveshare, released under the following license:
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/ioctl.h>

#include <display/framebuffer.hpp>

int fbfd = 0;
char *fbp = 0;

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

int page_size = 0;
int cur_page = 0;

struct fb_var_screeninfo orig_vinfo;
long int screensize = 0;

char *kbfds = "/dev/tty";
int kbfd = open(kbfds, O_WRONLY);

int init_framebuffer() {
    fbfd = open("/dev/fb0", O_RDWR);
    if(fbfd == -1) {
        printf("FATAL: Couldn't open framebuffer at /dev/fb0\n");
        return -1;
    }

    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
    }

    memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));
    vinfo.bits_per_pixel = 8;
    vinfo.xres_virtual = vinfo.xres;
    vinfo.yres_virtual = vinfo.yres;
    if(ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
        printf("Error setting variable information.\n");
    }
    if (kbfd >= 0) {
        ioctl(kbfd, KDSETMODE, KD_GRAPHICS);
    }
    else {
        printf("Could not open %s.\n", kbfds);
    }

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
    }

    page_size = finfo.line_length * vinfo.yres;

    // map fb to user mem
    screensize = finfo.smem_len;
    fbp = (char*)mmap(0,
            screensize,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fbfd,
            0);

    return 0;
}

void release_framebuffer() {
    munmap(fbp, screensize);
    // reset cursor
    if (kbfd >= 0) {
        ioctl(kbfd, KDSETMODE, KD_TEXT);
        // close kb file
        close(kbfd);
    }
    // reset the display mode
    if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
        printf("Error re-setting variable information.\n");
    }
    // close fb file    
    close(fbfd);
}

void clear_screen(int c) {
    // Set whole block to color
    memset(fbp + cur_page * page_size, c, page_size);
}

void put_pixel(int x, int y, int c) {
    // Write pixel data to offset
    unsigned int pix_offset = x + y * finfo.line_length;
    pix_offset += cur_page * page_size;
    *((char*)(fbp + pix_offset)) = c;
}