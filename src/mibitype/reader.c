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

#include <mibitype/reader.h>
#include <mibitype/errors.h>

#include <stdlib.h>
#include <stdio.h>

int mt_reader_init(MTReader *reader, char *file) {
    FILE *fp = fopen(file, "rb");

    if(fp == NULL) return MT_E_OPEN_FILE;

    fseek(fp, 0, SEEK_END);
    reader->size = ftell(fp);
    rewind(fp);

    reader->buffer = malloc(reader->size);

    if(reader->buffer == NULL){
        fclose(fp);

        return MT_E_OUT_OF_MEM;
    }

    fread(reader->buffer, 1, reader->size, fp);

    fclose(fp);

    return MT_E_NONE;
}

unsigned char mt_reader_read_char(MTReader *reader) {
    if(reader->cur+1 >= reader->size) return 0;

    return reader->buffer[reader->cur++];
}

unsigned short int mt_reader_read_short(MTReader *reader) {
    unsigned char byte1, byte2;

    if(reader->cur+2 >= reader->size) return 0;

    byte1 = reader->buffer[reader->cur++];
    byte2 = reader->buffer[reader->cur++];

    return (byte1<<8) | byte2;
}

unsigned long int mt_reader_read_int(MTReader *reader) {
    unsigned char byte1, byte2, byte3, byte4;

    if(reader->cur+4 >= reader->size) return 0;

    byte1 = reader->buffer[reader->cur++];
    byte2 = reader->buffer[reader->cur++];
    byte3 = reader->buffer[reader->cur++];
    byte4 = reader->buffer[reader->cur++];

    return (byte1<<24) | (byte2<<16) | (byte3<<8) | byte4;
}

void mt_reader_read_array(MTReader *reader, unsigned char *array,
                          size_t bytes) {
    size_t i;

    if(reader->cur+4 >= reader->size){
        for(i=0;i<bytes;i++){
            array[i] = 0;
        }
        return;
    }

    for(i=0;i<bytes;i++){
        array[i] = reader->buffer[reader->cur++];
    }
}

void mt_reader_free(MTReader *reader) {
    free(reader->buffer);
    reader->buffer = NULL;
}
