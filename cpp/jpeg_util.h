#ifndef _JPEG_UTIL_H_
#define _JPEG_UTIL_H_

#include <stdio.h>
#include <jpeglib.h>

// we need the stuff below to load jpeg images from memory
// for some reason this funtionality is missing in early versions of libjpeg ...

void init_source(j_decompress_ptr cinfo);

boolean fill_input_buffer(j_decompress_ptr cinfo);

void skip_input_data(j_decompress_ptr cinfo, long num_bytes);

void term_source(j_decompress_ptr cinfo);

void jpg_mem_src(j_decompress_ptr cinfo, const void* buffer, long nbytes);

#endif
