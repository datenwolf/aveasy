/* aveasy - a simple wrapper around libavformat, libavdevice and libswscale
Copyright (C) 2010, 2011  Wolfgang Draxinger <code+aveasy@datenwolf.net>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/pixfmt.h>

struct AVEasyInputContext;
typedef struct AVEasyInputContext AVEasyInputContext;


AVEasyInputContext *aveasy_input_open_v4l2(
	char const * const path, 
	unsigned short width, 
	unsigned short height,
	enum CodecID connection_codec, 
	enum PixelFormat pixel_format );


void aveasy_input_close(AVEasyInputContext * const ctx);

void *aveasy_input_read_frame(AVEasyInputContext * const ctx);

int aveasy_input_width(AVEasyInputContext const * const ctx);
int aveasy_input_height(AVEasyInputContext const * const ctx);
size_t aveasy_input_buffer_size(AVEasyInputContext const * const ctx);
void* aveasy_input_buffer(AVEasyInputContext const * const ctx);

