/// @file SlideSync.hpp
/// @brief Slide-video synchronizer header file
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

#ifndef SLIDESYNC_HPP
#define SLIDESYNC_HPP 1

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

using std::string;
using cv::Mat;

namespace slidesync
{

/// @brief Application state. Stage in the process
enum class SyncAppState
{
	Initializing,
	Synchronizing,
	GeneratingVideo
};

/// @brief Custom event identification numbers
enum class EventID
{
	SlideSyncID
};

}

#endif
