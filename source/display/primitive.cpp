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

#include <stdlib.h>

#include <display/framebuffer.hpp>
#include <display/primitive.hpp>

void fill_rect(int x, int y, int w, int h, int c) {
    h /= 4; // What the fuck
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