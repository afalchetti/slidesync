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

SyncInstructions::SyncInstructions(std::istream& descriptor)
	: instructions(), framerate(0), current_index(0), length(0)
{
	unsigned int ninstructions;
	
	descriptor.exceptions(descriptor.failbit);
	
	descriptor >> Skip("nslides")       >> Skip("=") >> length        >> Skip("\n");
	descriptor >> Skip("framerate")     >> Skip("=") >> framerate     >> Skip("\n");
	descriptor >> Skip("ninstructions") >> Skip("=") >> ninstructions >> Skip("\n");
	
	for (unsigned int i = 0; i < ninstructions; i++) {
		descriptor >> Skip("[");
		descriptor >> Skip("");
		// skipping the empty string will force the reader to discard any whitespace
		// this is to make the format symmetrical; otherwise "[123 ]" would be allowed, but "[ 123]" would not
		// and between allowing the left space and forbidding the right one, the lenient first option is
		// preferred to maximize user happiness (see: HTML parsers :P)
		
		SyncInstruction instruction;
		
		instruction.relative  = (descriptor.peek() == '+');
		instruction.timestamp = 0;
		instruction.code      = SyncInstructionCode::Undefined;
		instruction.data      = 0;
		
		if (instruction.relative) {  // discard the "+"
			descriptor.get();
		}
		
		if (framerate != 0) {
			string timestamp(11, ' ');  // "HH:mm:ss.FF"
			descriptor.read(&timestamp[0], 11);
			
			instruction.timestamp = timestamp2index(timestamp, framerate);
		}
		else {
			descriptor >> instruction.timestamp;
		}
		
		descriptor >> Skip("]") >> Skip(":") >> Skip("");
		
		string instruction_str;
		std::getline(descriptor, instruction_str, '\n');
		
		if (instruction_str == "next") {
			instruction.code = SyncInstructionCode::Next;
		}
		else if (instruction_str == "previous") {
			instruction.code = SyncInstructionCode::Previous;
		}
		else if (instruction_str == "end") {
			instruction.code = SyncInstructionCode::End;
		}
		else if (instruction_str.compare(0, 6, "go to ") == 0) {
			instruction.code = SyncInstructionCode::GoTo;
			instruction.data = std::stoi(instruction_str.substr(6)) - 1;
			
			if (instruction.data < 0 || instruction.data >= length) {
				continue;
			}
		}
		else {
			continue;
		}
		
		instructions.push_back(instruction);
	}
	
}

bool SyncInstructions::Next(unsigned int timestamp, bool relative)
{
	if (current_index >= length - 1) {
		return false;
	}
	
	instructions.push_back(SyncInstruction{timestamp, SyncInstructionCode::Next, 0, relative});
	current_index += 1;
	
	return true;
}

bool SyncInstructions::Previous(unsigned int timestamp, bool relative)
{
	if (current_index < 1) {
		return false;
	}
	
	instructions.push_back(SyncInstruction{timestamp, SyncInstructionCode::Previous, 0, relative});
	current_index -= 1;
	
	return true;
}

bool SyncInstructions::GoTo(unsigned int timestamp, unsigned int index, bool relative)
{
	if (index >= length) {
		return false;
	}
	
	instructions.push_back(SyncInstruction{timestamp, SyncInstructionCode::GoTo, index, relative});
	current_index = index;
	
	return true;
}

bool SyncInstructions::End(unsigned int timestamp, bool relative)
{
	instructions.push_back(SyncInstruction{timestamp, SyncInstructionCode::End, 0, relative});
	
	return true;
}

SyncInstructions::const_iterator SyncInstructions::cbegin()
{
	return instructions.cbegin();
}

SyncInstructions::const_iterator SyncInstructions::cend()
{
	return instructions.cend();
}

int SyncInstructions::Framerate() const
{
	return framerate;
}

string SyncInstructions::ToString() const
{
	std::ostringstream writer;
	
	// using "\n" instead of std::endl to make the file format system-agnostic
	writer << "nslides = "       << length              << "\n";
	writer << "framerate = "     << framerate           << "\n";
	writer << "ninstructions = " << instructions.size() << "\n";
	
	for (unsigned int i = 0; i < instructions.size(); i++) {
		SyncInstruction instruction = instructions[i];
		
		writer << "[";
		
		if (instruction.relative < 0) {
			writer << "+";
		}
		
		if (framerate != 0) {
			writer << index2timestamp(instruction.timestamp, framerate);
		}
		else {
			writer << instruction.timestamp;
		}
		
		writer << "]: ";
		
		switch (instruction.code) {
		case SyncInstructionCode::GoTo:
			writer << "go to " << (instruction.data + 1);
			break;
			
		case SyncInstructionCode::Next:
			writer << "next";
			break;
			
		case SyncInstructionCode::Previous:
			writer << "previous";
			break;
			
		case SyncInstructionCode::End:
			writer << "end";
			break;
			
		default:
			writer << "unrecognized(" << static_cast<int>(instruction.code) << ")";
		}
		
		writer << "\n";
	}
	
	return writer.str();
}

}
