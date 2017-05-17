/// @file SyncInstructions.cpp
/// @brief General utility functions
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
#include <sstream>
#include <ios>

#include <wx/wxprec.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "util.hpp"

namespace slidesync
{

bool isnumeric(int c)
{
	return '0' <= c && c <= '9';
}

int compare_lexiconumerical(const wxString& a, const wxString& b)
{
	unsigned int i;
	unsigned int k;
	int          diff;
	
	for (i = 0, k = 0; i < a.length() && k < b.length(); i++, k++) {
		if (isnumeric(a[i]) && isnumeric(b[k])) {
			unsigned int p;
			unsigned int q;
			
			// p and q will point to the end of the number
			for (p = i + 1; p < a.length() && isnumeric(a[p]); p++) {}
			for (q = k + 1; q < b.length() && isnumeric(b[q]); q++) {}
			
			// char lengths of the numbers
			int alen = p - i;
			int blen = q - k;
			
			if (alen != blen) {
				return alen - blen;
			}
			
			// the numbers have the same length, they can be compared lexicographically
			for (; i < p; i++, k++) {
				diff = (int) a[i] - (int) b[k];
				
				if (diff != 0) {
					return diff;
				}
			}
			
			i = p - 1;
			k = q - 1;
		}
		else {
			diff = (int) a[i] - (int) b[k];
			
			if (diff != 0) {
				return diff;
			}
		}
	}
	
	int aremaining = a.length() - i;
	int bremaining = b.length() - k;
	
	return aremaining - bremaining;
}

Skip::Skip(string literal)
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

std::istream& operator>>(std::istream& stream, const Skip& skip) {
	bool skipws = stream.skipws;
	stream >> std::noskipws;
	
	string remaining;
	
	if (stream.skipws) {
		string streamws  = "";
		
		while (std::isspace(stream.peek())) {
			streamws += stream.get();
		}
		
		if (streamws.length() < skip.literalws.length() ||
		    streamws.compare(streamws.length() - skip.literalws.length(),
		                     streamws.npos, skip.literalws) != 0) {
			stream.setstate(stream.failbit);
			
			if (stream.exceptions() & stream.failbit) {
				throw std::ios_base::failure("Literal leading whitespace not found");
			}
		}
		
		remaining = skip.literalword;
	}
	else {
		remaining = skip.literalws + skip.literalword;
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

string pad(string text, char fill, unsigned int size)
{
	unsigned int nfill = size - text.length();
	
	if (nfill < 0) {
		nfill = 0;
	}
	
	return string(nfill, fill).append(text);
}

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
	reader >> std::noskipws >> hours >> Skip(":") >> minutes >> Skip(":") >> seconds >> Skip(".") >> frames;
	
	return ((hours * 60 + minutes) * 60 + seconds) * framerate + frames;
}

}
