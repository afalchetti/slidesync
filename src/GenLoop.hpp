/// @file GenLoop.hpp
/// @brief Sync file to video generation processing loop header file
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

#ifndef GENLOOP_HPP
#define GENLOOP_HPP 1

#include <string>

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "ProcessLoop.hpp"
#include "SyncInstructions.hpp"
#include "avhelpers.hpp"

using std::string;
using cv::Mat;

namespace slidesync
{

class GenLoop;

/// @brief Internal video generation processor function pointer
typedef void (GenLoop::*GenProcessorFn)();

/// @brief Generate a video file from a slides file and a synchronization file. Core loop
class GenLoop : public ProcessLoop
{
private:
	/// @brief Observer reference for the list of slides to use for the slideshow
	std::vector<Mat>* slides;
	
	/// @brief Description of the slideshow transition times
	SyncInstructions instructions;
	
	/// @brief Iterator over the instructin list
	SyncInstructions::const_iterator instructions_it;
	
	/// @brief Timestamp of the previous instruction
	unsigned int timestamp;
	
	/// @brief Slide index after the previous instruction
	unsigned int slide;
	
	/// @brief Video encoder stream to file
	libav::VideoEncoder encoder;
	
	/// @brief Video generation processor
	/// 
	/// References the main routine which will be called periodically.
	GenProcessorFn processor;
	
	/// @brief Flag indicating if the loop is currently processing a frame or not
	bool processing;
	
public:
	/// @brief Construct a GenLoop
	/// 
	/// @param[in] slides List of slides to use for the slideshow.
	/// @param[in] instructions Description of the slideshow transition times.
	/// @param[in] filename Name of the output video file.
	GenLoop(std::vector<Mat>* slides, const SyncInstructions& instructions, const string& filename);
	
	/// @brief Recurrent action. Writes another frame to file
	virtual void Notify();

private:
	/// @brief Main processing stage. Write a frame to file
	void writeframe();
	
	/// @brief Idle processing stage. Do nothing
	/// 
	/// Usually entered when the work has finished or
	/// an error won't allow further processing
	void idle();
};

}

#endif
