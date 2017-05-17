/// @file ProcessLoop.hpp
/// @brief Video processing abstract loop header file
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

#ifndef PROCESSLOOP_HPP
#define PROCESSLOOP_HPP 1

#include <wx/wxprec.h>
#include <wx/timer.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

namespace slidesync
{

/// @brief Tag base class
class ProcessLoop : public wxTimer {};

class LoopEvent;

/// @brief Declaration of the event fired when the loop finishes processing the data
wxDECLARE_EVENT(LoopFinishedEvent, LoopEvent);

/// @brief Tag for events coming from the ProcessLoop hierarchy
class LoopEvent : public wxEvent
{
public:
	/// @brief Construct a LoopEvent object
	LoopEvent(wxEventType type, int id = 0);
	
	/// @brief Copy constructor. Copies any owned data
	LoopEvent(const LoopEvent& that);
	
	/// @brief Create a new deep copy of this event
	virtual wxEvent* Clone() const override;
};

}

#endif
