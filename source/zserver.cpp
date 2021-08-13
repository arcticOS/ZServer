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
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

#include <display/framebuffer.hpp>
#include <display/primitive.hpp>

#include <server.hpp>

#include <zserver.hpp>

#define COLOR_WHITE 0xFF
#define COLOR_BLACK 0x00

#define COMMAND_CLEAR 0x0000
#define COMMAND_PIXEL 0x0001

int main(int argc, char** argv) {
    signal(SIGINT, clean_shutdown); // Make sure we release the framebuffer when exiting manually

    // Grab the framebuffer
    // NOTE: this will not work if using X already
    if(init_framebuffer() != 0) {
        return -1;
    }

    // Start socket handler
    // NOTE: we don't need multi-connection handling here
    if(init_server() != 0) {
        return -2;
    }

    // Execute ~/.zinitrc by forking and using system() to call .zinitrc
    // This is (technically) insecure, but isn't an issue since we're already execing a shell script anyway here
    if(fork() == 0) {
        system("bash $HOME/.zinitrc");
        exit(0);
    }

    // Listen for connection
    if(init_connection() != 0) {
        return -3;
    }

    for(;;) {
        char* buffer = server_read();

        // First two bytes: Command
        uint16_t command = buffer[0] << 8;
        command |= buffer[1];

        // Clear
        if(command == COMMAND_CLEAR) {
            clear_screen(COLOR_WHITE);
        } else if(command == COMMAND_PIXEL) {
            // Third byte: X
            // Fourth byte: Y
            // Fifth byte: Color
            put_pixel(buffer[2], buffer[3], buffer[4]);
        } 

        // TODO: Accept commands
        // TODO: Accept raw data

        server_write("OK");
    }

    release_framebuffer();
}

void clean_shutdown(int dummy) {
	release_framebuffer();
	exit(0);
}
