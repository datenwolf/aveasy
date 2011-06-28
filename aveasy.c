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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "aveasy.h"

void describe_AVInputFormat(
	AVInputFormat const *const iformat )
{
	if( !iformat )
		return;

	fprintf( stderr,
		 "name: %s\n"
		 "long_name: %s\n"
		 "priv_data_size: %d\n"
		 "read_probe: %p\n"
		 "read_header: %p\n" "read_packet: %p\n" "read_close: %p\n"
#if LIBAVFORMAT_VERSION_MAJOR < 53
		 "read_seek: %p\n"
#endif
		 "read_timestamp: %p\n"
		 "flags %x\n"
		 "read_play: %p\n"
		 "read_pause: %p\n"
		 "read_seek2: %p\n",
		 iformat->name,
		 iformat->long_name,
		 iformat->priv_data_size,
		 iformat->read_probe,
		 iformat->read_header,
		 iformat->read_packet, iformat->read_close,
#if LIBAVFORMAT_VERSION_MAJOR < 53
		 iformat->read_seek,
#endif
		 iformat->read_timestamp,
		 iformat->flags,
		 iformat->read_play, iformat->read_pause, iformat->read_seek2 );

}

struct AVEasyInputContext {
	AVFormatParameters  format_parameters;
	AVInputFormat      *input_format;
	AVFormatContext    *format_context;
	AVCodecContext     *codec_context;
	AVCodec            *codec;
	AVFrame            *encoded_frame;
	AVFrame            *raw_frame;
	struct SwsContext  *sws_context;
	int                 video_stream;
	enum PixelFormat    pixel_format;
	size_t              buffer_size;
	void               *buffer;
};

AVEasyInputContext *aveasy_input_open_v4l2(
	char const * const path, 
	unsigned short width, 
	unsigned short height,
	enum CodecID connection_codec, 
	enum PixelFormat pixel_format )
{
	AVEasyInputContext *ctx;
	int i;
	
	ctx = malloc(sizeof(*ctx));
	if(!ctx)
		goto fail_alloc_context;
	
	ctx->format_context = avformat_alloc_context();
	if( !ctx->format_context )
		goto fail_alloc_context;
	ctx->format_context->video_codec_id = connection_codec;
	
	memset(&ctx->format_parameters, 0, sizeof(ctx->format_parameters));
	ctx->format_parameters.prealloced_context = 1;
	ctx->format_parameters.width  = width;
	ctx->format_parameters.height = height;
	ctx->input_format = av_find_input_format("video4linux2");
	if(!ctx->input_format)
		goto fail_find_input_format;
	
	if( av_open_input_file( &ctx->format_context,
				path,
				ctx->input_format,
				0,
				&ctx->format_parameters ) )
		goto fail_open_input_file;
	
	if( av_find_stream_info( ctx->format_context ) < 0 )
		goto fail_find_stream_info;

	dump_format( ctx->format_context, 0, path, false );
	ctx->video_stream = -1;
	for(i = 0; i < ctx->format_context->nb_streams; ++i) {
		if( ctx->format_context->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO ) {
			ctx->video_stream = i;
			break;
		}
	}
	if( ctx->video_stream == -1 )
		goto fail_find_videostream;
		
	ctx->codec_context = ctx->format_context->streams[ctx->video_stream]->codec;
	if( !ctx->codec_context )
		goto fail_get_codec_context;

	ctx->sws_context = sws_getContext(
		ctx->codec_context->width,
		ctx->codec_context->height,
		ctx->codec_context->pix_fmt,
		ctx->codec_context->width,
		ctx->codec_context->height,
		ctx->pixel_format = pixel_format,
		SWS_FAST_BILINEAR,
		0, 0, 0 );
	if( !ctx->sws_context )
		goto fail_get_swscontext;
	
	ctx->codec = avcodec_find_decoder( ctx->codec_context->codec_id );
	if(! ctx->codec )
		goto fail_find_decoder;
	if( avcodec_open( ctx->codec_context, ctx->codec ) < 0 )
		goto fail_decoder_open;
	
