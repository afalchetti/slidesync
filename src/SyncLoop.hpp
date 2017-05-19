/// @file SyncLoop.hpp
/// @brief Synchronization processing loop header file
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

#ifndef SYNCLOOP_HPP
#define SYNCLOOP_HPP 1

#include <opencv2/opencv.hpp>

#include "ProcessLoop.hpp"
#include "CVCanvas.hpp"
#include "Quad.hpp"
#include "SyncInstructions.hpp"

using std::string;
using cv::Mat;

namespace slidesync
{

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
	
	/// @brief Name of the cache file for the synchronization instructions
	string cachefname;
	
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
	unsigned int slide_index;
	
	/// @brief Keypoint detector
	cv::Ptr<cv::Feature2D> detector;
	
	/// @brief Keypoint matcher
	cv::Ptr<cv::DescriptorMatcher> matcher;
	
	/// @brief Precomputed keypoints for each slide
	std::vector<std::vector<cv::KeyPoint>> slide_keypoints;
	
	/// @brief Precomputed keypoint descriptors for each slide
	std::vector<Mat> slide_descriptors;
	
	/// @brief Reference frame (for differential processing)
	Mat ref_frame;
	
	/// @brief Previously computed keypoints for the reference frame
	std::vector<cv::KeyPoint> ref_frame_keypoints;
	
	/// @brief Previously computed keypoint descriptors for the reference frame
	Mat ref_frame_descriptors;
	
	/// @brief Subset of ref_frame_keypoints but only containing the keypoints inside the ref_slidepose Quad
	std::vector<cv::KeyPoint> ref_quad_keypoints;
	
	/// @brief Subset of ref_frame_descriptors but only containing the keypoints inside the ref_slidepose Quad
	Mat ref_quad_descriptors;
	
	/// @brief Index lookup table, indicating the index in ref_quad_keypoints for every element
	///        in ref_frame_keypoints
	/// 
	/// A value of -1 indicates the particular keypoint is not inside the presentation Quad.
	std::vector<int> ref_quad_indices;
	
	/// @brief Description of the slide pose in the reference frame
	/// 
	/// The quad's vertices can be outside the frame region, since the slides
	/// could be out-of-frame.
	Quad ref_slidepose;
	
	/// @brief Description of the slide pose in the previous frame
	/// 
	/// This quad will be used as a auxiliar reference in case the tracker gets lost.
	Quad prev_slidepose;
	
	/// @brief Count of consecutive frames where the presentation Quad has been closed to where it was
	///        int the previous slide (indicator of a robust match)
	unsigned int nearcount;
	
	/// @brief Count of consecutive frames the tracker hasn't been able to find anything decent (indicator
	///        of being totally lost and requiring a full scan through the slides)
	unsigned int badcount;
	
	/// @brief Synchronization instructions to match the slides with the footage
	SyncInstructions sync_instructions;
	
	/// @brief Frame-slide processor
	/// 
	/// References the main routine which will be called periodically.
	/// Since the processing consists of several stages which do very
	/// different operations, it makes sense to separate their implementations
	/// (and function delegates are nicer than big switches).
	SyncProcessorFn processor;
	
	/// @brief Flag indicating if the loop is currently processing a frame or not
	bool processing;
	
public:
	/// @brief Construct a SyncLoop
	/// 
	/// @param[in] canvas OpenGL canvas to draw user interactive interface.
	/// @param[in] footage Recording of the presentation.
	/// @param[in] slides Array of slide images.
	/// @param[in] cachefname Name of the SyncInstructions cache file.
	SyncLoop(CVCanvas* canvas, cv::VideoCapture* footage, std::vector<Mat>* slides,
	         const string& cachefname);
	
	/// @brief Set the internal canvas
	void SetCanvas(CVCanvas* canvas);
	
	/// @brief Set the internal footage
	void SetFootage(cv::VideoCapture* footage);
	
	/// @brief Recurrent action. Updates the canvas
	virtual void Notify();
	
	/// @brief Get the internal synchronization instructions
	SyncInstructions GetSyncInstructions();
	
private:
	/// @brief Get the next frame on the footage
	Mat next_frame();
	
	/// @brief Compute a matching between two images given their keypoints
	/// 
	/// @param[in] descriptors1 Corresponding descriptors in the first image.
	/// @param[in] descriptors2 Corresponding descriptors in the second image.
	/// @returns Matching keypoints
	std::vector<cv::DMatch> match(const Mat& descriptors1, const Mat& descriptors2);
	
	/// @brief Refine a matching using RANSAC and get an appropriate homography matrix
	/// 
	/// @param[in] keypoints1 Matching keypoints in the first image.
	/// @param[in] keypoints2 Corresponding matching keypoints in the second image.
	/// @param[in] matches Original matching keypoints.
	/// @param[out] inliers Filtered matching keypoints, after RANSAC.
	/// @returns Homography matrix.
	Mat refineHomography(const std::vector<cv::KeyPoint>& keypoints1, const std::vector<cv::KeyPoint>& keypoints2,
	                     const std::vector<cv::DMatch>& matches, std::vector<cv::DMatch>& inliers);
	
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
};

}

#endif
