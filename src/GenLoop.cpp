/// @file GenLoop.cpp
/// @brief Sync file to video generation processing loop
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

#include <string>
#include <iostream>
#include <fstream>

#include "ProcessLoop.hpp"
#include "GenLoop.hpp"
#include "util.hpp"
#include "avhelpers.hpp"

using std::string;

namespace slidesync
{

GenLoop::GenLoop(std::vector<Mat>* slides, const SyncInstructions& instructions, const string& filename)
	: slides(slides),
	  instructions(instructions),
	  instructions_it(this->instructions.cbegin()),
	  timestamp(0),
	  slide(0),
	  encoder(filename,
	          (slides != nullptr && slides->size() > 0) ? (*slides)[0].cols : 0,
	          (slides != nullptr && slides->size() > 0) ? (*slides)[0].rows : 0,
	          instructions.Framerate()),
	  processor(&GenLoop::writeframe),
	  processing(false)
{
	if (instructions_it == this->instructions.cend()) {
		return;
	}
	
	// do not add the first frame if the first instruction sets a new first frame
	if (instructions_it->timestamp == 0) {
		switch (instructions_it->code) {
		case SyncInstructionCode::Next:
			slide = 1;
			break;
			
		case SyncInstructionCode::GoTo:
			slide = instructions_it->data;
			break;
		default:
			break;
		}
	}
	
	encoder << (*slides)[slide];
}

void GenLoop::Notify()
{
	if (processing) {
		return;
	}
	
	processing = true;
	(this->*processor)();
	processing = false;
}

void GenLoop::writeframe()
{
	if (instructions_it == instructions.cend()) {
		processor = &GenLoop::idle;
		
		LoopEvent loopfinished(LoopFinishedEvent);
		wxPostEvent(this, loopfinished);
		return;
	}
	
	int delta = (instructions_it->relative) ? instructions_it->timestamp :
	                                          instructions_it->timestamp - timestamp;
	
	// do not execute overlapping instructions, otherwise the result could be
	// the wrong length, e.g. 1000 overlapping instructions in a 2-frame video
	// will be at least 1000 frames of output
	if (delta == 0) {
		++instructions_it;
		return;
	}
	
	const unsigned int logtime     = 8;
	unsigned int       remaining   = delta - 1;
	unsigned int       frame_index = timestamp;
	unsigned int       framerate   = instructions.Framerate();
	
	for (; remaining > logtime; remaining -= logtime, frame_index += logtime) {
		std::cout << "Encoding... [" << index2timestamp(frame_index, framerate) << "]" << std::endl;
		encoder << logtime;
		
		wxTheApp->Yield();
	}
	
	std::cout << "Encoding... [" << index2timestamp(frame_index, framerate) << "]" << std::endl;
	encoder << remaining;
	
	switch (instructions_it->code) {
	case SyncInstructionCode::Next:
		slide += 1;
		break;
		
	case SyncInstructionCode::Previous:
		slide -= 1;
		break;
		
	case SyncInstructionCode::GoTo:
		slide = instructions_it->data;
		break;
		
	default:
		break;
	}
	
	encoder << (*slides)[slide];
	
	timestamp = instructions_it->timestamp;
	++instructions_it;
}

void GenLoop::idle() {}

}
