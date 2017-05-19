/// @file avhelpers.cpp
/// @brief FFMPEG (libav*) based functions
// 
// Part of SlideSync
// 
// Copyright 2017 Angelo Falchetti Pareja
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>
#include <string>
#include <stdexcept>

extern "C" {

#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

}

#include "avhelpers.hpp"

using std::string;
using cv::Mat;

namespace libav
{

// Global functions

void initialize_ffmpeg()
{
	av_register_all();
}

// VideoEncoderInternal declarations

/// @brief Non-public encoder which directly uses the FFMPEG library
class VideoEncoderInternal
{
private:
	/// @brief Container format of the video stream
	AVOutputFormat* format;
	
	/// @brief Context for the video stream
	AVFormatContext* context;
	
	/// @brief Internal libav video stream
	AVStream* stream;
	
	/// @brief Reference to the output frame image
	AVFrame* frame;
	
	/// @brief Current frame index
	unsigned frame_index;
	
	/// @brief Output video width
	unsigned int width;
	
	/// @brief Output video height
	unsigned int height;
	
	/// @brief Whether this encoder is ready to write frames
	bool ready;
	
public:
	/// @brief Construct a VideoEncoder pointing to the file with the given name
	/// 
	/// @param[in] filename Name of the file to save the video to.
	/// @param[in] width Width of the output video.
	/// @param[in] height Height of the output video.
	/// @param[in] framerate Number of frames per second.
	VideoEncoderInternal(const string& filename, unsigned int width, unsigned int height,
	                     unsigned int framerate);
	
	/// @brief Copy constructor. Deleted
	VideoEncoderInternal(const VideoEncoderInternal& that) = delete;
	
	/// @brief Move constructor. Deleted
	VideoEncoderInternal(const VideoEncoderInternal&& that) = delete;
	
	/// @brief Copy assignment. Deleted
	VideoEncoderInternal& operator=(const VideoEncoderInternal& that) = delete;
	
	/// @brief Move assignment. Deleted
	VideoEncoderInternal& operator=(VideoEncoderInternal&& that) = delete;
	
	/// @brief Destruct this VideoEncoder
	~VideoEncoderInternal();
	
