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
    unsigned long int tag;
    unsigned long int checksum;
    unsigned long int offset;
    unsigned long int size;
} MTTTFTableDir;

typedef struct {
    unsigned short int format;
    unsigned short int platform_id;
    unsigned long int group_num;
    size_t data_cur;
} MTTTFCmap;

typedef struct {
    unsigned short int table_num;

    unsigned short int glyph_num;
    unsigned short int simple_points_max;

    unsigned short int units_per_em;

    short int long_offsets;

    size_t best_map;
    MTTTFCmap cmap;

    size_t glyf_table_pos;
    size_t maxp_table_pos;
    size_t loca_table_pos;
    size_t cmap_table_pos;
    size_t htmx_table_pos;

    unsigned short int added_contours;

    unsigned short int advance_width_num;

    unsigned char *flags;

    MTTTFTableDir *table_dir;
} MTTTF;

int mt_ttf_is_valid(void *_data, MTReader *reader);

int mt_ttf_init(void *_data, void *_font);

size_t mt_ttf_get_glyph_id(void *_data, void *_font, size_t c);

int mt_ttf_load_glyph(void *_data, void *_font, void *_glyph, size_t id);

int mt_ttf_load_missing(void *_data, void *_font, void *_glyph);

int mt_ttf_size_to_pixels(void *_data, void *_font, int points, int size);

void mt_ttf_free(void *_data, void *_font);

#endif
