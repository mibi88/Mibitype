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

#include <mibitype/loaders/ttf.h>
#include <mibitype/errors.h>

#include <string.h>

#define MT_TTF_CHAR_TO_INT(a, b, c, d) ((a)|((b)<<8)|((c)<<16)|((d)<<24))
#define MT_TTF_STR_TO_INT(s) MT_TTF_CHAR_TO_INT((s)[0], (s)[1], (s)[2], (s)[3])

#define MT_TTF_REQUIRED_TABLES_NUM 9

#define MT_TTF_EXTEND_SIGN(n, b) ((n)&(1<<((b)-1)) ? -(((n)^0xFFFF)+1) : (n))

enum {
    MT_TTF_CMAP = MT_TTF_CHAR_TO_INT('c', 'm', 'a', 'p'),
    MT_TTF_GLYF = MT_TTF_CHAR_TO_INT('g', 'l', 'y', 'f'),
    MT_TTF_HEAD = MT_TTF_CHAR_TO_INT('h', 'e', 'a', 'd'),
    MT_TTF_HHEA = MT_TTF_CHAR_TO_INT('h', 'h', 'e', 'a'),
    MT_TTF_HMTX = MT_TTF_CHAR_TO_INT('h', 'm', 't', 'x'),
    MT_TTF_LOCA = MT_TTF_CHAR_TO_INT('l', 'o', 'c', 'a'),
    MT_TTF_MAXP = MT_TTF_CHAR_TO_INT('m', 'a', 'x', 'p'),
    MT_TTF_NAME = MT_TTF_CHAR_TO_INT('n', 'a', 'm', 'e'),
    MT_TTF_POST = MT_TTF_CHAR_TO_INT('p', 'o', 's', 't')
};

int _mt_ttf_load_dir(MTTTF *ttf, MTReader *reader, int is_check) {
    size_t i;

    const unsigned long int required_tables[MT_TTF_REQUIRED_TABLES_NUM] = {
        MT_TTF_CMAP,
        MT_TTF_GLYF,
        MT_TTF_HEAD,
        MT_TTF_HHEA,
        MT_TTF_HMTX,
        MT_TTF_LOCA,
        MT_TTF_MAXP,
        MT_TTF_NAME,
        MT_TTF_POST
    };

    unsigned char found[MT_TTF_REQUIRED_TABLES_NUM];

    size_t n;
    size_t table_num;

    long unsigned int tag;

    /* Load the font directory. */
    /* We'll load the offset subtable:
     * uint32 the scaler type (or the sfnt version in Microsofts opentype doc.)
     * uint16 the number of tables
     * uint16 the search range (IDK what it is for, I think it helps to read
     *                          the font faster).
     * uint16 the entry selector (IDK what it is for, I think it helps to read
     *                            the font faster).
     * uint16 the range shift (IDK what it is for, I think it helps to read
     *                         the font faster).
     */

    /* We'll just get the number of tables for now. */
    MT_READER_SKIP(reader, 4);
    table_num = mt_reader_read_short(reader);
    MT_READER_SKIP(reader, 3*2);

    if(!is_check) ttf->table_num = table_num;

    memset(found, 0, MT_TTF_REQUIRED_TABLES_NUM);

    /* Load the table directory, which is made up of the following values
     * uint32 the tag (we'll load it as an array of 4 bytes).
     * uint32 the checksum of the table.
     * uint32 the offset (where it is starting at sfnt).
     * uint32 the size of the table in bytes.
     */

    if(!is_check){
        ttf->table_dir = malloc(sizeof(MTTTFTableDir)*ttf->table_num);
        if(!ttf->table_dir) return MT_E_OUT_OF_MEM;
    }

    for(i=0;i<table_num;i++){
        tag = (unsigned long int)mt_reader_read_char(reader);
        tag |= (unsigned long int)mt_reader_read_char(reader)<<8;
        tag |= (unsigned long int)mt_reader_read_char(reader)<<16;
        tag |= (unsigned long int)mt_reader_read_char(reader)<<24;

        for(n=0;n<MT_TTF_REQUIRED_TABLES_NUM;n++){
            if(required_tables[n] == tag){
                found[n] = 1;
                break;
            }
        }

        if(is_check){
            MT_READER_SKIP(reader, 4*4);
        }else{
            ttf->table_dir[i].tag = tag;
            ttf->table_dir[i].checksum = mt_reader_read_int(reader);
            ttf->table_dir[i].offset = mt_reader_read_int(reader);
            ttf->table_dir[i].size = mt_reader_read_int(reader);

#if MT_DEBUG
            /* I may cause issues depending on the endianness */
            fputs("mibitype: Table name: ", stdout);
            fwrite(&ttf->table_dir[i].tag, 1, 4, stdout);
            printf(" at offset %08lx\n", ttf->table_dir[i].offset);
#endif
        }
    }

    for(i=0;i<MT_TTF_REQUIRED_TABLES_NUM;i++){
#if MT_DEBUG
        fputs("Table \"", stdout);
        fwrite(&required_tables[i], 1, 4, stdout);
        puts("\" not found!");
#endif
        if(!found[i]) return MT_E_CORRUPTED;
    }

    /* TODO: Check if the checksum is correct as described at
     * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.h
     * tml.
     */

    return MT_E_NONE;
}

