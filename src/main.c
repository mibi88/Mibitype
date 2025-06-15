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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <render.h>

#include <mibitype/font.h>

Renderer renderer;

MTFont font;

size_t selected;
char lock;

#define DEBUG_UTF8 0
#define DEBUG_METRICS 0
#define VIEW_GLYPHS 0

#define SCALE 1
#define WIDTH 320
#define HEIGHT 240

int points = 72;

#define PIXELS(s) mt_font_size_to_pixels(font, points, s)

void debug_render_glyph(MTFont *font, MTGlyph *glyph, int dx, int dy,
                        float scale){
    size_t point_num;
    int x, y, sx, sy;
    size_t i, n;

    int xmin, xmax, ymin, ymax;
    int advance_width, left_side_bearing;

    if(glyph->contour_ends == NULL) return;
    point_num = glyph->contour_ends[glyph->contour_num-1];
    if(!point_num) return;
    x = PIXELS(glyph->points->x*scale);
    y = PIXELS(glyph->points->y*scale);
    sx = x;
    sy = y;

#if DEBUG_METRICS
    xmin = PIXELS(glyph->xmin*scale);
    xmax = PIXELS(glyph->xmax*scale);
    ymin = PIXELS(glyph->ymin*scale);
    ymax = PIXELS(glyph->ymax*scale);

    advance_width = PIXELS(glyph->advance_width*scale);
    left_side_bearing = PIXELS(glyph->left_side_bearing*scale);

    render_line(&renderer, xmin+dx, dy-ymin, xmax+dx, dy-ymin, 0, 255, 0);
    render_line(&renderer, xmin+dx, dy-ymax, xmax+dx, dy-ymax, 0, 255, 0);
    render_line(&renderer, xmin+dx, dy-ymin, xmin+dx, dy-ymax, 0, 255, 0);
    render_line(&renderer, xmax+dx, dy-ymin, xmax+dx, dy-ymax, 0, 255, 0);

    render_line(&renderer, advance_width+dx, dy-ymin, advance_width+dx,
                dy-ymax, 255, 0, 0);

    render_line(&renderer, left_side_bearing+dx, dy-ymin, left_side_bearing+dx,
                dy-ymax, 0, 0, 255);
#endif

    for(i=0,n=0;i<point_num;i++){
        if(i == glyph->contour_ends[n]){
            render_line(&renderer, x+dx, dy-y, sx+dx, dy-sy, 255, 255, 255);
            if(i < point_num-1){
                x = PIXELS(glyph->points[i+1].x*scale);
                y = PIXELS(glyph->points[i+1].y*scale);
                sx = x;
                sy = y;
            }else{
                break;
            }
            n++;
        }else{
            render_line(&renderer, x+dx, dy-y,
                        PIXELS(glyph->points[i+1].x*scale)+dx,
                        dy-PIXELS(glyph->points[i+1].y*scale), 255, 255, 255);
            x = PIXELS(glyph->points[i+1].x*scale);
            y = PIXELS(glyph->points[i+1].y*scale);
        }
    }
    render_line(&renderer, x+dx, dy-y, sx+dx, dy-sy, 255, 255, 255);
    for(i=0;i<point_num;i++){
        render_set_pixel(&renderer, PIXELS(glyph->points[i].x*scale)+dx,
                         dy-PIXELS(glyph->points[i].y*scale),
                         glyph->points[i].on_curve ? 255 : 0,
                         glyph->points[i].on_curve ? 0 : 255, 0);
    }
}

