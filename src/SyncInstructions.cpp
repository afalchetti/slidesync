/// @file SyncInstructions.cpp
/// @brief Descriptor of slide synchronization
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

#include <vector>
#include <tuple>
#include <string>
#include <sstream>
#include <ios>

#include "SyncInstructions.hpp"
#include "util.hpp"

namespace slidesync
{

enum {SYNC_NEXT     = -1,
      SYNC_PREVIOUS = -2};

SyncInstructions::SyncInstructions(unsigned int length)
	: instructions(), framerate(0), current_index(0), length(length) {}

SyncInstructions::SyncInstructions(unsigned int length, unsigned int framerate)
	: instructions(), framerate(framerate), current_index(0), length(length) {}

SyncInstructions::SyncInstructions(string descriptor)
	: instructions(), framerate(0), current_index(0), length(0)
{
	std::istringstream reader(descriptor);
	unsigned int       ninstructions;
	
	reader.exceptions(reader.failbit);
	
	reader >> Skip("nslides")       >> Skip("=") >> length        >> Skip("\n");
	reader >> Skip("framerate")     >> Skip("=") >> framerate     >> Skip("\n");
	reader >> Skip("ninstructions") >> Skip("=") >> ninstructions >> Skip("\n");
	
	for (unsigned int i = 0; i < ninstructions; i++) {
		reader >> Skip("[");
		reader >> Skip("");
		// skipping the empty string will force the reader to discard any whitespace
		// this is to make the format symmetrical; otherwise "[123 ]" would be allowed, but "[ 123]" would not
		// and between allowing the left space and forbidding the right one, the lenient first option is
		// preferred to maximize user happiness (see: HTML parsers :P)
		
		bool relative = (reader.peek() == '+');
		int  index = 0;
		
		
		if (relative) {  // discard the "+"
			reader.get();
		}
		
		if (framerate != 0) {
			string timestamp(11, ' ');  // "HH:mm:ss.FF"
			reader.read(&timestamp[0], 11);
			
			index = timestamp2index(timestamp, framerate);
		}
		else {
			reader >> index;
		}
		
		if (relative) {
			index = -index;
		}
		
		reader >> Skip("]") >> Skip(":");
		
		string instruction_str;
		std::getline(reader, instruction_str, '\n');
		int    instruction;
		
		if (instruction_str == "next") {
			instruction = SYNC_NEXT;
		}
		else if (instruction_str == "previous") {
			instruction = SYNC_PREVIOUS;
		}
		else if (instruction_str.compare(0, 6, "go to ") == 0) {
			instruction = std::stoi(instruction_str.substr(6)) - 1;
			
			if (instruction < 0 || (unsigned int) instruction >= length) {
				continue;
			}
		}
		else {
			continue;
		}
		
		instructions.push_back(std::make_tuple(index, instruction));
	}
	
}

bool SyncInstructions::Next(int timestamp)
{
	if (current_index >= length - 1) {
		return false;
	}
	
	instructions.push_back(std::make_tuple(timestamp, SYNC_NEXT));
	current_index += 1;
	
	return true;
}

bool SyncInstructions::Previous(int timestamp)
{
	if (current_index < 1) {
		return false;
	}
	
	instructions.push_back(std::make_tuple(timestamp, SYNC_PREVIOUS));
	current_index -= 1;
	
	return true;
}

bool SyncInstructions::GoTo(int timestamp, unsigned int index)
{
	if (index >= length) {
		return false;
	}
	
	instructions.push_back(std::make_tuple(timestamp, index));
	current_index = index;
	
	return true;
}

string SyncInstructions::ToString() const
{
	std::ostringstream writer;
	
	// using "\n" instead of std::endl to make the file format system-agnostic
	writer << "nslides = "       << length              << "\n";
	writer << "framerate = "     << framerate           << "\n";
	writer << "ninstructions = " << instructions.size() << "\n";
	
	for (unsigned int i = 0; i < instructions.size(); i++) {
		int index       = std::get<0>(instructions[i]);
		int instruction = std::get<1>(instructions[i]);
		
		writer << "[";
		
		if (index < 0) {
			writer << "+";
			index = -index;
		}
		
		if (framerate != 0) {
			writer << index2timestamp(index, framerate);
		}
		else {
			writer << index;
		}
		
		writer << "]: ";
		
		if (instruction >= 0) {
			writer << "go to " << (instruction + 1);
		}
		else {
			switch(instruction) {
			case SYNC_NEXT:
				writer << "next";
				break;
				
			case SYNC_PREVIOUS:
				writer << "previous";
				break;
				
			default:
				writer << "unrecognized(" << -instruction << ")";
			}
		}
		
		writer << "\n";
	}
	
	return writer.str();
}

}