int mt_ttf_is_valid(void *_data, MTReader *reader) {
    (void)_data;

    return _mt_ttf_load_dir(NULL, reader, 1) != MT_E_CORRUPTED;
}

int _mt_ttf_load_maxp(MTTTF *ttf, MTFont *font) {
    /* TODO: Doc. */

    MT_READER_JMP(font->reader, ttf->maxp_table_pos);

    /* Skip the version number for now. */
    if(mt_reader_read_int(font->reader) != 0x00010000) return MT_E_CORRUPTED;
    ttf->glyph_num = mt_reader_read_short(font->reader);
    ttf->simple_points_max = mt_reader_read_short(font->reader);

#if MT_DEBUG
    printf("mibitype: This font has %d glyphs\n", ttf->glyph_num);
#endif

    return MT_E_NONE;
}

int _mt_ttf_get_table_pos(MTTTF *ttf, unsigned long int table, size_t *pos) {
    char found = 0;

    size_t i;

    /* Find the table. */
    for(i=0;i<ttf->table_num;i++){
        if(ttf->table_dir[i].tag == table){
            *pos = ttf->table_dir[i].offset;
            found = 1;
            break;
        }
    }

    return !found;
}

int _mt_ttf_load_head(MTTTF *ttf, MTFont *font) {
    size_t offset;

    int rc;

    if((rc = _mt_ttf_get_table_pos(ttf, MT_TTF_HEAD, &offset))) return rc;

    MT_READER_JMP(font->reader, offset);
    MT_READER_SKIP(font->reader, 4*4+2);
    ttf->units_per_em = mt_reader_read_short(font->reader);
    MT_READER_SKIP(font->reader, 2*8);
    font->xmin = mt_reader_read_short(font->reader);
    font->ymin = mt_reader_read_short(font->reader);
    font->xmax = mt_reader_read_short(font->reader);
    font->ymax = mt_reader_read_short(font->reader);

    font->xmin = MT_TTF_EXTEND_SIGN(font->xmin, 16);
    font->ymin = MT_TTF_EXTEND_SIGN(font->ymin, 16);
    font->xmax = MT_TTF_EXTEND_SIGN(font->xmax, 16);
    font->ymax = MT_TTF_EXTEND_SIGN(font->ymax, 16);

#if MT_DEBUG
    printf("mibitype: Max glyph sizes: xmin: %d, ymin: %d, xmax: %d, "
           "ymax: %d\n", font->xmin, font->ymin, font->xmax, font->ymax);
#endif

    MT_READER_SKIP(font->reader, 3*2);
    ttf->long_offsets = mt_reader_read_short(font->reader);

#if MT_DEBUG
    printf("Long offsets: %d\n", ttf->long_offsets);
#endif

    return 0;
}

