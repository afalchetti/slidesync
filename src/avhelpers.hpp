/// @file avhelpers.hpp
/// @brief FFMPEG (libav*) based functions header file
/// 
/// FFMPEG is a pure C library, so it requires careful memory management.
/// Its direct use has been restricted to this module to simplify its use
/// for the rest of the program and make any memory leak easier to track.
/// The encoding flow is exposed through the RAII VideoEncoder class.
/// 
/// This header file shall not expose the FFMPEG headers to avoid
/// cluttering the global namespace.
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

#ifndef AVHELPERS_HPP
#define AVHELPERS_HPP 1

#include <memory>
#include <string>
#include <stdexcept>

#include <opencv2/opencv.hpp>

using std::string;
using cv::Mat;

namespace libav
{

/// @brief Initialize the FFMPEG library
void initialize_ffmpeg();

// Opaque reference to libav-using encoder. Separated to avoid exposing the
// libav internals to the rest of the project.
class VideoEncoderInternal;

/// @brief Video encoder stream
/// 
/// This objects acts similarly to std::ofstream, but takes OpenCV frames and outputs them
/// into appropriately formatted MP4 video files. It acquires and releases any required
/// resources.
class VideoEncoder
{
private:
	/// @brief Internal encoder
	std::unique_ptr<VideoEncoderInternal> encoder;
	
public:
	/// @brief Construct a VideoEncoder pointing to the file with the given name
	/// 
	/// @param[in] filename Name of the file to save the video to.
	/// @param[in] width Width of the output video.
	/// @param[in] height Height of the output video.
	/// @param[in] framerate Number of frames per seconds.
	VideoEncoder(const string& filename, unsigned int width, unsigned int height, unsigned int framerate);
	
	/// @brief Copy constructor. Deleted
	VideoEncoder(const VideoEncoder& that) = delete;
	
	/// @brief Move constructor. Deleted
	VideoEncoder(const VideoEncoder&& that) = delete;
	
	/// @brief Copy assignment. Deleted
	VideoEncoder& operator=(const VideoEncoder& that) = delete;
	
	/// @brief Move assignment. Deleted
	VideoEncoder& operator=(VideoEncoder&& that) = delete;
	
	/// @brief Destruct this VideoEncoder
	~VideoEncoder();
	
	/// @brief Flush the video file to disk and close any resources
	/// 
	/// Trying to operate on the object after closing will result in an exception.
	void Close();
	
	friend VideoEncoder& operator<<(VideoEncoder& stream, const Mat& image);
	friend VideoEncoder& operator<<(VideoEncoder& stream, unsigned int repeat);
};

/// @brief Encode a RGB frame to video file
/// 
/// @param[inout] stream Video encoder stream.
/// @param[in] image New frame to append to the encoder stream.
/// @returns Reference to the stream.
VideoEncoder& operator<<(VideoEncoder& stream, const Mat& image);

/// @brief Repeat the encoding of the last frame a number of times
/// 
/// @param[inout] stream Video encoder stream.
/// @param[in] repeat Number of times to re-encode the last frame into the stream.
/// @returns Reference to the stream.
VideoEncoder& operator<<(VideoEncoder& stream, unsigned int repeat);

/// @brief Exception generated inside the FFMPEG library
class avexception : std::runtime_error
{
public:
	/// @brief Construct an avexception with the given message
	/// 
	/// @param[in] message Explanatory message.
	avexception(const char* message) noexcept;
	
	/// @brief Recover the explanatory message
	virtual const char* what() const noexcept;
};

}

#endif
