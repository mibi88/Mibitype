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

#include <mibitype/formats/ttf.h>

void mt_ttf_init(MTTTF *ttf, uint8_t *buffer, size_t size) {
    ttf->buffer = buffer;
    ttf->size = size;
    ttf->cur = 0;
    ttf->table_dir = NULL;
}

uint8_t mt_ttf_read_char(MTTTF *ttf) {
    if(ttf->cur+1 >= ttf->size) return 0;
    return ttf->buffer[ttf->cur++];
}

uint16_t mt_ttf_read_short(MTTTF *ttf) {
    uint8_t byte1, byte2;
    if(ttf->cur+2 >= ttf->size) return 0;
    byte1 = ttf->buffer[ttf->cur++];
    byte2 = ttf->buffer[ttf->cur++];
    return (byte1<<8) | byte2;
}

uint32_t mt_ttf_read_int(MTTTF *ttf) {
    uint8_t byte1, byte2, byte3, byte4;
    if(ttf->cur+4 >= ttf->size) return 0;
    byte1 = ttf->buffer[ttf->cur++];
    byte2 = ttf->buffer[ttf->cur++];
    byte3 = ttf->buffer[ttf->cur++];
    byte4 = ttf->buffer[ttf->cur++];
    return (byte1<<24) | (byte2<<16) | (byte3<<8) | byte4;
}

void mt_ttf_read_array(MTTTF *ttf, uint8_t *array, size_t bytes) {
    size_t i;
    if(ttf->cur+4 >= ttf->size){
        for(i=0;i<bytes;i++){
            array[i] = 0;
        }
        return;
    }
    for(i=0;i<bytes;i++){
        array[i] = ttf->buffer[ttf->cur++];
    }
}

void mt_ttf_skip(MTTTF *ttf, size_t bytes) {
    ttf->cur += bytes;
}

char mt_ttf_load(MTTTF *ttf) {
    /* Load the font. */
    if(mt_ttf_load_dir(ttf)) return 1;
    if(mt_ttf_load_profile(ttf)) return 1;
    if(mt_ttf_load_header(ttf)) return 1;
    if(mt_ttf_load_glyphs(ttf)) return 1;
    if(mt_ttf_load_cmap(ttf)) return 1;
    return 0;
}

char mt_ttf_load_glyphs(MTTTF *ttf) {
    size_t i;
    char found = 0;
    size_t glyf_table_pos;
    size_t loca_table_pos;
    ttf->glyphs = malloc(ttf->glyph_num*sizeof(MTTTFGlyph));
    if(ttf->glyphs == NULL){
        free(ttf->table_dir);
        ttf->table_dir = NULL;
        return 1;
    }
    /* Find the glyf table. */
    for(i=0;i<ttf->table_num;i++){
        if(*(uint32_t*)ttf->table_dir[i].tag == *(uint32_t*)"glyf"){
            found = 1;
            break;
        }
    }
    if(!found) return 1;
    glyf_table_pos = ttf->table_dir[i].offset;
#if MT_DEBUG
    printf("\"glyf\" table offset: %08lx\n", glyf_table_pos);
#endif
    /* Find the loca table. */
    for(i=0;i<ttf->table_num;i++){
        if(*(uint32_t*)ttf->table_dir[i].tag == *(uint32_t*)"loca"){
            found = 1;
            break;
        }
    }
    if(!found) return 1;
    loca_table_pos = ttf->table_dir[i].offset;
#if MT_DEBUG
    printf("\"loca\" table offset: %08lx\n", loca_table_pos);
#endif
    for(i=0;i<ttf->glyph_num;i++){
#if MT_DEBUG
        printf("mibitype: Loading glyph %04lx\n", i);
#endif
        if(ttf->long_offsets){
            ttf->cur = loca_table_pos+i*4;
            ttf->cur = glyf_table_pos+mt_ttf_read_int(ttf);
        }else{
            ttf->cur = loca_table_pos+i*2;
            ttf->cur = glyf_table_pos+mt_ttf_read_short(ttf)*2;
        }
        ttf->glyphs[i].contour_ends = NULL;
        if(mt_ttf_load_glyph(ttf, ttf->glyphs+i)){
            free(ttf->table_dir);
            /* TODO: Free the other glyphs. */
            ttf->table_dir = NULL;
            return 1;
        }
    }
    return 0;
}