int _mt_ttf_load_cmap(MTTTF *ttf, MTFont *font) {
    size_t i;

    unsigned short int encoding_subtables;

    unsigned short int platform_id;
    unsigned short int platform_specific_id;
    unsigned short int format;

    unsigned long int length;
    unsigned long int group_num;
#if MT_DEBUG
    unsigned long int seg_count;
    unsigned long int start_char, end_char, start_index;
    size_t n;
#endif

    (void)length;

    MT_READER_JMP(font->reader, ttf->cmap_table_pos);

    /* Skip the version number (which is zero). */
    MT_READER_SKIP(font->reader, 2);
    encoding_subtables = mt_reader_read_short(font->reader);

    for(i=0;i<encoding_subtables;i++){
        MT_READER_JMP(font->reader, ttf->cmap_table_pos+4+i*8);

        platform_id = mt_reader_read_short(font->reader);
        ttf->cmap.platform_id = platform_id;

#if MT_DEBUG
        printf("mibitype: Platform ID: %d\n", platform_id);
#endif

        platform_specific_id = mt_reader_read_short(font->reader);
#if MT_DEBUG
        printf("mibitype: Platform specific ID: %d\n",
               platform_specific_id);
#endif

        if(!platform_id){
            /* It is an unicode encoding subtable. */
            /* Jump to the start of the mapping table */
            MT_READER_JMP(font->reader, ttf->cmap_table_pos+
                                        mt_reader_read_int(font->reader));

            if(platform_specific_id == 3 || platform_specific_id == 4){
                /* It is a unicode 2.0 full repertoire (IDK what it means)
                 * character table */
                format = mt_reader_read_short(font->reader);
                ttf->cmap.format = format;
#if MT_DEBUG
                printf("mibitype: Character map format: %d\n", format);
#endif
                if(format == 4){
                    /* It is a two byte encoding format. */
                    length = mt_reader_read_short(font->reader);

                    /* Skip the language code */
                    MT_READER_SKIP(font->reader, 2);

                    ttf->cmap.data_cur = font->reader->cur;

#if MT_DEBUG
                    /* Load the seg count */
                    seg_count = mt_reader_read_short(font->reader)/2;
                    printf("mibitype: Segment count: %lu\n", seg_count);
#endif

                    ttf->best_map = i;
                    break;
                }else if(format == 12){
                    /* Skip the reserved thing */
                    MT_READER_SKIP(font->reader, 2);
                    length = mt_reader_read_int(font->reader);

                    /* Skip the language code */
                    MT_READER_SKIP(font->reader, 4);
                    group_num = mt_reader_read_int(font->reader);
                    ttf->cmap.group_num = group_num;
                    ttf->cmap.data_cur = font->reader->cur;
                    ttf->best_map = i;

#if MT_DEBUG
                    printf("mibitype: Group num: %lu\n", group_num);
                    for(n=0;n<group_num;n++){
                        start_char = mt_reader_read_int(font->reader);
                        end_char = mt_reader_read_int(font->reader);
                        start_index = mt_reader_read_int(font->reader);
                        printf("mibitype: start char: %04lx\n"
                               "mibitype: end char: %04lx\n"
                               "mibitype: start index: %04lx\n", start_char,
                               end_char, start_index);
                    }

                    break;
#endif
                }
            }
        }else if(platform_id == 3){
            /* TODO: Implement this! */
        }
    }

    return MT_E_NONE;
}

