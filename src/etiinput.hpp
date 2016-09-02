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
#include <stdio.h>

#ifndef _ETIINPUT_H_
#define _ETIINPUT_H_


#define ETI_STREAM_TYPE_NONE 0
#define ETI_STREAM_TYPE_RAW 1
#define ETI_STREAM_TYPE_STREAMED 2
#define ETI_STREAM_TYPE_FRAMED 3

/* Identify the stream type, and return 0 on success, -1 on failure */
int identify_eti_format(FILE* inputfile, int *stream_type);

/* Read the next ETI frame into buf, which must be at least 6144 bytes big
 * Return number of bytes read, or zero if EOF */
int get_eti_frame(FILE* inputfile, int stream_type, void* buf);

#endif