char mt_ttf_load_glyph(MTTTF *ttf, MTTTFGlyph *glyph) {
    size_t i;
    char found;
    /* Load the glyph description. It is made up of:
     * int16 the number of contours (useful to know if it is a simple glyph).
     * int16 the minimum X coordinate.
     * int16 the minimum Y coordinate.
     * int16 the maximum X coordinate.
     * int16 the maximum Y coordinate.
     */
    glyph->contour_num = mt_ttf_read_short(ttf);
    glyph->xmin = mt_ttf_read_short(ttf);
    glyph->ymin = mt_ttf_read_short(ttf);
    glyph->xmax = mt_ttf_read_short(ttf);
    glyph->ymax = mt_ttf_read_short(ttf);
    if(glyph->contour_num >= 0){
        if(mt_ttf_load_simple_glyph(ttf, glyph)){
            return 1;
        }
    }else{
        /* It is a compound glyph. */
#if MT_DEBUG
        puts("mibitype: Compound glyph found!");
#endif
        /* Load the first glyph because compound glyphs are currently
         * unsupported. */
        /* Find the glyf table. */
        for(i=0;i<ttf->table_num;i++){
            if(*(uint32_t*)ttf->table_dir[i].tag == *(uint32_t*)"glyf"){
                found = 1;
                break;
            }
        }
        if(!found) return 1;
        ttf->cur = ttf->table_dir[i].offset;
        mt_ttf_load_glyph(ttf, glyph);
    }
    return 0;
}

char mt_ttf_load_simple_glyph(MTTTF *ttf, MTTTFGlyph *glyph) {
    size_t i, n;
    coord_t x, y;
    uint16_t point_num;
    uint16_t instruction_num;
    uint8_t flag;
    uint8_t count;
    uint8_t *flags;
    int16_t value;
    uint16_t unsigned_value;
    /* Simple glyphs are stored as following:
     * uint16 x contour_num is an array containing indices of the last
     *                      points of a contour.
     * uint16 the number of instruction (IDK what they are useful to).
     * uint16 x instruction_num the instruction (IDK what they are useful to).
     * uint16 x (depending on the flags) an array of flags.
     * uint8/uint16 x the number of points X coordinates of the points.
     * uint8/uint16 x the number of points Y coordinates of the points.
     * the coordinates are relative to the previous point or (0;0) for the
     * first point.
     */
#if MT_DEBUG
    puts("mibitype: Simple glyph found!");
    printf("mibitype: Contour num: %d\n", glyph->contour_num);
#endif
    glyph->contour_ends = malloc(glyph->contour_num*sizeof(uint16_t));
    if(!glyph->contour_ends) return 2;
    for(i=0;i<(size_t)glyph->contour_num;i++){
        glyph->contour_ends[i] = mt_ttf_read_short(ttf);
    }
    /* Skip all the instruction stuff for now. */
    instruction_num = mt_ttf_read_short(ttf);
    mt_ttf_skip(ttf, instruction_num);
    /* Read the points and flags. */
    point_num = glyph->contour_ends[glyph->contour_num-1]+1;
#if MT_DEBUG
    printf("mibitype: Point num: %d\n", point_num);
    printf("mibitype: cur: %016lx\n", ttf->cur);
#endif
    flags = malloc(point_num*sizeof(uint8_t));
    if(!flags) return 3;
    for(i=0;i<point_num;i++){
        flag = mt_ttf_read_char(ttf);
        flags[i] = flag;
        if(flag&(1<<3)){
            count = mt_ttf_read_char(ttf);
            for(n=0;n<count;n++){
                if(i+1 >= point_num){
#if MT_DEBUG
                    puts("mibitype: Too many flags!");
#endif
                    break;
                }
                flags[++i] = flag;
            }
        }
    }
#if MT_DEBUG
    for(i=0;i<point_num;i++){
        printf("mibitype: cur: %016lx. Flag %ld: %02x\n", ttf->cur, i+1,
               flags[i]);
    }
#endif
    glyph->points = malloc(point_num*sizeof(MTTTFPoint));
    /* Load the X coordinates and set if the point is on the curve. */
    x = 0;
    y = 0;
    for(i=0;i<point_num;i++){
        glyph->points[i].on_curve = flags[i]&1;
        if(flags[i]&(1<<1)){
            /* The X coordinate is a single byte long */
            value = mt_ttf_read_char(ttf);
            if(!(flags[i]&(1<<4))) value = -value;
#if MT_DEBUG
            printf("mibitype: cur: %016lx. X (1 byte) coordinate offset: %d\n",
                   ttf->cur, value);
#endif
            x += value;
        }else if(!(flags[i]&(1<<4))){
            /* The X coordinate is two bytes long */
            value = mt_ttf_read_short(ttf);
#if MT_DEBUG
            printf("mibitype: cur: %016lx. X (2 bytes) coordinate offset: "
                   "%d\n", ttf->cur, value);
#endif
            x += value;
        }
#if MT_DEBUG
        else{
            printf("mibitype: cur: %016lx. Repeated X coordinate.\n",
                   ttf->cur);
        }
#endif
        glyph->points[i].x = x;
    }
    /* Load the Y coordinates. */
    for(i=0;i<point_num;i++){
        if(flags[i]&(1<<2)){
            /* The Y coordinate is a single byte long */
            value = mt_ttf_read_char(ttf);
            if(!(flags[i]&(1<<5))) value = -value;
#if MT_DEBUG
            printf("mibitype: cur: %016lx. Y (1 byte) coordinate offset: %d\n",
                   ttf->cur, value);
#endif
            y += value;
        }else if(!(flags[i]&(1<<5))){
            /* The Y coordinate is two bytes long */
            value = mt_ttf_read_short(ttf);
#if MT_DEBUG
            printf("mibitype: cur: %016lx. Y (2 bytes) coordinate offset: "
                   "%d\n", ttf->cur, value);
#endif
            y += value;
        }
#if MT_DEBUG
        else{
            printf("mibitype: cur: %016lx. Repeated Y coordinate.\n",
                   ttf->cur);
        }
#endif
        glyph->points[i].y = y;
    }
    free(flags);
    flags = NULL;
#if MT_DEBUG
    puts("mibitype: Glyph loaded. Coordinates:");
    for(i=0;i<point_num;i++){
        printf("mibitype: Point %ld: (%d;%d)\n", i+1, glyph->points[i].x,
               glyph->points[i].y);
    }
    puts("mibitype: ==========================");
#endif
    return 0;
}

