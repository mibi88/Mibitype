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

#include <render.h>

#include <mibitype/formats/ttf.h>

Renderer renderer;

MTTTF ttf;

size_t selected;
char lock;

#define RENDER_FROM_CHAR 1

void debug_render_glyph(MTTTFGlyph *glyph, int dx, int dy, float scale){
    size_t point_num;
    coord_t x, y, sx, sy;
    size_t i, n;
    if(!glyph->contour_ends) return;
    point_num = glyph->contour_ends[glyph->contour_num-1];
    if(!point_num) return;
    x = glyph->points->x*scale;
    y = glyph->points->y*scale;
    sx = x;
    sy = y;
    for(i=0,n=0;i<point_num;i++){
        if(i == glyph->contour_ends[n]){
            render_line(&renderer, x+dx, dy-y, sx+dx, dy-sy, 255, 255, 255);
            if(i < point_num-1){
                x = glyph->points[i+1].x*scale;
                y = glyph->points[i+1].y*scale;
                sx = x;
                sy = y;
            }else{
                break;
            }
            n++;
        }else{
            render_line(&renderer, x+dx, dy-y, glyph->points[i+1].x*scale+dx,
                        dy-glyph->points[i+1].y*scale, 255, 255, 255);
            x = glyph->points[i+1].x*scale;
            y = glyph->points[i+1].y*scale;
        }
    }
    render_line(&renderer, x+dx, dy-y, sx+dx, dy-sy, 255, 255, 255);
    for(i=0;i<point_num;i++){
        render_set_pixel(&renderer, glyph->points[i].x*scale+dx,
                         dy-glyph->points[i].y*scale, 255, 0, 0);
    }
}

void loop(int ms) {
    size_t pos;
    if(!lock){
        if(render_keydown(&renderer, KEY_LEFT)){
#if RENDER_FROM_CHAR
            if(!selected) selected = 0xFFFF;
            selected--;
#else
            if(!selected) selected = ttf.glyph_num;
            selected--;
#endif
            printf("Selected \'%c\' (%ld), glyph index: %ld\n", (char)selected,
                   selected, mt_ttf_get_index(&ttf, selected));
        }
        if(render_keydown(&renderer, KEY_RIGHT)){
#if RENDER_FROM_CHAR
            selected++;
#else
            if(selected >= (size_t)(ttf.glyph_num-1)) selected = 0;
            else selected++;
#endif
            printf("Selected \'%c\' (%ld), glyph index: %ld\n", (char)selected,
                   selected, mt_ttf_get_index(&ttf, selected));
        }
    }
    lock = render_keydown(&renderer, KEY_LEFT) |
           render_keydown(&renderer, KEY_RIGHT);
    render_clear(&renderer, 1);
#if RENDER_FROM_CHAR
    pos = mt_ttf_get_index(&ttf, selected);
    if(pos < (size_t)(ttf.glyph_num)){
        debug_render_glyph(ttf.glyphs+pos, 120, 120, 0.08);
    }else{
        debug_render_glyph(ttf.glyphs, 120, 120, 0.08);
    }
#else
    debug_render_glyph(ttf.glyphs+selected, 120, 120, 0.08);
#endif
    render_show_fps(&renderer);
    render_update(&renderer);
}

int main(int argc, char **argv) {
    uint8_t *buffer;
    size_t size;
    FILE *fp;
    if(argc < 2){
        fputs("USAGE: mibitype [FILE]\n", stderr);
        return EXIT_FAILURE;
    }

    fp = fopen(argv[1], "r");
    if(!fp){
        fputs("mibitype: Unable to load the file!\n", stderr);
        return EXIT_FAILURE;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    buffer = malloc(size);
    if(!buffer){
        fputs("mibitype: Unable to load the font in RAM!\n", stderr);
        fclose(fp);
        return EXIT_FAILURE;
    }
    fread(buffer, 1, size, fp);
    fclose(fp);

    printf("Size (in bytes): %ld\n", size);

    mt_ttf_init(&ttf, buffer, size);
    if(mt_ttf_load(&ttf)){
        fputs("mibitype: Unable to load the font!\n", stderr);
        free(buffer);
        return EXIT_FAILURE;
    }

    printf("Number of tables: %d\n", ttf.table_num);

#if RENDER_FROM_CHAR
    selected = ' ';
    printf("Selected \'%c\' (%04lx), glyph index: %ld\n", (char)selected,
           selected, mt_ttf_get_index(&ttf, selected));
#else
    selected = 0;
#endif
    lock = 0;

    render_init(&renderer, 320, 240, "MibiType");
    render_main_loop(&renderer, loop);

    mt_ttf_free(&ttf);

    free(buffer);
    return EXIT_SUCCESS;
}