int _mt_ttf_load_hhea(MTTTF *ttf, MTFont *font) {
    size_t offset;

    int rc;

    if((rc = _mt_ttf_get_table_pos(ttf, MT_TTF_HHEA, &offset))) return rc;

    MT_READER_JMP(font->reader, offset);

    MT_READER_SKIP(font->reader, 1*4);

    font->ascender = mt_reader_read_short(font->reader);
    font->descender = mt_reader_read_short(font->reader);
    font->line_gap = mt_reader_read_short(font->reader);

    font->ascender = MT_TTF_EXTEND_SIGN(font->ascender, 16);
    font->descender = MT_TTF_EXTEND_SIGN(font->descender, 16);
    font->line_gap = MT_TTF_EXTEND_SIGN(font->line_gap, 16);

    MT_READER_SKIP(font->reader, 12*2);

    ttf->advance_width_num = mt_reader_read_short(font->reader);
    if(!ttf->advance_width_num) return MT_E_CORRUPTED;

    return MT_E_NONE;
}

int mt_ttf_init(void *_data, void *_font) {
    int rc;

    MTTTF *ttf = _data;
    MTFont *font = _font;

    ttf->flags = NULL;
    ttf->table_dir = NULL;

    _mt_ttf_load_dir(ttf, font->reader, 0);

    if(_mt_ttf_get_table_pos(ttf, MT_TTF_GLYF, &ttf->glyf_table_pos)){
        return MT_E_CORRUPTED;
    }
    if(_mt_ttf_get_table_pos(ttf, MT_TTF_MAXP, &ttf->maxp_table_pos)){
        return MT_E_CORRUPTED;
    }
    if(_mt_ttf_get_table_pos(ttf, MT_TTF_LOCA, &ttf->loca_table_pos)){
        return MT_E_CORRUPTED;
    }
    if(_mt_ttf_get_table_pos(ttf, MT_TTF_CMAP, &ttf->cmap_table_pos)){
        return MT_E_CORRUPTED;
    }
    if(_mt_ttf_get_table_pos(ttf, MT_TTF_HMTX, &ttf->htmx_table_pos)){
        return MT_E_CORRUPTED;
    }

    if((rc = _mt_ttf_load_maxp(ttf, _font))) return rc;

    if((rc = _mt_ttf_load_head(ttf, _font))) return rc;

    if((rc = _mt_ttf_load_cmap(ttf, _font))) return rc;

    if((rc = _mt_ttf_load_hhea(ttf, _font))) return rc;

    ttf->flags = malloc(ttf->simple_points_max);
    if(ttf->flags == NULL) return MT_E_OUT_OF_MEM;

    return MT_E_NONE;
}

