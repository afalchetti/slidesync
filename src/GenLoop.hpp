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

#include <wx/wxprec.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "ProcessLoop.hpp"
#include "SyncInstructions.hpp"

using std::string;

namespace slidesync
{

class GenLoop;

/// @brief Internal video generation processor function pointer
typedef void (GenLoop::*genProcessorFn)();

/// @brief Generate a video file from a slides file and a synchronization file. Core loop
class GenLoop : public ProcessLoop
{
public:
	/// @brief Construct a GenLoop
	GenLoop();
};

}

#endif
