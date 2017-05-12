/// @file Loops.cpp
/// @brief Video processing loops
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

#include <cmath>

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
#include <wx/cmdline.h>
#include <wx/timer.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "CVCanvas.hpp"
#include "Loops.hpp"

using std::string;
using cv::Mat;

namespace slidesync
{

SyncLoop::SyncLoop(CVCanvas* canvas, cv::VideoCapture* footage, std::vector<Mat>* slides)
	: canvas(canvas),
	  footage(footage),
	  frame_index(0),
	  coarse_index(0),
	  length(footage->get(CV_CAP_PROP_FRAME_COUNT)), 
	  slides(slides),
	  slide_index(0),
	  prev_frame(),
	  prev_slidepose(),
	  sync_instructions(slides->size(), (unsigned int) round(footage->get(CV_CAP_PROP_FPS))),
	  processor(&SyncLoop::Initialize) {}

void SyncLoop::SetCanvas(CVCanvas* canvas)
{
	this->canvas = canvas;
}

void SyncLoop::SetFootage(cv::VideoCapture* footage)
{
	this->footage = footage;
}

void SyncLoop::Notify()
{
	(this->*processor)();
}

SyncInstructions SyncLoop::GetSyncInstructions()
{
	return sync_instructions;
}

void SyncLoop::Initialize()
{
	processor = &SyncLoop::Track;
}

void SyncLoop::Track()
{
	Mat frame;
	Mat gray;
	Mat display;
	
	(*footage) >> frame;
	
	for (unsigned int i = 0; i < frameskip; i++) {
		footage->grab();
	}
	
	if (frame.empty()) {
		return;
	}
	
	cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
	cv::cvtColor(frame, display, cv::COLOR_BGR2RGBA);
	
	canvas->UpdateGL(display);
	
	unsigned int nextindex = frame_index + frameskip + 1;
	
	if (nextindex < length) {
		coarse_index += 1;
		frame_index   = nextindex;
	}
	else {
		// reached the end of the video
	}
}

}