size_t mt_ttf_get_glyph_id(void *_data, void *_font, size_t c) {
    MTTTF *ttf = _data;
    MTFont *font = _font;

    size_t i;
    unsigned long int start_char, end_char, start_index;

    unsigned short int seg_count;

    size_t old_pos;

    size_t delta, offset;

    /* TODO: Make something clean. */

    MT_READER_JMP(font->reader, ttf->cmap.data_cur);

    if(ttf->cmap.platform_id == 0){
        if(ttf->cmap.format == 4){
#if MT_DEBUG
            puts("mibitype: Loading glyph id from cmap format 4!");
#endif
            /* Load the seg count */
            seg_count = mt_reader_read_short(font->reader)/2;

            /* Skip all the search related things */
            MT_READER_SKIP(font->reader, 2*3);

            /* Search the segment (endcodes are sorted, but I don't kniw if
             * they can contain multiple elements with the same value so I'm
             * not doing any binary search for now. */
            for(i=0;i<seg_count;i++){
                end_char = mt_reader_read_short(font->reader);
                old_pos = font->reader->cur;
                if(end_char >= c){
                    MT_READER_SKIP(font->reader, seg_count*2);
                    start_char = mt_reader_read_short(font->reader);
#if MT_DEBUG
                    printf("mibitype: start_char: %lx, end_char: %lx\n",
                           start_char, end_char);
#endif
                    if(start_char > c){
                        /* This segment doesn't contain this char, go back */
                        MT_READER_JMP(font->reader, old_pos);
                    }else{
                        /* The char is in this segment, get the index */
                        MT_READER_SKIP(font->reader, seg_count*2-2);
                        delta = mt_reader_read_short(font->reader);

                        MT_READER_SKIP(font->reader, seg_count*2-2);
                        offset = mt_reader_read_short(font->reader);

#if MT_DEBUG
                        printf("mibitype: Segment found: start_char: %lx, "
                               "end_char: %lx, delta: %lx, offset: %lx\n",
                               start_char, end_char, delta, offset);
#endif
                        if(offset){
                            MT_READER_SKIP(font->reader,
                                           offset+2*(c-start_char)-2);
                            return delta+mt_reader_read_short(font->reader);
                        }else{
                            return (delta+c)&0xFFFF;
                        }
                    }
                }
#if MT_DEBUG
                else{
                    printf("mibitype: end_char: %lx\n", end_char);
                }
#endif
            }
#if MT_DEBUG
            puts("mibitype: NO CORRESPONDING GLYPH ID FOUND!");
            return 0;
#endif
        }else if(ttf->cmap.format == 12){
            for(i=0;i<ttf->cmap.group_num;i++){
                start_char = mt_reader_read_int(font->reader);
                end_char = mt_reader_read_int(font->reader);
                start_index = mt_reader_read_int(font->reader);
                if(c >= start_char && c <= end_char){
#if MT_DEBUG
                    puts("mibitype: Using table 12!");
#endif
                    return c-start_char+start_index;
                }
            }
        }
    }

#if MT_DEBUG
    puts("mibitype: NO SUPPORTED CMAP TABLE FOUND!");
#endif
    return c;
}

void _mt_ttf_load_glyph_info(MTTTF *ttf, MTFont *font, MTGlyph *glyph,
                             size_t id, int load_sizes, int load_metrics) {
    /* Load the glyph description. It is made up of:
     * int16 the number of contours (useful to know if it is a simple glyph).
     * int16 the minimum X coordinate.
     * int16 the minimum Y coordinate.
     * int16 the maximum X coordinate.
     * int16 the maximum Y coordinate.
     */

    size_t old_pos;

    if(ttf->long_offsets){
        MT_READER_JMP(font->reader, ttf->loca_table_pos+id*4);
        MT_READER_JMP(font->reader, ttf->glyf_table_pos+
                                    mt_reader_read_int(font->reader));
    }else{
        MT_READER_JMP(font->reader, ttf->loca_table_pos+id*2);
        MT_READER_JMP(font->reader, ttf->glyf_table_pos+
                                    mt_reader_read_short(font->reader)*2);
    }

    ttf->added_contours = mt_reader_read_short(font->reader);
    if(load_sizes){
        glyph->xmin = mt_reader_read_short(font->reader);
        glyph->ymin = mt_reader_read_short(font->reader);
        glyph->xmax = mt_reader_read_short(font->reader);
        glyph->ymax = mt_reader_read_short(font->reader);

        glyph->xmin = MT_TTF_EXTEND_SIGN(glyph->xmin, 16);
        glyph->ymin = MT_TTF_EXTEND_SIGN(glyph->ymin, 16);
        glyph->xmax = MT_TTF_EXTEND_SIGN(glyph->xmax, 16);
        glyph->ymax = MT_TTF_EXTEND_SIGN(glyph->ymax, 16);
    }else{
        MT_READER_SKIP(font->reader, 4*2);
    }

    if(load_metrics){
        old_pos = font->reader->cur;
        if(id < ttf->advance_width_num){
            MT_READER_JMP(font->reader, ttf->htmx_table_pos+4*id);
            glyph->advance_width = mt_reader_read_short(font->reader);
        }else{
            MT_READER_JMP(font->reader, ttf->htmx_table_pos+4*
                          (ttf->advance_width_num-1));
            glyph->advance_width = mt_reader_read_short(font->reader);
            MT_READER_JMP(font->reader, ttf->htmx_table_pos+4*
                          ttf->advance_width_num+
                          (id-ttf->advance_width_num)*2);
        }
        glyph->left_side_bearing = mt_reader_read_short(font->reader);
        glyph->left_side_bearing = MT_TTF_EXTEND_SIGN(glyph->left_side_bearing,
                                                      16);
        MT_READER_JMP(font->reader, old_pos);
    }
}

