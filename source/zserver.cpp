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
 *
 * Drawing code based on https://github.com/JSBattista/Characters_To_Linux_Buffer_THE_HARD_WAY/blob/master/display.c
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

#include <zserver.hpp>

int fbfd = 0;
char *fbp = 0;

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

int page_size = 0;
int cur_page = 0;

int main(int argc, char** argv) {
    // Startup code
    struct fb_var_screeninfo orig_vinfo;
    long int screensize = 0;

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

    char *kbfds = "/dev/tty";
    int kbfd = open(kbfds, O_WRONLY);
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


    for(int x = 0; x < 1024; x++)
        fill_rect(100, 100, 256, 256, 0xFF);

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

void fill_rect(int x, int y, int w, int h, int c) {
    for(int cx = x; cx < x + w; cx++) {
        for(int cy = y; cy < y + h; cy++) {
            put_pixel(cx, cy, c);
        }
    }
}

void draw_rect(int x, int y, int w, int h, int c) {
    draw_hline(x, x + w, y, c);
    draw_hline(x, x + w, y + h, c);

    draw_vline(y, y + h, x, c);
    draw_vline(y, y + h, x + w, c);
}

// Fast horizontal line
void draw_hline(int x1, int x2, int y, int c) {
    for(int cx = x1; cx < x2; cx ++) {
        put_pixel(cx, y, c);
    }
}

// Fast vertical line
void draw_vline(int y1, int y2, int x, int c) {
    for(int cy = y1; cy < y2; cy ++) {
        put_pixel(x, cy, c);
    }
}

void draw_line(int x1, int y1, int x2, int y2, int c) {
    // Skip DDA if we're drawing a non-diagonal line
    if(x1 == x2) draw_vline(y1, y2, x1, c);
    else if(y1 == y2) draw_hline(x1, x2, y1, c);
    else { // DDA Line Drawing Algorithm
        int dx = x2 - x1;
        int dy = y2 - y1;
        int steps = 0;

        steps = abs(dy);
        if(abs(dx) > abs(dy)) steps = abs(dx);

        float x_inc = dx / (float) steps;
        float y_inc = dy / (float) steps;

        float x = x1;
        float y = y1;

        for(int v = 0; v < steps; v++) {
            put_pixel(x, y, c);
            x += x_inc;
            y += y_inc;
        }
    }
}