	friend VideoEncoderInternal& operator<<(VideoEncoderInternal& stream, const Mat& image);
	friend VideoEncoderInternal& operator<<(VideoEncoderInternal& stream, unsigned int repeat);
};

/// @brief Encode a RGB frame to video file
/// 
/// @param[inout] stream Video encoder stream.
/// @param[in] image New frame to append to the encoder stream.
/// @returns Reference to the stream.
VideoEncoderInternal& operator<<(VideoEncoderInternal& stream, const Mat& image);

/// @brief Repeat the encoding of the last frame a number of times
/// 
/// @param[inout] stream Video encoder stream.
/// @param[in] repeat Number of times to re-encode the last frame into the stream.
/// @returns Reference to the stream.
VideoEncoderInternal& operator<<(VideoEncoderInternal& stream, unsigned int repeat);

// VideoEncoder definitions

VideoEncoder::VideoEncoder(const string& filename, unsigned int width, unsigned int height,
                           unsigned int framerate)
	: encoder(new VideoEncoderInternal(filename, width, height, framerate)) {}

VideoEncoder::~VideoEncoder() = default;

void VideoEncoder::Close()
{
	encoder.reset(nullptr);
}

VideoEncoder& operator<<(VideoEncoder& stream, const Mat& image)
{
	(*stream.encoder) << image;
	
	return stream;
}

VideoEncoder& operator<<(VideoEncoder& stream, unsigned int repeat)
{
	(*stream.encoder) << repeat;
	
	return stream;
}

// VideoEncoderInternal definitions

VideoEncoderInternal::VideoEncoderInternal(const string& filename, unsigned int width, unsigned int height,
                                           unsigned int framerate)
	: format(nullptr),
	  context(nullptr),
	  stream(nullptr),
	  frame(nullptr),
	  frame_index(0),
	  width(width),
	  height(height),
	  ready(false)
{
	format = av_guess_format(nullptr, filename.c_str(), nullptr);
	
	if (format == nullptr) {
		throw avexception("Can't find suitable format for 'mp4'");
	}
	
	context = avformat_alloc_context();
	
	if (context == nullptr) {
		throw avexception("Can't allocate format context");
	}
	
	context->oformat = format;
	snprintf(context->filename, sizeof (context->filename), "%s", filename.c_str());
	
	if (format->video_codec == CODEC_ID_NONE) {
		throw avexception("No video codec");
	}
	
	// defining codec settings
	
	AVCodec* codec = avcodec_find_encoder(format->video_codec);
	
	if (codec == nullptr) {
		throw avexception("Can't find suitable codec for encoding");
	}
	
	stream = avformat_new_stream(context, codec);
	
	if (stream == nullptr) {
		throw avexception("Can't create video stream");
	}
	
	AVCodecContext* cctx = stream->codec;
	stream->id           = 0;
	
	cctx->bit_rate = 2 * 1024 * 1024;
	cctx->width    = width;
	cctx->height   = height;
	cctx->gop_size = 18;
	cctx->pix_fmt  = PIX_FMT_YUV420P;
	
	// FFMPEG bug: when encoding MP4/H.264, it does not respect this framerate;
	// it only uses the denominator, so 23.976 fps becomes 23976 fps
	// 
	// So, for the time being, output at 24 fps, which will not be correctly aligned,
	// but which can be quickly fixed manually using ffmpeg itself (the program, not
	// the library) or another video editor
	if (false) {
	//if (framerate == 24 || framerate == 30) {
		cctx->time_base = AVRational{1001, 1000 * (int) framerate};
	}
	else {
		cctx->time_base = AVRational{1, (int) framerate};
	}
	
	if (format->flags & AVFMT_GLOBALHEADER) {
		cctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	
	if (avcodec_open2(cctx, codec, nullptr) < 0) {
		throw avexception("Can't open codec");
	}
	
	// opening the video file for writing
	
	frame = avcodec_alloc_frame();
	
	if (frame == nullptr) {
		throw avexception("Can't allocate frame");
	}
	
	frame->format = cctx->pix_fmt;
	frame->width  = cctx->width;
	frame->height = cctx->height;
	
	av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, cctx->pix_fmt, 32);
	
	if (avio_open(&context->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
		throw avexception("Can't open file for video writing");
	}
	
	avformat_write_header(context, nullptr);
	ready = true;
}

VideoEncoderInternal::~VideoEncoderInternal()
{
	if (context != nullptr) {
		if (ready) {
			av_write_trailer(context);
			ready = false;
		}
		
		if (context->pb != nullptr) {
			avio_closep(&context->pb);
		}
		
		if (stream != nullptr) {
			if (frame != nullptr) {
				av_freep(&frame->data[0]);
				av_freep(&frame);
			}
			
			avcodec_close(stream->codec);
		}
		
		avformat_free_context(context);
	}
	
}

VideoEncoderInternal& operator<<(VideoEncoderInternal& stream, const Mat& image)
{
	if ((unsigned int) image.cols != stream.width  ||
	    (unsigned int) image.rows != stream.height ||
	    image.type() != CV_8UC3) {
		throw std::invalid_argument("the frame must be in 8-8-8-bit RGB format (CV_8UC3 in OpenCV)");
	}
	
	Mat yuv;
	cvtColor(image, yuv, CV_RGB2YCrCb);
	
	for (unsigned int k = 0; k < stream.height; k++) {
		cv::Vec3b* row = yuv.ptr<cv::Vec3b>(k);
		
		for (unsigned int i = 0; i < stream.width; i++) {
			stream.frame->data[0][k * stream.frame->linesize[0] + i] = row[i][0];
		}
	}
	
	for (unsigned int k = 0, k2 = 0; k2 < stream.height; k++, k2 += 2) {
		cv::Vec3b* row = yuv.ptr<cv::Vec3b>(k2);
		
		for (unsigned int i = 0, i2 = 0; i2 < stream.width; i++, i2 += 2) {
			stream.frame->data[1][k * stream.frame->linesize[1] + i] = row[i2][1];
			stream.frame->data[2][k * stream.frame->linesize[2] + i] = row[i2][2];
		}
	}
	
	return stream;
}

VideoEncoderInternal& operator<<(VideoEncoderInternal& stream, unsigned int repeat)
{
	AVCodecContext* cctx = stream.stream->codec;
	
	for (unsigned int i = 0; i < repeat; i++) {
		AVPacket* packet  = static_cast<AVPacket*>(av_mallocz(sizeof (AVPacket)));
		av_init_packet(packet);
		packet->data = nullptr;
		packet->size = 0;
		
		stream.frame->pts = stream.frame_index;
		
		if (packet == nullptr) {
			throw avexception("Can't allocate packet");
		}
		
		int gotpacket;
		int encode_ret = avcodec_encode_video2(cctx, packet, stream.frame, &gotpacket);
		
		if (encode_ret < 0) {
			throw avexception(("Can't write video frame (" + std::to_string(-encode_ret) + ")").c_str());
		}
		
		if (gotpacket) {
			av_interleaved_write_frame(stream.context, packet);
		}
		
		stream.frame_index += 1;
	}
	
	return stream;
}

// avexception definitions

avexception::avexception(const char* message) noexcept
	: std::runtime_error(message) {}

const char* avexception::what() const noexcept
{
	return std::runtime_error::what();
}

}