int _mt_ttf_load_simple_glyph(MTTTF *ttf, MTFont *font, MTGlyph *glyph) {
    /* Simple glyphs are stored as following:
     * uint16 x contour_num is an array containing indices of the last
     *                      points of a contour.
     * uint16 the number of instruction (IDK what they are for).
     * uint16 x instruction_num the instruction (IDK what they are for).
     * uint16 x (depending on the flags) an array of flags.
     * uint8/uint16 x the number of points X coordinates of the points.
     * uint8/uint16 x the number of points Y coordinates of the points.
     * the coordinates are relative to the previous point or (0;0) for the
     * first point.
     */

    size_t i, n;

    const size_t new_contour_num = glyph->contour_num+ttf->added_contours;
    const size_t previous_point_num = glyph->contour_num ?
                                    glyph->contour_ends[glyph->contour_num-1]+
                                    1 : 0;

    unsigned short int point_num, instruction_num;

    unsigned char flag, count;
    short int value;

    int x, y;

    void *new;

    if(!new_contour_num) return MT_E_NONE;

    new = realloc(glyph->contour_ends, new_contour_num*sizeof(size_t));
    if(new == NULL) return MT_E_OUT_OF_MEM;
    glyph->contour_ends = new;

    for(i=glyph->contour_num;i<new_contour_num;i++){
        glyph->contour_ends[i] = mt_reader_read_short(font->reader);
    }

    /* Skip all the instruction stuff for now. */
    instruction_num = mt_reader_read_short(font->reader);
    MT_READER_SKIP(font->reader, instruction_num);

    /* Read the points and flags. */
    point_num = glyph->contour_ends[new_contour_num-1]+1;

#if MT_DEBUG
    printf("mibitype: Point num: %d\n", point_num);
    printf("mibitype: cur: %016lx\n", font->reader->cur);
#endif

    if(point_num > ttf->simple_points_max) return MT_E_CORRUPTED;

    for(i=0;i<point_num;i++){
        flag = mt_reader_read_char(font->reader);
        ttf->flags[i] = flag;

        if(flag&(1<<3)){
            count = mt_reader_read_char(font->reader);
            for(n=0;n<count;n++){
                if(i+1 >= point_num){
#if MT_DEBUG
                    puts("mibitype: Too many flags!");
#endif
                    break;
                }
                ttf->flags[++i] = flag;
            }
        }
    }

#if MT_DEBUG
    for(i=0;i<point_num;i++){
        printf("mibitype: cur: %016lx. Flag %ld: %02x\n", font->reader->cur,
               i+1, ttf->flags[i]);
    }
#endif

    new = realloc(glyph->points, (previous_point_num+point_num)*
                  sizeof(MTPoint));
    if(new == NULL) return MT_E_OUT_OF_MEM;
    glyph->points = new;

    /* Load the X coordinates and set if the point is on the curve. */
    x = 0;
    y = 0;

    for(n=previous_point_num,i=0;i<point_num;n++,i++){
        glyph->points[n].on_curve = ttf->flags[i]&1;
        if(ttf->flags[i]&(1<<1)){
            /* The X coordinate is a single byte long */
            value = mt_reader_read_char(font->reader);
            if(!(ttf->flags[i]&(1<<4))) value = -value;
#if MT_DEBUG
            printf("mibitype: cur: %016lx. X (1 byte) coordinate offset: %d\n",
                   font->reader->cur, value);
#endif
            x += value;
        }else if(!(ttf->flags[i]&(1<<4))){
            /* The X coordinate is two bytes long */
            value = mt_reader_read_short(font->reader);
#if MT_DEBUG
            printf("mibitype: cur: %016lx. X (2 bytes) coordinate offset: "
                   "%d\n", font->reader->cur, value);
#endif
            x += value;
        }
#if MT_DEBUG
        else{
            printf("mibitype: cur: %016lx. Repeated X coordinate.\n",
                   font->reader->cur);
        }
#endif
        glyph->points[n].x = x;
    }

    /* Load the Y coordinates. */
    for(n=previous_point_num,i=0;i<point_num;n++,i++){
        if(ttf->flags[i]&(1<<2)){
            /* The Y coordinate is a single byte long */
            value = mt_reader_read_char(font->reader);
            if(!(ttf->flags[i]&(1<<5))) value = -value;
#if MT_DEBUG
            printf("mibitype: cur: %016lx. Y (1 byte) coordinate offset: %d\n",
                   font->reader->cur, value);
#endif
            y += value;
        }else if(!(ttf->flags[i]&(1<<5))){
            /* The Y coordinate is two bytes long */
            value = mt_reader_read_short(font->reader);
#if MT_DEBUG
            printf("mibitype: cur: %016lx. Y (2 bytes) coordinate offset: "
                   "%d\n", font->reader->cur, value);
#endif
            y += value;
        }
#if MT_DEBUG
        else{
            printf("mibitype: cur: %016lx. Repeated Y coordinate.\n",
                   font->reader->cur);
        }
#endif
        glyph->points[n].y = y;
    }

#if MT_DEBUG
    puts("mibitype: Glyph loaded. Coordinates:");
    for(n=previous_point_num,i=0;i<point_num;n++,i++){
        printf("mibitype: Point %ld: (%d;%d)\n", i+1, glyph->points[n].x,
               glyph->points[n].y);
    }
    puts("mibitype: ==========================");
#endif

    for(i=glyph->contour_num;i<new_contour_num;i++){
        glyph->contour_ends[i] += previous_point_num;
    }

    glyph->contour_num = new_contour_num;

    return MT_E_NONE;
}

