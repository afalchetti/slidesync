/// @file util.hpp
/// @brief General utility functions header file
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

#ifndef SLIDESYNC_UTIL_HPP
#define SLIDESYNC_UTIL_HPP 1

#include <string>
#include <sstream>
#include <ios>

using std::string;

namespace slidesync
{

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
struct Skip
{
	/// @brief Leading whitespace in the literal string to skip
	string literalws;
	
	/// @brief Literal string to skip, after removing the leading whitespace
	string literalword;
	
	/// @brief Construct a skip-literal target
	/// 
	/// @param[in] literal String to skip.
	Skip(string literal);
};

/// @brief Skip an expected string literal
/// 
/// @param[in] stream Input stream.
/// @param[in] skip String literal to skip.
std::istream& operator>>(std::istream& stream, const Skip& skip);

/// @brief Prepend a character to a string until it has a given size
/// 
/// @param[in] text Input string.
/// @param[in] fill Filler character.
/// @param[in] size Desired string length.
/// @returns Padded string.
string pad(string text, char fill, unsigned int size);

/// @brief Calculate the number of chars needed to write the number in decimal base
/// 
/// @param[in] x Written number.
unsigned int nchars(unsigned int x);

/// @brief Get a timestamp representation (hours:minutes:seconds.frame) of a frame index
/// 
/// @param[in] index Frame index.
/// @param[in] framerate Footage frame rate.
/// @returns String representation.
string index2timestamp(unsigned int index, unsigned int framerate);

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
int timestamp2index(string timestamp, unsigned int framerate);

}

#endif
