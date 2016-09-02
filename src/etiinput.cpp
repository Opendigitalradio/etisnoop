/*
   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
   Her Majesty the Queen in Right of Canada (Communications Research
   Center Canada)

   Copyrigth (C) 2014
   Matthias P. Braendli, matthias.braendli@mpb.li

   Taken from ODR-DabMod

   Supported file formats: RAW, FRAMED, STREAMED
   Supports re-sync to RAW ETI file
 */
/*
   This file is part of ODR-DabMod.

   ODR-DabMod is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   ODR-DabMod is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with ODR-DabMod.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "etiinput.hpp"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>

int identify_eti_format(FILE* inputFile, int *streamType)
{
    *streamType = ETI_STREAM_TYPE_NONE;

    struct stat inputFileStat;
    fstat(fileno(inputFile), &inputFileStat);
    size_t inputfilelength_ = inputFileStat.st_size;

    int nbframes_;

    uint32_t sync;
    uint32_t nbFrames;
    uint16_t frameSize;

    char discard_buffer[6144];

    if (fread(&sync, sizeof(sync), 1, inputFile) != 1) {
        fprintf(stderr, "Unable to read sync in input file!\n");
        perror("");
        return -1;
    }
    if ((sync == 0x49c5f8ff) || (sync == 0xb63a07ff)) {
        *streamType = ETI_STREAM_TYPE_RAW;
        if (inputfilelength_ > 0) {
            nbframes_ = inputfilelength_ / 6144;
        }
        else {
            nbframes_ = ~0;
        }
        if (fseek(inputFile, -sizeof(sync), SEEK_CUR) != 0) {
            // if the seek fails, consume the rest of the frame
            if (fread(discard_buffer, 6144 - sizeof(sync), 1, inputFile)
                    != 1) {
                fprintf(stderr, "Unable to read from input file!\n");
                perror("");
                return -1;
            }
        }
        return 0;
    }

    nbFrames = sync;
    if (fread(&frameSize, sizeof(frameSize), 1, inputFile) != 1) {
        fprintf(stderr, "Unable to read frame size in input file!\n");
        perror("");
        return -1;
    }
    sync >>= 16;
    sync &= 0xffff;
    sync |= ((uint32_t)frameSize) << 16;

    if ((sync == 0x49c5f8ff) || (sync == 0xb63a07ff)) {
        *streamType = ETI_STREAM_TYPE_STREAMED;
        frameSize = nbFrames & 0xffff;
        if (inputfilelength_ > 0) {
            nbframes_ = inputfilelength_ / (frameSize + 2);
        }
        else {
            nbframes_ = ~0;
        }
        if (fseek(inputFile, -6, SEEK_CUR) != 0) {
            // if the seek fails, consume the rest of the frame
            if (fread(discard_buffer, frameSize - 4, 1, inputFile)
                    != 1) {
                fprintf(stderr, "Unable to read from input file!\n");
                perror("");
                return -1;
            }
        }
        return 0;
    }

    if (fread(&sync, sizeof(sync), 1, inputFile) != 1) {
        fprintf(stderr, "Unable to read nb frame in input file!\n");
        perror("");
        return -1;
    }
    if ((sync == 0x49c5f8ff) || (sync == 0xb63a07ff)) {
        *streamType = ETI_STREAM_TYPE_FRAMED;
        if (fseek(inputFile, -6, SEEK_CUR) != 0) {
            // if the seek fails, consume the rest of the frame
            if (fread(discard_buffer, frameSize - 4, 1, inputFile)
                    != 1) {
                fprintf(stderr, "Unable to read from input file!\n");
                perror("");
                return -1;
            }
        }
        nbframes_ = ~0;
        return 0;
    }

    // Search for the sync marker byte by byte
    for (size_t i = 10; i < 6144 + 10; ++i) {
        sync >>= 8;
        sync &= 0xffffff;
        if (fread((uint8_t*)&sync + 3, 1, 1, inputFile) != 1) {
            fprintf(stderr, "Unable to read from input file!\n");
            perror("");
            return -1;
        }
        if ((sync == 0x49c5f8ff) || (sync == 0xb63a07ff)) {
            *streamType = ETI_STREAM_TYPE_RAW;
            if (inputfilelength_ > 0) {
                nbframes_ = (inputfilelength_ - i) / 6144;
            }
            else {
                nbframes_ = ~0;
            }
            if (fseek(inputFile, -sizeof(sync), SEEK_CUR) != 0) {
                if (fread(discard_buffer, 6144 - sizeof(sync), 1, inputFile)
                        != 1) {
                    fprintf(stderr, "Unable to read from input file!\n");
                    perror("");
                    return -1;
                }
            }
            return 0;
        }
    }

    (void)nbframes_; // suppress warning "nbframes_ unused"
    fprintf(stderr, "Bad input file format!\n");
    return -1;
}

int get_eti_frame(FILE* inputfile, int stream_type, void* buf)
{
    // Initialise buffer
    memset(buf, 0x55, 6144);

    uint16_t frameSize;
    if (stream_type == ETI_STREAM_TYPE_RAW) {
        frameSize = 6144;
    }
    else {
        if (fread(&frameSize, sizeof(frameSize), 1, inputfile) != 1) {
            // EOF
            return 0;
        }
    }

    if (frameSize > 6144) { // there might be a better limit
        printf("Wrong frame size %u in ETI file!\n", frameSize);
        return -1;
    }

    int read_bytes = fread(buf, 1, frameSize, inputfile);
    if (read_bytes != frameSize) {
        // A short read of a frame (i.e. reading an incomplete frame)
        // is not tolerated. Input files must not contain incomplete frames
        printf("Incomplete frame in ETI file!\n");
        return -1;
    }

    // We have added padding, so we always return 6144 bytes
    return 6144;
}

