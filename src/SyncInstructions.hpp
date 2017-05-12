/// @file SyncInstructions.hpp
/// @brief Descriptor of slide synchronization header file
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

#ifndef SYNCINSTRUCTIONS_HPP
#define SYNCINSTRUCTIONS_HPP 1

#include <vector>
#include <tuple>
#include <string>

using std::string;

namespace slidesync
{

// TODO change internal representation to be automatically sorted
//      so there's no issue with video generation

/// @brief Slide synchronization descriptor
/// 
/// @remarks Instructions are expected to be added sequentially in time;
///          otherwise, other API's behaviour is undefined. In particular
///          video generation will read the instructions sequentially and
///          will not be able to modify parts of the video it already generated
class SyncInstructions
{
private:
	/// @brief List of instructions
	/// 
	/// Each element has the form (frame_index, instructions),
	/// where the indices can be positive to indicate an absolute time
	/// or negative to indicate a relative jump forward;
	/// and the instructions can be either a positive slide index to jump to,
	/// or a special instruction indicated with a negative number (non-public enumeration).
	std::vector<std::tuple<int, int>> instructions;
	
	/// @brief Footage frame rate. Used for printing timestamps
	unsigned int framerate;
	
	/// @brief Current slide index after following instructions
	/// @remarks It assumes the presentation start with the first slide.
	///          If this is not appropriate, you should call GoTo() before anything else.
	unsigned int current_index;
	
	/// @brief Number of slides in the presentation
	unsigned int length;
	
public:
	/// @brief Construct a SyncInstructions object with no framerate
	/// 
	/// The object won't be able to calculate timestamps, so the raw frame indices
	/// will be used instead when printing.
	/// 
	/// @param[in] length Number of slides in the presentation.
	SyncInstructions(unsigned int length);
	
	/// @brief Construct a SyncInstructions object with a given framerate
	/// 
	/// @param[in] length Number of slides in the presentation.
	/// @param[in] framerate Footage frame rate.
	SyncInstructions(unsigned int length, unsigned int framerate);
	
	/// @brief Construct a SyncInstructions object from its string representation
	/// 
	/// @param[in] descriptor String representation.
	SyncInstructions(string descriptor);
	
	/// @brief Add a "next slide" instruction
	/// 
	/// @param[in] timestamp Frame index. It can be positive to indicate an absolute
	///                      index or negative to indicate a relative jump forward.
	/// @returns True if successful; otherwise, false. Trying to move to a
	///          non-existant or invalid slide will cause a failure.
	bool Next(int timestamp);
	
	/// @brief Add a "previous slide" instruction
	/// 
	/// @param[in] timestamp Frame index. It can be positive to indicate an absolute
	///                      index or negative to indicate a relative jump forward.
	/// @returns True if successful; otherwise, false. Trying to move to a
	///          non-existant or invalid slide will cause a failure.
	bool Previous(int timestamp);
	
	/// @brief Add a "go to slide" instruction
	/// 
	/// @param[in] timestamp Frame index. It can be positive to indicate an absolute
	///                      index or negative to indicate a relative jump forward.
	/// @param[in] index Frame index to jump to.
	/// @returns True if successful; otherwise, false. Trying to move to a
	///          non-existant or invalid slide will cause a failure.
	bool GoTo(int timestamp, unsigned int index);
	
	/// @brief Generate an appropriate string representation of the synchronization
	string ToString();
};

}

#endif
