/* Mibitype - A small library to load fonts.
 * by Mibi88
 *
 * This software is licensed under the BSD-3-Clause license:
 *
 * Copyright 2024 Mibi88
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RENDER_H
#define RENDER_H

/* Some key codes. */
enum {
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_SPACE,
    KEY_LCTRL,
    KEY_LALT,
    KEY_LSHIFT,
    KEY_AMOUNT
};

typedef struct {
    int w, h;
    void *window;
    void *renderer;
    int fps;
} Renderer;

void render_init(Renderer *renderer, int width, int height, char *title);

void render_set_pixel(Renderer *renderer, int x, int y, int r, int g, int b);

void render_line(Renderer *renderer, int x1, int y1, int x2, int y2, int r,
                 int g, int b);

void render_rect(Renderer *renderer, int sx, int sy, int w, int h, int r,
                 int g, int b);

void render_update(Renderer *renderer);

void render_clear(Renderer *renderer, char black);

char render_keydown(Renderer *renderer, int key);

int render_get_width(Renderer *renderer);

int render_get_height(Renderer *renderer);

int render_ms(Renderer *renderer);

void render_show_fps(Renderer *renderer);

void render_main_loop(Renderer *renderer, void (*loop_function)(int));

#endif