int _mt_ttf_load_compound_glyph(MTTTF *ttf, MTFont *font, MTGlyph *glyph) {
    unsigned short int flags;
    unsigned short int index;

    unsigned short int num1, num2;
    short int x, y;

    unsigned short int xscale, yscale, scale01, scale10;

    size_t old_pos;

    size_t i;

    size_t point_num;
    size_t old_point_num;

    (void)xscale;
    (void)yscale;
    (void)scale01;
    (void)scale10;

    /* It is a compound glyph. */
#if MT_DEBUG
    puts("mibitype: Compound glyph found!");
#endif

    /* Read all the component glyph informations */
    do{
        flags = mt_reader_read_short(font->reader);

        index = mt_reader_read_short(font->reader);

        /* The following items depend on the flags */
        if(flags&(1<<1)){
            /* The following values are x/y coordinates */

            if(flags&1){
                /* The following values are words */
                x = mt_reader_read_short(font->reader);
                y = mt_reader_read_short(font->reader);
            }else{
                /* The following values are bytes */
                x = mt_reader_read_char(font->reader);
                y = mt_reader_read_char(font->reader);
                if(x&(1<<7)) x |= 0xFF00;
                if(y&(1<<7)) y |= 0xFF00;
#if MT_DEBUG
                printf("mibitype: Offsets: %d, %d\n", x, y);
#endif
            }
        }else{
            /* The following values are point numbers */

            if(flags&1){
                /* The following values are words */
                num1 = mt_reader_read_short(font->reader);
                num2 = mt_reader_read_short(font->reader);
            }else{
                /* The following values are bytes */
                num1 = mt_reader_read_char(font->reader);
                num2 = mt_reader_read_char(font->reader);
            }
        }

#if MT_DEBUG
        printf("mibitype: Component glyph: index: %04x, %s as %s\n", index,
               flags&(1<<1) ? "point numbers" : "coordinates",
               flags&1 ? "words" : "bytes");
#endif

        old_pos = font->reader->cur;
        old_point_num = glyph->contour_num ?
                        glyph->contour_ends[glyph->contour_num-1]+1 : 0;

        _mt_ttf_load_glyph_info(ttf, font, glyph, index, 0, flags&(1<<9));

        if(!(ttf->added_contours&(1<<15))){
            if(_mt_ttf_load_simple_glyph(ttf, font, glyph)){
                return MT_E_CORRUPTED;
            }
        }else{
            return MT_E_NONE;
        }

        point_num = glyph->contour_num ?
                    glyph->contour_ends[glyph->contour_num-1]+1 : 0;
#if MT_DEBUG
        puts("mibitype: Component loaded!");
#endif

        MT_READER_JMP(font->reader, old_pos);

        /* Move the glyph around as needed */

        if(!(ttf->added_contours&(1<<15))){
            if(!(flags&(1<<1))){
                x = 0;
                y = 0;

                if(num2+old_point_num < point_num &&
                   (size_t)num1 < point_num){
                    x = glyph->points[num1].x-glyph->points[num2+point_num].x;
                    y = glyph->points[num1].y-glyph->points[num2+point_num].y;
                }
            }

            for(i=old_point_num;i<point_num;i++){
                /* TODO: Apply the transformations */

                /* Move the points around */
                glyph->points[i].x += x;
                glyph->points[i].y += y;
            }
        }

        if(flags&(1<<3)){
            /* The component glyph has a scale */
            xscale = mt_reader_read_short(font->reader);
        }else if(flags&(1<<6)){
            /* The component glyph has a different scale on the X and Y axis */
            xscale = mt_reader_read_short(font->reader);
            yscale = mt_reader_read_short(font->reader);
        }else if(flags&(1<<7)){
            /* The component glyph has a 2x2 transformation */
            xscale = mt_reader_read_short(font->reader);
            scale01 = mt_reader_read_short(font->reader);
            scale10 = mt_reader_read_short(font->reader);
            yscale = mt_reader_read_short(font->reader);
        }

        /* If the 5th flag bit is set, another component glyph follows */
#if MT_DEBUG
        if(flags&(1<<5)) puts("mibitype: Continue");
#endif
    }while(flags&(1<<5));

#if MT_DEBUG
    puts("mibitype: ===========");
#endif

    return MT_E_NONE;
}

int mt_ttf_load_glyph(void *_data, void *_font, void *_glyph, size_t id) {
    MTTTF *ttf = _data;

    _mt_ttf_load_glyph_info(ttf, _font, _glyph, id, 1, 1);

    mt_glyph_init(_glyph);

    if(!(ttf->added_contours&(1<<15))){
        /* It is a simple glyph */
        return _mt_ttf_load_simple_glyph(ttf, _font, _glyph);
    }else{
        /* It is a compound glyph */
        return _mt_ttf_load_compound_glyph(ttf, _font, _glyph);
    }

    return MT_E_NONE;
}

int mt_ttf_load_missing(void *_data, void *_font, void *_glyph) {
    return mt_ttf_load_glyph(_data, _font, _glyph, 0);
}

int mt_ttf_size_to_pixels(void *_data, void *_font, int points, int size) {
    MTFont *font = _font;
    MTTTF *ttf = _data;
    return size*(points*font->dpi)/(72*ttf->units_per_em);
}

void mt_ttf_free(void *_data, void *_font) {
    MTTTF *ttf = _data;

    (void)_font;

    free(ttf->flags);
    ttf->flags = NULL;

    free(ttf->table_dir);
    ttf->table_dir = NULL;
}