char mt_ttf_load_compound_glyph(MTTTF *ttf, MTTTFGlyph *glyph) {
    /**/
    return 0;
}

char mt_ttf_load_dir(MTTTF *ttf) {
    size_t i;
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
    mt_ttf_skip(ttf, 4);
    ttf->table_num = mt_ttf_read_short(ttf);
    mt_ttf_skip(ttf, 2);
    mt_ttf_skip(ttf, 2);
    mt_ttf_skip(ttf, 2);
    /* Load the table directory, which is made up of the following values
     * uint32 the tag (we'll load it as an array of 4 bytes).
     * uint32 the checksum of the table.
     * uint32 the offset (where it is starting at sfnt).
     * uint32 the size of the table in bytes.
     */
    ttf->table_dir = malloc(sizeof(MTTTFTableDir)*ttf->table_num);
    if(!ttf->table_dir) return 1;
    for(i=0;i<ttf->table_num;i++){
        mt_ttf_read_array(ttf, ttf->table_dir[i].tag, 4);
        ttf->table_dir[i].checksum = mt_ttf_read_int(ttf);
        ttf->table_dir[i].offset = mt_ttf_read_int(ttf);
        ttf->table_dir[i].size = mt_ttf_read_int(ttf);
#if MT_DEBUG
        fputs("mibitype: Table name: ", stdout);
        fwrite(ttf->table_dir[i].tag, 1, 4, stdout);
        printf(" at offset %08x\n", ttf->table_dir[i].offset);
#endif
    }
    /* TODO: Check if the checksum is correct as described at
     * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.h
     * tml.
     */
    return 0;
}

char mt_ttf_load_profile(MTTTF *ttf) {
    /* TODO: Doc. */
    size_t i;
    char found = 0;
    /* Find the maxp table. */
    for(i=0;i<ttf->table_num;i++){
        if(*(uint32_t*)ttf->table_dir[i].tag == *(uint32_t*)"maxp"){
            found = 1;
            break;
        }
    }
    if(!found) return 1;
    ttf->cur = ttf->table_dir[i].offset;
#if MT_DEBUG
    printf("mibitype: \"maxp\" table offset: %08lx\n", ttf->cur);
#endif
    /* Skip the version number for now. */
    mt_ttf_skip(ttf, 4);
    ttf->glyph_num = mt_ttf_read_short(ttf);
#if MT_DEBUG
    printf("mibitype: This font has %d glyphs\n", ttf->glyph_num);
#endif
    return 0;
}

char mt_ttf_load_header(MTTTF *ttf) {
    size_t i;
    char found;
    /* Find the head table. */
    for(i=0;i<ttf->table_num;i++){
        if(*(uint32_t*)ttf->table_dir[i].tag == *(uint32_t*)"head"){
            found = 1;
            break;
        }
    }
    if(!found) return 1;
    ttf->cur = ttf->table_dir[i].offset;
    mt_ttf_skip(ttf, 2*2+4*3+2*2+8*2+2*7);
    ttf->long_offsets = mt_ttf_read_short(ttf);
#if MT_DEBUG
    printf("Long offsets: %d\n", ttf->long_offsets);
#endif
    return 0;
}