	// Fix for some some codecs which report wrong frame rate
	if( ctx->codec_context->time_base.num > 999 &&
	    ctx->codec_context->time_base.den == 1 )
		ctx->codec_context->time_base.den = 1000;
	
	ctx->encoded_frame = avcodec_alloc_frame();
	if( !ctx->encoded_frame )
		goto fail_alloc_encoded_frame;
		
	ctx->raw_frame = avcodec_alloc_frame();
	if( !ctx->raw_frame )
		goto fail_alloc_raw_frame;

	ctx->buffer_size = avpicture_get_size(
		ctx->pixel_format,
		ctx->codec_context->width,
		ctx->codec_context->height );
	
	ctx->buffer = av_malloc( ctx->buffer_size );
	if( !ctx->buffer )
		goto fail_alloc_buffer;
	avpicture_fill(	(AVPicture*) ctx->raw_frame,
			ctx->buffer,
			ctx->pixel_format,
			ctx->codec_context->width,
			ctx->codec_context->height );
	

	return ctx;
/* ----------------------------------------------- */
fail_alloc_buffer:
	av_free(ctx->raw_frame);

fail_alloc_raw_frame:
	av_free(ctx->encoded_frame);

fail_alloc_encoded_frame:
fail_decoder_open:
fail_find_decoder:
	sws_freeContext(ctx->sws_context);

fail_get_swscontext:
fail_get_codec_context:
fail_find_videostream:
	for(i = 0; i < ctx->format_context->nb_streams; ++i) {
		if( ctx->format_context->streams[i]->codec ) {
			avcodec_close(ctx->format_context->streams[i]->codec);
		}
	}

fail_find_stream_info:
	av_close_input_file(ctx->format_context);

fail_open_input_file:
fail_find_input_format:
	av_free(ctx->format_context);

fail_alloc_context:
	return 0;
}

void aveasy_input_close(AVEasyInputContext * const ctx)
{
	int i;
	
	if(!ctx)
		return;

	av_free(ctx->raw_frame);
	av_free(ctx->encoded_frame);
	sws_freeContext(ctx->sws_context);
	for(i = 0; i < ctx->format_context->nb_streams; ++i) {
		if( ctx->format_context->streams[i]->codec ) {
			avcodec_close(ctx->format_context->streams[i]->codec);
		}
	}
	av_close_input_file(ctx->format_context);
	av_free(ctx->format_context);
}

void *aveasy_input_read_frame(AVEasyInputContext * const ctx)
{
	AVPacket packet;

	if(!ctx)
		return 0;

	av_init_packet(&packet);
	for(;;) {
		if( av_read_frame(ctx->format_context, &packet) ) {
			av_free_packet( &packet );
			return 0;
		}
			

		if( packet.stream_index == ctx->video_stream ) {
			int frame_finished = 0;
			avcodec_decode_video2( 
				ctx->codec_context, 
				ctx->encoded_frame, 
				&frame_finished,
				&packet);

			if( frame_finished ) {
				int ret =
				    sws_scale( ctx->sws_context,
				    	       ctx->encoded_frame->data,
					       ctx->encoded_frame->linesize,
					       0,
					       ctx->codec_context->height,
					       ctx->raw_frame->data,
					       ctx->raw_frame->linesize );
				av_free_packet( &packet );
				return ctx->buffer;
			}
		}

	}

	av_free_packet( &packet );
	return 0;
}

int aveasy_input_width(AVEasyInputContext const * const ctx)
{
	if(!ctx)
		return 0;
	return ctx->codec_context->width;
}

int aveasy_input_height(AVEasyInputContext const * const ctx)
{
	if(!ctx)
		return 0;
	return ctx->codec_context->height;
}

size_t aveasy_input_buffer_size(AVEasyInputContext const * const ctx)
{
	if(!ctx)
		return 0;
	return ctx->buffer_size;
}

void* aveasy_input_buffer(AVEasyInputContext const * const ctx)
{
	if(!ctx)
		return 0;
	return ctx->buffer;
}