void debug_render_str(MTFont *font, char *str, int dx, int dy, float scale) {
    MTGlyph *glyph;
    int x = dx, y = 0;

    size_t i;

    size_t c = 0;

    int byte_num = 1;

#if DEBUG_UTF8
    puts("=======================");
#endif

    for(i=0;i<strlen(str);i++){
#if DEBUG_UTF8
        printf("char: %x\n", (int)str[i]&0xFF);
        printf("byte num: %x\n", byte_num);
#endif
        if(byte_num > 1){
#if DEBUG_UTF8
            printf("before shift: %lx\n", c);
#endif
            c <<= 6;
#if DEBUG_UTF8
            printf("after shift: %lx\n", c);
            printf("new bits: %x\n", str[i]&0x3F);
#endif
            c |= str[i]&0x3F;
#if DEBUG_UTF8
            printf("after or: %lx\n", c);
#endif
            byte_num--;
            if(byte_num > 1) continue;
        }else{
            if((str[i]&0xE0) == 0xC0){
                c = str[i]&0x1F;
                byte_num = 2;
#if DEBUG_UTF8
                printf("after init: %lx\n", c);
#endif
                continue;
            }else if((str[i]&0xF0) == 0xE0){
                c = str[i]&0x0F;
                byte_num = 3;
#if DEBUG_UTF8
                printf("after init: %lx\n", c);
#endif
                continue;
            }else if((str[i]&0xF8) == 0xF0){
                c = str[i]&0x07;
                byte_num = 4;
#if DEBUG_UTF8
                printf("after init: %lx\n", c);
#endif
                continue;
            }else{
                c = str[i];
                byte_num = 1;
            }
        }
        if(c == '\n'){
            /* TODO */
        }
        if(c == ' '){
            /* Ugly workaround because somehow many fonts don't seem to include
             * an empty space character */
            x += PIXELS(400*scale);
            continue;
        }

#if DEBUG_UTF8
        printf("%lx, %c\n", c, (char)c);
#endif
        glyph = mt_font_get_glyph(font, c);
        debug_render_glyph(font, glyph, x, dy-y, scale);

        x += PIXELS(glyph->advance_width*scale);
    }
}

double scale = SCALE;

double x = (5-WIDTH/2)/SCALE;
double y = (120-HEIGHT/2)/SCALE;

void loop(int ms) {
#if VIEW_GLYPHS
    MTGlyph *glyph;
#endif

    float delta = 1/(float)ms;

    (void)ms;

    render_clear(&renderer, 1);

#if VIEW_GLYPHS

    if(!lock){
        if(render_keydown(&renderer, KEY_LEFT)){
            if(!selected) selected = 0xFFFF;
            selected--;
            printf("Selected \'%c\' (%04lx)\n", (char)selected&0x7F, selected);
        }

        if(render_keydown(&renderer, KEY_RIGHT)){
            selected++;
            printf("Selected \'%c\' (%04lx)\n", (char)selected&0x7F, selected);
        }
    }

    lock = render_keydown(&renderer, KEY_LEFT) |
           render_keydown(&renderer, KEY_RIGHT);

    glyph = mt_font_get_glyph(&font, selected);

    debug_render_glyph(&font, glyph, 120, 120, 0.08);

#else
    debug_render_str(&font, "The quick brown fox jumps over the lazy dog. "
                     "Victor jagt zw\303\266lf Boxk\303\244mpfer quer "
                     "\303\274ber den gro\303\337en Sylter Deich. Voix "
                     "ambigu\303\253 d\342\200\231un c\305\223ur qui, au "
                     "z\303\251phyr, pr\303\251f\303\250re les jattes de "
                     "kiwis. Chinese characters can also be displayed: "
                     "\344\275\240\345\245\275.",
                     x*scale+render_get_width(&renderer)/2,
                     y*scale+render_get_height(&renderer)/2, scale);
    if(render_keydown(&renderer, KEY_LSHIFT)){
        scale *= 1.01;
    }
    if(render_keydown(&renderer, KEY_LCTRL)){
        scale /= 1.01;
    }

    if(render_keydown(&renderer, KEY_UP)) y += 100*delta/scale;
    if(render_keydown(&renderer, KEY_DOWN)) y -= 100*delta/scale;
    if(render_keydown(&renderer, KEY_LEFT)) x += 100*delta/scale;
    if(render_keydown(&renderer, KEY_RIGHT)) x -= 100*delta/scale;
#endif

    render_show_fps(&renderer);
    render_update(&renderer);
}

int main(int argc, char **argv) {
    MTReader reader;

    if(argc < 2){
        fputs("USAGE: mibitype [FILE]\n", stderr);

        return EXIT_FAILURE;
    }

    if(mt_reader_init(&reader, argv[1])){
        fputs("mibitype: Failed to open file!\n", stderr);

        return EXIT_FAILURE;
    }

    if(mt_font_init(&font, &reader, 90)){
        fputs("mibitype: Unable to load the font!\n", stderr);

        return EXIT_FAILURE;
    }

    selected = ' ';
    printf("Selected \'%c\' (%04lx)\n", (char)selected&0x7F, selected);

    lock = 0;

    render_init(&renderer, WIDTH, HEIGHT, "MibiType");
    render_main_loop(&renderer, loop);

    mt_font_free(&font);
    mt_reader_free(&reader);

    return EXIT_SUCCESS;
}

