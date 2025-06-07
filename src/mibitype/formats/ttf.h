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

#ifndef MT_TTF_H
#define MT_TTF_H

#include <mibitype/defs.h>
#include <mibitype/font.h>

typedef struct {
    uint8_t tag[4];
    uint32_t checksum;
    uint32_t offset;
    uint32_t size;
} MTTTFTableDir;

typedef int32_t coord_t;

typedef struct {
    coord_t x, y;
    uint8_t on_curve;
} MTTTFPoint;

typedef struct {
    int16_t contour_num;
    uint16_t *contour_ends;
    int16_t xmin;
    int16_t ymin;
    int16_t xmax;
    int16_t ymax;
    MTTTFPoint *points;
} MTTTFGlyph;

typedef struct {
    uint16_t format;
    uint16_t platform_id;
    uint32_t group_num;
    size_t data_cur;
} MTTTFCmap;

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t cur;
    uint16_t table_num;
    uint16_t glyph_num;
    int16_t long_offsets;
    uint16_t encoding_subtables;
    size_t best_map;
    Font font;
    MTTTFTableDir *table_dir;
    MTTTFGlyph *glyphs;
    MTTTFCmap *cmaps;
} MTTTF;

void mt_ttf_init(MTTTF *ttf, uint8_t *buffer, size_t size);

uint8_t mt_ttf_read_char(MTTTF *ttf);
uint16_t mt_ttf_read_short(MTTTF *ttf);
uint32_t mt_ttf_read_int(MTTTF *ttf);
void mt_ttf_read_array(MTTTF *ttf, uint8_t *array, size_t bytes);

void mt_ttf_skip(MTTTF *ttf, size_t bytes);

int mt_ttf_load(MTTTF *ttf);
int mt_ttf_load_glyphs(MTTTF *ttf);
int mt_ttf_load_dir(MTTTF *ttf);
int mt_ttf_load_profile(MTTTF *ttf);
int mt_ttf_load_header(MTTTF *ttf);
int mt_ttf_load_cmap(MTTTF *ttf);
int mt_ttf_load_glyph(MTTTF *ttf, MTTTFGlyph *glyph);
int mt_ttf_load_simple_glyph(MTTTF *ttf, MTTTFGlyph *glyph);
int mt_ttf_load_compound_glyph(MTTTF *ttf, MTTTFGlyph *glyph);

size_t mt_ttf_get_index(MTTTF *ttf, uint32_t c);

void mt_ttf_free(MTTTF *ttf);

#endif
