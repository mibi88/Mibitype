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

#include <mibitype/font.h>
#include <mibitype/errors.h>

#include <mibitype/loaderlist.h>

#include <string.h>

int mt_font_init(MTFont *font, MTReader *reader) {
    size_t i;
    int found = 0;

    int rc;

    font->reader = reader;

    font->data = NULL;

    MT_READER_JMP(reader, 0);

    /* Find what kind of file it is */
    for(i=0;i<MT_LOADER_AMOUNT;i++){
        if(!MT_LOADERLIST_GET(i, is_valid)(font->data, reader)){
            font->loader = i;
            found = 1;
            break;
        }
    }

    if(!found) return MT_E_UNKNOWN_TYPE;

    MT_READER_JMP(reader, 0);

    font->data = malloc(mt_loaders[font->loader].data_size);
    if(font->data == NULL) return MT_E_OUT_OF_MEM;

    if((rc = MT_LOADERLIST_GET(font->loader, init)(font->data, font))){
        return rc;
    }

    if((rc = MT_LOADERLIST_GET(font->loader, load_missing)(font->data, font,
                                                           &font->missing))){
        return rc;
    }

    return MT_E_NONE;
}

size_t mt_font_get_glyph_id(MTFont *font, size_t c) {
    return MT_LOADERLIST_GET(font->loader, get_glyph_id)(font->data, font, c);
}

MTGlyph *mt_font_search_glyph(MTFont *font, size_t c) {
    /* Some interpolation search */
    size_t first, last;
    size_t selected;

    if(!font->glyph_num) return NULL;

    first = 0;
    last = font->glyph_num-1;

    if(c == font->glyphs->c){
        return font->glyphs;
    }else if(c == font->glyphs[last].c){
        return font->glyphs+last;
    }else if(c < font->glyphs->c){
        font->expected_glyph_pos = 0;
        return NULL;
    }else if(c > font->glyphs[last].c){
        font->expected_glyph_pos = last+1;
        return NULL;
    }

    while(first != last) {
        selected = (last+first)/2;
        if(font->glyphs[selected].c == c){
            return font->glyphs+selected;
        }else if(font->glyphs[selected].c < c){
            first = selected+1;
        }else{
            last = selected;
        }
    }

    if(font->glyphs[first].c == c) return font->glyphs+first;

    font->expected_glyph_pos = first;
    return NULL;
}

int _mt_font_load_glyph(MTFont *font, size_t c) {
    void *new;
    int rc;
    MTGlyph tmp;

#if MT_DEBUG
    printf("mibitype: Load glyph %lu at %lu!\n", c, font->expected_glyph_pos);
#endif

    rc = MT_LOADERLIST_GET(font->loader, load_glyph)(font->data, font,
                           &tmp, mt_font_get_glyph_id(font, c));

    if(rc){
        mt_glyph_free(&tmp);
        return rc;
    }

    tmp.c = c;

    new = realloc(font->glyphs, (font->glyph_num+1)*sizeof(MTGlyph));
    if(new == NULL){
        mt_glyph_free(&tmp);
        return MT_E_OUT_OF_MEM;
    }
    font->glyphs = new;

    if(font->expected_glyph_pos < font->glyph_num){
        memmove(font->glyphs+font->expected_glyph_pos+1,
                font->glyphs+font->expected_glyph_pos,
                (font->glyph_num-font->expected_glyph_pos)*sizeof(MTGlyph));
    }

    font->glyphs[font->expected_glyph_pos] = tmp;

    font->glyph_num++;

    return rc;
}

MTGlyph *mt_font_get_glyph(MTFont *font, size_t c) {
    MTGlyph *glyph;

    glyph = mt_font_search_glyph(font, c);

    if(glyph == NULL){
#if MT_DEBUG
        size_t i;
        puts("mibitype: ========");
        for(i=0;i<font->glyph_num;i++){
            printf("mibitype: Glyph %lu is already loaded.\n",
                   font->glyphs[i].c);
        }
        puts("mibitype: ========");
#endif
        if(_mt_font_load_glyph(font, c)){
#if MT_DEBUG
            puts("mibitype: Failed to load glyph!");
#endif
            return &font->missing;
        }
        return font->glyphs+font->expected_glyph_pos;
    }

#if MT_DEBUG
    /*puts("mibitype: Glyph already loaded!");*/
#endif

    return glyph;
}

void mt_font_free(MTFont *font) {
    size_t i;

    for(i=0;i<font->glyph_num;i++){
        mt_glyph_free(font->glyphs+i);
    }

    mt_glyph_free(&font->missing);

    free(font->glyphs);
    font->glyphs = NULL;

    MT_LOADERLIST_GET(font->loader, free)(font->data, font);

    free(font->data);
    font->data = NULL;
}
