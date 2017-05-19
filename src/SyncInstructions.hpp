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

#include <iostream>
#include <vector>
#include <tuple>
#include <string>

using std::string;

namespace slidesync
{

// TODO change internal representation to be automatically sorted
//      so there's no issue with video generation

/// @brief Possible instructions to give a synchronized slideshow
enum class SyncInstructionCode {Undefined,
                                Next,
                                Previous,
                                GoTo,
                                End};

/// @brief Synchronization instruction
struct SyncInstruction
{
	/// @brief Frame index at which this instruction should be executed
	unsigned int timestamp;
	
	/// @brief Command to execute
	SyncInstructionCode code;
	
 	/// @brief Extra information for the executed command
	unsigned int data;
	
 	/// @brief True if the timestamp refers to an absolute frame index;
 	///        false if it is relative to the previous instruction
	bool relative;
};

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
	std::vector<SyncInstruction> instructions;
	
	/// @brief Footage frame rate. Used for printing timestamps
	unsigned int framerate;
	
	/// @brief Current slide index after following instructions
	/// @remarks It assumes the presentation start with the first slide.
	///          If this is not appropriate, you should call GoTo() before anything else.
	unsigned int current_index;
	
	/// @brief Number of slides in the presentation
	unsigned int length;
	
public:
	// note that aliasing the internal iterator exposes private definitions into the public API,
	// i.e. if we were to change the vector<> to a list<> any dependent project would break
	// (however as long as they reference it using the alias, it can be trivially recompiled
	// with the new headers). A proper solution would be to define a new iterator class, specific
	// for this class. The cost is considered so low, especially for this non-performant-sensitive
	// class, that the simpler solution has been chosen anyway
	
	/// @brief Constant iterator
	typedef std::vector<SyncInstruction>::const_iterator const_iterator;
	
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
	/// @param[in] descriptor String representation through a stream reader.
	SyncInstructions(std::istream& descriptor);
	
	/// @brief Add a "next slide" instruction
	/// 
	/// @param[in] timestamp Frame index.
	/// @param[in] relative True if the timestamp indicates an absolute timestamp;
	///                     false if it is relative to the previous instruction.
	/// @returns True if successful; otherwise, false. Trying to move to a
	///          non-existant or invalid slide will cause a failure.
	bool Next(unsigned int timestamp, bool relative = false);
	
	/// @brief Add a "previous slide" instruction
	/// 
	/// @param[in] timestamp Frame index.
	/// @param[in] relative True if the timestamp indicates an absolute timestamp;
	///                     false if it is relative to the previous instruction.
	/// @returns True if successful; otherwise, false. Trying to move to a
	///          non-existant or invalid slide will cause a failure.
	bool Previous(unsigned int timestamp, bool relative = false);
	
	/// @brief Add a "go to slide" instruction
	/// 
	/// @param[in] timestamp Frame index.
	/// @param[in] index Frame index to jump to.
	/// @param[in] relative True if the timestamp indicates an absolute timestamp;
	///                     false if it is relative to the previous instruction.
	/// @returns True if successful; otherwise, false. Trying to move to a
	///          non-existant or invalid slide will cause a failure.
	bool GoTo(unsigned int timestamp, unsigned int index, bool relative = false);
	
	/// @brief End the presentation
	/// 
	/// @param[in] timestamp Frame index.
	/// @param[in] relative True if the timestamp indicates an absolute timestamp;
	///                     false if it is relative to the previous instruction.
	/// @returns True if successful; otherwise, false.
	bool End(unsigned int timestamp, bool relative = false);
	
	/// @brief Get a constant iterator for the instruction list
	const_iterator cbegin();
	
	/// @brief Get a constant iterator end mark for the instruction list
	const_iterator cend();
	
	/// @brief Get the number of frame per second.
	int Framerate() const;
	
	/// @brief Generate an appropriate string representation of the synchronization
	string ToString() const;
};

}

#endif