char mt_ttf_load_cmap(MTTTF *ttf) {
    size_t i;
    size_t cmap_start;
    char found;
    uint16_t platform_id;
    uint16_t platform_specific_id;
    uint16_t format;
    uint32_t length;
    uint32_t group_num;
#if MT_DEBUG
    uint32_t start_char, end_char, start_index;
    size_t n;
#endif
    /* Find the cmap table. */
    for(i=0;i<ttf->table_num;i++){
        if(*(uint32_t*)ttf->table_dir[i].tag == *(uint32_t*)"cmap"){
            found = 1;
            break;
        }
    }
    if(!found) return 1;
    cmap_start = ttf->table_dir[i].offset;
    ttf->cur = cmap_start;
    /* Skip the version number (which is zero). */
    mt_ttf_skip(ttf, 2);
    ttf->encoding_subtables = mt_ttf_read_short(ttf);
    ttf->cmaps = malloc(ttf->encoding_subtables*sizeof(MTTTFCmap));
    for(i=0;i<ttf->encoding_subtables;i++){
        ttf->cur = cmap_start+4+i*8;
        platform_id = mt_ttf_read_short(ttf);
        ttf->cmaps[i].platform_id = platform_id;
#if MT_DEBUG
        printf("mibitype: Platform ID: %d\n", platform_id);
#endif
        if(!platform_id){
            /* It is an unicode encoding subtable. */
            platform_specific_id = mt_ttf_read_short(ttf);
#if MT_DEBUG
            printf("mibitype: Platform specific ID: %d\n",
                   platform_specific_id);
#endif
            /* Jump to the start of the mapping table */
            ttf->cur = cmap_start+mt_ttf_read_int(ttf);
            if(platform_specific_id == 3 || platform_specific_id == 4){
                /* It is a unicode 2.0 full repertoire (IDK what it means)
                 * character table */
                format = mt_ttf_read_short(ttf);
                ttf->cmaps[i].format = format;
#if MT_DEBUG
                printf("mibitype: Character map format: %d\n", format);
#endif
                if(format == 4){
                    /* It is a two byte encoding format. */
                    length = mt_ttf_read_short(ttf);
                    /* Skip the language code */
                    mt_ttf_skip(ttf, 2);
                    /* TODO: Implement this awful thing! */
                }else if(format == 12){
                    /* It is a very weird format. */
                    /* Skip the reserved thing */
                    mt_ttf_skip(ttf, 2);
                    length = mt_ttf_read_int(ttf);
                    /* Skip the language code */
                    mt_ttf_skip(ttf, 4);
                    group_num = mt_ttf_read_int(ttf);
                    ttf->cmaps[i].group_num = group_num;
                    ttf->cmaps[i].data_cur = ttf->cur;
                    ttf->best_map = i;
#if MT_DEBUG
                    printf("mibitype: Group num: %d\n", group_num);
                    for(n=0;n<group_num;n++){
                        start_char = mt_ttf_read_int(ttf);
                        end_char = mt_ttf_read_int(ttf);
                        start_index = mt_ttf_read_int(ttf);
                        printf("mibitype: start char: %04x\n"
                               "mibitype: end char: %04x\n"
                               "mibitype: start index: %04x\n", start_char,
                               end_char, start_index);
                    }
#endif
                }
            }
        }else if(platform_id == 3){
            /* ISO platform ID */
            platform_specific_id = mt_ttf_read_short(ttf);
#if MT_DEBUG
            printf("mibitype: Platform specific ID: %d\n",
                   platform_specific_id);
#endif
            if(platform_specific_id == 1){
                /* Unicode BMP */
                format = mt_ttf_read_short(ttf);
                ttf->cmaps[i].format = format;
#if MT_DEBUG
                printf("mibitype: Character map format: %d\n", format);
#endif
                /* TODO: Implement this. */
                ttf->best_map = i;
            }
        }
    }
    return 0;
}

size_t mt_ttf_get_index(MTTTF *ttf, uint32_t c) {
    size_t n;
    uint32_t start_char, end_char, start_index;
    MTTTFCmap *cmap;
    cmap = ttf->cmaps+ttf->best_map;
    /* TODO: Make something clean. */
    ttf->cur = cmap->data_cur;
    if(cmap->platform_id == 0){
        if(cmap->format == 4){
            /* TODO */
        }else if(cmap->format == 12){
            for(n=0;n<cmap->group_num;n++){
                start_char = mt_ttf_read_int(ttf);
                end_char = mt_ttf_read_int(ttf);
                start_index = mt_ttf_read_int(ttf);
                if(c >= start_char && c <= end_char){
                    return c-start_char+start_index;
                }
            }
        }
    }else if(cmap->platform_id == 3){
        /* TODO: Implement this correctly. */
        return c-0x1D;
    }
    return 0;
}

void mt_ttf_free(MTTTF *ttf) {
    free(ttf->table_dir);
    ttf->table_dir = NULL;
    free(ttf->glyphs);
    ttf->glyphs = NULL;
}
