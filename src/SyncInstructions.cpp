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

namespace slidesync
{

enum {SYNC_NEXT     = -1,
      SYNC_PREVIOUS = -2};

/// @brief std::istream skip-literal target
/// 
/// This structure can be used to skip a literal string from a stream
/// and check that indeed it was present. So,
/// 
///     int x, y;
///     istringstream reader("2 / 3");
///     reader >> x >> skip("/") >> y;
/// 
/// will set x to 2 and y 3, whilst
/// 
///     int x, y;
///     istringstream reader("2 + 3");
///     reader >> x >> skip("/") >> y;
/// 
/// will fail as istreams usually do with mismatched input.
/// 
/// @remarks Similarly to other read outputs, leading whitespace will be
///          disregarded if reader.skipws is set.
struct skip
{
	/// @brief Leading whitespace in the literal string to skip
	string literalws;
	
	/// @brief Literal string to skip, after removing the leading whitespace
	string literalword;
	
	/// @brief Construct a skip-literal target
	/// 
	/// @param[in] literal String to skip
	skip(string literal)
		: literalws(), literalword()
	{
		unsigned int wordstart = 0;
		
		for (; wordstart < literal.length(); wordstart++) {
			if (!std::isspace(literal[wordstart])) {
				break;
			}
		}
		
		literalws   = literal.substr(0, wordstart);
		literalword = literal.substr(wordstart);
	}
};

/// @brief Skip an expected string literal
/// 
/// @param[in] stream Input stream.
/// @param[in] toskip String literal to skip.
std::istream& operator>>(std::istream& stream, const skip& toskip) {
	bool skipws = stream.skipws;
	stream >> std::noskipws;
	
	string remaining;
	
	if (stream.skipws) {
		string streamws  = "";
		
		while (std::isspace(stream.peek())) {
			streamws += stream.get();
		}
		
		if (streamws.length() < toskip.literalws.length() ||
		    streamws.compare(streamws.length() - toskip.literalws.length(),
		                     streamws.npos, toskip.literalws) != 0) {
			stream.setstate(stream.failbit);
			
			if (stream.exceptions() & stream.failbit) {
				throw std::ios_base::failure("Literal leading whitespace not found");
			}
		}
		
		remaining = toskip.literalword;
	}
	else {
		remaining = toskip.literalws + toskip.literalword;
	}
	
	for (unsigned int i = 0; i < remaining.length(); i++) {
		char c = stream.peek();
		
		if (c != remaining[i]) {
			stream.setstate(stream.failbit);
			
			if (stream.exceptions() & stream.failbit) {
				
				throw std::ios_base::failure("Literal mismatch. Found '" +
				                             ((c == std::char_traits<char>::eof()) ? string("EOF") :
				                                                                     string(1, c)) +
				                             "' but expected '" + remaining[i] + "'");
			}
		}
		
		stream.get();
	}
	
	stream >> skipws;
	return stream;
}

/// @brief Prepend a character to a string until it has a given size
/// 
/// @param[in] text Input string.
/// @param[in] fill Filler character.
/// @param[in] size Desired string length.
/// @returns Padded string.
string pad(string text, char fill, unsigned int size)
{
	unsigned int nfill = size - text.length();
	
	if (nfill < 0) {
		nfill = 0;
	}
	
	return string(nfill, fill).append(text);
}

/// @brief Calculate the number of chars needed to write the number in decimal base
/// 
/// @param[in] x Written number.
unsigned int nchars(unsigned int x)
{
	if (x == 0) {
		return 1;
	}
	
	unsigned int nchars = 0;
	unsigned int power  = 1;
	
	while (x >= power) {
		power *= 10;
		nchars++;
	}
	
	return nchars;
}

/// @brief Get a timestamp representation (hours:minutes:seconds.frame) of a frame index
/// 
/// @param[in] index Frame index.
/// @param[in] framerate Footage frame rate.
/// @returns String representation.
string index2timestamp(unsigned int index, unsigned int framerate)
{
	if (framerate == 0) {
		return "";
	}
	
	unsigned int frames       = index % framerate;
	unsigned int totalseconds = index / framerate;
	
	unsigned int seconds      = totalseconds % 60;
	unsigned int totalminutes = totalseconds / 60;
	
	unsigned int minutes = totalminutes % 60;
	unsigned int hours   = totalminutes / 60;
	
	return pad(std::to_string(hours),   '0', 2) + ":" +
	       pad(std::to_string(minutes), '0', 2) + ":" +
	       pad(std::to_string(seconds), '0', 2) + "." +
	       pad(std::to_string(frames),  '0', nchars(framerate));
}

/// @brief Get a frame index from its timestamp representation (hours:minutes:seconds.frame) 
/// 
/// @param[in] timestamp Timestamp string following the format "HH:mm:ss.FF", where
///                      HH is hours, mm is minutes, ss is seconds and FF is frame.
///                      Note that this timestamp is almost but not the same as the
///                      real time that has passed since the start of the footage;
///                      e.g. the framerate is usually 23.976 Hz but counted as
///                      24 frames per second.
/// @param[in] framerate Footage frame rate.
/// @returns Frame index.
int timestamp2index(string timestamp, unsigned int framerate)
{
	std::istringstream reader(timestamp);
	reader.exceptions(reader.failbit);
	
	unsigned int hours;
	unsigned int minutes;
	unsigned int seconds;
	unsigned int frames;
	
	// noskipws because otherwise it would be madness, e.g. "23:    12  : 24.  \t\n 16"
	// also, the compact format forces a constant timestamp length which simplifies memory allocation
	// (for any given framerate and reasonable number of hours, i.e. please don't process 100+ hour
	// dissertation videos, I'm sure people will bail before hour 32)
	reader >> std::noskipws >> hours >> skip(":") >> minutes >> skip(":") >> seconds >> skip(".") >> frames;
	
	return ((hours * 60 + minutes) * 60 + seconds) * framerate + frames;
}

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
	
	reader >> skip("nslides")       >> skip("=") >> length        >> skip("\n");
	reader >> skip("framerate")     >> skip("=") >> framerate     >> skip("\n");
	reader >> skip("ninstructions") >> skip("=") >> ninstructions >> skip("\n");
	
	for (unsigned int i = 0; i < ninstructions; i++) {
		reader >> skip("[");
		reader >> skip("");
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
		
		reader >> skip("]") >> skip(":");
		
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
			instruction = std::stoi(instruction_str.substr(6));
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

string SyncInstructions::ToString()
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
			writer << "go to" <<instruction;
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
