/// @file Loops.hpp
/// @brief Video processing loops header file
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

#ifndef LOOPS_HPP
#define LOOPS_HPP 1

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
#include <wx/cmdline.h>
#include <wx/timer.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "CVCanvas.hpp"
#include "Quad.hpp"
#include "SyncInstructions.hpp"

using std::string;
using cv::Mat;

namespace slidesync
{

/// @brief Tag base class
class ProcessLoop : public wxTimer {};

class SyncLoop;

/// @brief Internal synchronization processor function pointer
typedef void (SyncLoop::*SyncProcessorFn)();

/// @brief Generate a synchronization file from a footage file and a slides file. Core loop
/// 
/// Match the slides in the slides file to the frames in the footage file and discover
/// which slides have been used at which times. Then, generate a synchronization file
/// describing such matches through "instructions" to the slideshow, such as "go to slide 3
/// at time 00:17:23.146" or "after 5 seconds go the next slide".
class SyncLoop : public ProcessLoop
{
private:
	/// @brief Number of frames to skip between keyframes
	///
	/// Presentation are very static so processing them at 30 fps would be incredibly wasteful.
	/// Hence, the video is subsampled, i.e. the effective framecount is framecount / (frameskip + 1).
	static const unsigned int frameskip = 7;
	
	/// @brief Maximum ratio between best match and second match's distance to consider
	///        a keypoint pair a good match
	static constexpr float max_matchratio = 0.8;
	
	/// @brief RANSAC threshold to decide a point is within the inlier group
	static constexpr float RANSAC_threshold = 2.5;
	
	/// @brief OpenGL canvas observer reference
	CVCanvas* canvas;
	
	/// @brief Video input observer reference
	cv::VideoCapture* footage;
	
	/// @brief Frame index for the next Notify() call
	unsigned int frame_index;
	
	/// @brief Coarse frame index for the next Notify() call
	/// 
	/// The canvas will skip frames; this index represents the effective
	/// frame as seen by the user, but not the real one in the video file.
	unsigned int coarse_index;
	
	/// @brief Footage length
	unsigned int length;
	
	/// @brief Slides image array observer reference
	std::vector<Mat>* slides;
	
	/// @brief Slide index
	int slide_index;
	
	/// @brief Keypoint detector
	cv::Ptr<cv::Feature2D> detector;
	
	/// @brief Keypoint matcher
	cv::Ptr<cv::DescriptorMatcher> matcher;
	
	/// @brief Precomputed keypoints for each slide
	std::vector<std::vector<cv::KeyPoint>> slide_keypoints;
	
	/// @brief Precomputed keypoint descriptors for each slide
	std::vector<Mat> slide_descriptors;
	
	/// @brief Previous frame (for differential processing)
	Mat prev_frame;
	
	/// @brief Previously computed keypoints for the previous frame
	std::vector<cv::KeyPoint> prev_frame_keypoints;
	
	/// @brief Previously computed keypoint descriptors for the previous frame
	Mat prev_frame_descriptors;
	
	/// @brief Description of the slide pose in the previous frame
	/// 
	/// The quad's vertices can be outside the frame region, since the slides
	/// could be out-of-frame.
	Quad prev_slidepose;
	
	/// @brief Synchronization instructions to match the slides with the footage
	SyncInstructions sync_instructions;
	
	/// @brief Frame-slide processor
	/// 
	/// References the main routine which will be called periodically.
	/// Since the processing consists of several stages which do very
	/// different operations, it makes sense to separate their implementations
	/// (and function delegates are nicer than big switches).
	SyncProcessorFn processor;
	
public:
	/// @brief Construct a SyncLoop
	/// 
	/// @param[in] canvas OpenGL canvas to draw user interactive interface
	/// @param[in] footage Recording of the presentation
	/// @param[in] slides Array of slide images
	SyncLoop(CVCanvas* canvas, cv::VideoCapture* footage, std::vector<Mat>* slides);
	
	/// @brief Set the internal canvas
	void SetCanvas(CVCanvas* canvas);
	
	/// @brief Set the internal footage
	void SetFootage(cv::VideoCapture* footage);
	
	/// @brief Recurrent action. Updates the canvas
	virtual void Notify();
	
	/// @brief Get the internal synchronization instructions
	SyncInstructions GetSyncInstructions();
	
private:
	/// @brief First processing stage. Initializes the required internal resources
	/// 
	/// Pre-processes the slide images and matches them to the first frame.
	void initialize();
	
	/// @brief Main processing stage. Follows the slide projection in the frame
	/// 
	/// As the processor detects slide changes in the footage, it will update
	/// the sync instructions, which can be retrieved later with GetSyncInstructions().
	void track();
	
	/// @brief Idle processing stage. Do nothing
	/// 
	/// Usually entered when the work has finished or
	/// an error won't allow further processing.
	void idle();
	
	/// @brief Get the next frame on the footage
	Mat next_frame();
};

}

#endif
