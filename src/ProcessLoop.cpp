/// @file ProcessLoop.cpp
/// @brief Video processing abstract loop
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

#include <wx/wxprec.h>
#include <wx/timer.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "ProcessLoop.hpp"

namespace slidesync
{

/// @brief Definition of the event fired when the loop finishes processing the data
wxDEFINE_EVENT(LoopFinishedEvent, LoopEvent);

LoopEvent::LoopEvent(wxEventType type, int id)
	: wxEvent(id, type) {}

LoopEvent::LoopEvent(const LoopEvent& that)
	: wxEvent(that){}

wxEvent* LoopEvent::Clone() const
{
	return new LoopEvent(*this);
}

}
