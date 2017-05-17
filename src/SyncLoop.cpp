/// @file SyncLoop.cpp
/// @brief Synchronization processing loop
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

#include <cmath>
#include <limits>
#include <iostream>
#include <fstream>
#include <vector>

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
#include <wx/timer.h>
#include <wx/file.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "CVCanvas.hpp"
#include "ProcessLoop.hpp"
#include "SyncLoop.hpp"
#include "SyncInstructions.hpp"
#include "util.hpp"

using std::vector;
using std::string;
using cv::Mat;

namespace slidesync
{

SyncLoop::SyncLoop(CVCanvas* canvas, cv::VideoCapture* footage, vector<Mat>* slides,
                   const string& cachefname)
	: cachefname(cachefname),
	  canvas(canvas),
	  footage(footage),
	  frame_index(0),
	  coarse_index(0),
	  length(footage->get(cv::CAP_PROP_FRAME_COUNT)), 
	  slides(slides),
	  slide_index(0),
	  detector(cv::BRISK::create()),
	  matcher(cv::DescriptorMatcher::create("BruteForce-Hamming")),
	  slide_keypoints(),
	  slide_descriptors(),
	  ref_frame(),
	  ref_frame_keypoints(),
	  ref_frame_descriptors(),
	  ref_quad_keypoints(),
	  ref_quad_descriptors(),
	  ref_quad_indices(),
	  ref_slidepose(),
	  prev_slidepose(),
	  nearcount(),
	  badcount(),
	  sync_instructions(slides->size(), (unsigned int) round(footage->get(cv::CAP_PROP_FPS))),
	  processor(&SyncLoop::initialize),
	  processing(false) {}

void SyncLoop::SetCanvas(CVCanvas* canvas)
{
	this->canvas = canvas;
}

void SyncLoop::SetFootage(cv::VideoCapture* footage)
{
	this->footage = footage;
}

void SyncLoop::Notify()
{
	if (processing) {
		return;
	}
	
	processing = true;
	(this->*processor)();
	processing = false;
}

SyncInstructions SyncLoop::GetSyncInstructions()
{
	return sync_instructions;
}

Mat SyncLoop::next_frame()
{
	Mat frame;
	
	if (frame_index >= length) {
		return frame;
	}
	
	(*footage) >> frame;
	
	for (unsigned int i = 0; i < frameskip; i++) {
		footage->grab();
	}
	
	coarse_index += 1;
	frame_index  += frameskip + 1;
	
	return frame;
}

/// @brief Filter a list of keypoint into those keypoints inside a quad
/// 
/// @param[in] keypoints Image keypoints.
/// @param[in] descriptors Corresponding keypoint descriptors.
/// @param[in] quad Quad to use as a filter.
/// @param[out] quad_keypoints Filtered image keypoints, inside the Quad.
/// @param[out] quad_descriptors Filtered keypoint descriptors, inside the Quad.
/// @returns Index lookup table m[i] indicating the corresponding index k for each in index i in keypoints,
///          i.e. keypoints[m[i]] = quad_keypoints[i]. If a keypoint is not inside the quad, it will be marked
///          with a -1.
static vector<int> quadfilter(const vector<cv::KeyPoint>& keypoints, const Mat& descriptors,
                               const Quad& quad, vector<cv::KeyPoint>& quad_keypoints, Mat& quad_descriptors)
{
	vector<int> lookup(keypoints.size(), -1);
	
	quad_keypoints.clear();
	quad_descriptors = Mat(0, descriptors.cols, descriptors.type());
	
	for (unsigned int i = 0, k = 0; i < keypoints.size(); i++) {
		bool inside = quad.Inside(keypoints[i].pt.x, keypoints[i].pt.y);
		
		if (inside) {
			quad_keypoints.push_back(keypoints[i]);
			quad_descriptors.push_back(descriptors.row(i));
			
			lookup[i] = k;
			k        += 1;
		}
	}
	
	return lookup;
}

/// @brief Robust version of Quad perspective which can handle degenerate cases. In particular,
///        if the homography matrix is empty, it will be replaced with one which sinks the Quad
///        into the origin
/// 
/// @param[in] quad Quad to transform.
/// @param[in] homography Perspective homography matrix, possibly empty.
/// @returns Transformed Quad.
static Quad quadperspective(const Quad& quad, const Mat& homography)
{
	if (homography.empty()) {
		return Quad();
	}
	
	return quad.Perspective(homography);
}

/// @brief Calculate the deviation (and deformation) between two Quads
/// 
/// @param[in] first First Quad.
/// @param[in] second Second Quad.
/// @param[out] deformation Maximum L2(corner displacement) after removing the effect of the deviation.
/// @returns Deviation: average displacement of the Quad's vertices.
static double quaddeviation(const Quad& first, const Quad& second, double& deformation)
{
	double diff[8] = {second.X1() - first.X1(), second.Y1() - first.Y1(),
	                  second.X2() - first.X2(), second.Y2() - first.Y2(),
	                  second.X3() - first.X3(), second.Y3() - first.Y3(),
	                  second.X4() - first.X4(), second.Y4() - first.Y4()};
	
	double avgdiff[2] = {(diff[0] + diff[2] + diff[4] + diff[6]) / 4,
	                     (diff[1] + diff[3] + diff[5] + diff[7]) / 4};
	
	double maxdeviation2 = 0;
	
	for (unsigned int i = 0; i < 8; i += 2) {
		double dx = diff[i]     - avgdiff[0];
		double dy = diff[i + 1] - avgdiff[1];
		
		double deviation2 = dx * dx + dy * dy;
		
		if (deviation2 > maxdeviation2) {
			maxdeviation2 = deviation2;
		}
	}
	
	deformation = std::sqrt(maxdeviation2);
	
	return std::sqrt(avgdiff[0] * avgdiff[0] + avgdiff[1] * avgdiff[1]);
}

/// @brief Minimum number of matches to consider a matching good
static const int min_matchsize = 5;

/// @brief Number of matches that is good enough regardless of the percentage of the total keypoints they are
static const int great_matchsize = 20;

/// @brief Internal function, do not use. Compute a cost for matching two frames, considering reprojection
///        errors and changes in the presentation Quad
/// 
/// @param[in] keypoints1 Keypoints for the first frame.
/// @param[in] keypoints2 Keypoints for the second frame.
/// @param[in] matches Matching between the keypoints in the two frames.
/// @param[in] homography Computed homography relating the frames.
/// @param[in] slidepose1 Pose of the presentation slide in the first frame.
/// @param[in] slidepose2 Pose of the presentation slide in the second frame.
/// @returns Matching cost.
static double matchcost(const vector<cv::KeyPoint>& keypoints1,
                        const vector<cv::KeyPoint>& keypoints2,
                        const vector<cv::DMatch>& matches, const Mat& homography,
                        const Quad& slidepose1, const Quad& slidepose2)
{
	// minimal deviation and deformation to consider them errors
	const double deviation0    = 5;      // 5 pixels of grace for slow camera movement
	const double deformation0  = 6 - 1;  // 6 pixels deformation will be as heavy as 1 pixel match error
	                                     // and after that this cost increments faster (heavy deformation
	                                     // is a strong indicator of a wrong slide)
	
	if (matches.size() < min_matchsize) {
		return std::numeric_limits<double>::infinity();
	}
	
	if (!slidepose1.ConvexClockwise() || !slidepose2.ConvexClockwise()) {
		return std::numeric_limits<double>::infinity();
	}
	
	if (slidepose1.Area() < 10 * 10 || slidepose2.Area() < 10 * 10) {
		return std::numeric_limits<double>::infinity();
	}
	
	if (slidepose1.Area() > 5000 * 5000 || slidepose2.Area() > 5000 * 5000) {
		return std::numeric_limits<double>::infinity();
	}
	
	double deformation;
	double deviation = quaddeviation(slidepose1, slidepose2, deformation);
	
	double deviationcost   = (deviation   > deviation0)   ?  deviation   - deviation0 : 0;
	double deformationcost = (deformation > deformation0) ? (deformation - deformation0) *
	                                                        (deformation - deformation0) : 0;
	double matchcost       = 0;
	int    matchsize       = matches.size();
	
	for (unsigned int i = 0; i < matches.size(); i++) {
		cv::Point2f keypoint_f            = keypoints1[matches[i].queryIdx].pt;
		double      keypoint[3]           = {keypoint_f.x, keypoint_f.y, 1};
		Mat         keypoint_homogeneous(3, 1, CV_64F, keypoint);
		Mat         projected_homogeneous = homography * keypoint_homogeneous;
		
		double* pjh = projected_homogeneous.ptr<double>();
		cv::Point2f projected(pjh[0] / pjh[2], pjh[1] / pjh[2]);
		
		double dx = projected.x - keypoints2[matches[i].trainIdx].pt.x;
		double dy = projected.y - keypoints2[matches[i].trainIdx].pt.y;
		
		double cost = std::sqrt(dx * dx + dy * dy);
		
		if (!std::isnan(cost)) {
			matchcost += cost;
		}
		else {
			matchsize -= 1;
		}
	}
	
	// some of the matches could be NaNs, so we must check again for match size
	if (matchsize < min_matchsize) {
		return std::numeric_limits<double>::infinity();
	}
	
	matchcost /= matchsize;
	
	return matchcost + deviationcost + deformationcost;
}

/// @brief Check if the slide sections of two frames are a good match
/// 
/// @param[in] keypoints1 Keypoints for the first frame.
/// @param[in] keypoints2 Keypoints for the second frame.
/// @param[in] matches Matching between the keypoints in the first frame and the ones in the second.
/// @param[in] slidepose1 Slide pose in the first frame.
/// @param[in] slidepose2 Slide pose in the second frame.
/// @returns True if the matching is good enough to be considered correct; otherwise, false.
static bool slidematch(const vector<cv::KeyPoint>& keypoints1, const vector<cv::KeyPoint>& keypoints2,
                       const vector<cv::DMatch>& matches, const Mat& homography,
                       const Quad& slidepose1, const Quad& slidepose2)
{
	if (matches.size() < min_matchsize) {
		return false;
	}
	
	const double min_ratio   = 0.1;
	double ratio1 = ((double) matches.size()) / keypoints1.size();
	double ratio2 = ((double) matches.size()) / keypoints2.size();
	
	if (homography.empty() || (matches.size() < great_matchsize && (ratio1 < min_ratio || ratio2 < min_ratio))) {
		return false;
	}
	
	double cost = matchcost(keypoints1, keypoints2,
	                        matches, homography,
	                        slidepose1, slidepose2);
	
	return cost < 20.0;
}

vector<cv::DMatch> SyncLoop::match(const Mat& descriptors1, const Mat& descriptors2)
{
	vector<vector<cv::DMatch>> matches;
	vector<cv::DMatch>         bestmatches;
	
	if (descriptors1.rows < 2 || descriptors2.rows < 2) {
		return bestmatches;
	}
	
	matcher->knnMatch(descriptors1, descriptors2, matches, 2);
	
	for (unsigned int i = 0; i < matches.size(); i++) {
		if (matches[i][0].distance < max_matchratio * matches[i][1].distance) {
			bestmatches.push_back(matches[i][0]);
		}
	}
	
	return bestmatches;
}

Mat SyncLoop::refineHomography(const vector<cv::KeyPoint>& keypoints1,
                               const vector<cv::KeyPoint>& keypoints2,
                               const vector<cv::DMatch>& matches,
                               vector<cv::DMatch>& inliers)
{
	vector<cv::Point2f> keypoints1_f;
	vector<cv::Point2f> keypoints2_f;
	
	for (unsigned int i = 0; i < matches.size(); i++) {
		keypoints1_f.push_back(keypoints1[matches[i].queryIdx].pt);
		keypoints2_f.push_back(keypoints2[matches[i].trainIdx].pt);
	}
	
	Mat homography;
	Mat inliers_mat;
	inliers.clear();
	
	if (keypoints1_f.size() >= min_matchsize) {
		homography = cv::findHomography(keypoints1_f, keypoints2_f,
		                                cv::RANSAC, RANSAC_threshold, inliers_mat);
		
		for (unsigned int i = 0; i < keypoints1_f.size(); i++) {
			if (inliers_mat.at<uchar>(i)) {
				inliers.push_back(matches[i]);
			}
		}
	}
	
	return homography;
}

/// @brief Draw a Quad into a OpenCV matrix
/// 
/// @param[in] canvas Matrix to draw the Quad in.
/// @param[in] quad Quad to draw.
/// @param[in] color Line color.
/// @param[in] offsetx Amount that every vertex will be moved in the X coordinate.
/// @param[in] offsety Amount that every vertex will be moved in the Y coordinate.
void drawquad(Mat& canvas, const Quad& quad, cv::Scalar color, double offsetx = 0, double offsety = 0)
{
	cv::Point vertices[4] = {cv::Point(quad.X1() + offsetx, quad.Y1() + offsety),
	                         cv::Point(quad.X2() + offsetx, quad.Y2() + offsety),
	                         cv::Point(quad.X3() + offsetx, quad.Y3() + offsety),
	                         cv::Point(quad.X4() + offsetx, quad.Y4() + offsety)};
	
	cv::line(canvas, vertices[0], vertices[1], color);
	cv::line(canvas, vertices[1], vertices[2], color);
	cv::line(canvas, vertices[2], vertices[3], color);
	cv::line(canvas, vertices[3], vertices[0], color);
}

void SyncLoop::initialize()
{
	// preprocess slide keypoints
	
	for (unsigned int i = 0; i < slides->size(); i++) {
		vector<cv::KeyPoint> keypoints;
		Mat                  descriptors;
		
		detector->detectAndCompute((*slides)[i], cv::noArray(), keypoints, descriptors);
		
		slide_keypoints  .push_back(keypoints);
		slide_descriptors.push_back(descriptors);
		
		wxTheApp->Yield(true);
	}
	
	if (wxFile::Exists(cachefname)) {
		std::ifstream instructions(cachefname);
		
		try {
			sync_instructions = SyncInstructions(instructions);
			LoopEvent loopfinished(LoopFinishedEvent);
			wxPostEvent(this, loopfinished);
			
			processor = &SyncLoop::idle;
			return;
		}
		catch (const std::ios_base::failure& e) {
			std::cerr << "Can't parse instructions file" << std::endl << e.what() << std::endl;
		}
	}
	
	// match the first frame to find the slides projection or screen in the footage
	
	Mat firstframe;
	
	// peek the first frame,
	// processing VideoCapture objects that are not rewindable
	// is not supported (e.g. realtime camera streams)
	(*footage) >> firstframe;
	footage->set(cv::CAP_PROP_POS_FRAMES, 0);
	
	cv::cvtColor(firstframe, firstframe, cv::COLOR_BGR2GRAY);
	
	vector<cv::KeyPoint> frame_keypoints;
	Mat                  frame_descriptors;
	
	detector->detectAndCompute(firstframe, cv::noArray(), frame_keypoints, frame_descriptors);
	
	wxTheApp->Yield(true);
	
	vector<cv::DMatch> matches = match(slide_descriptors[0], frame_descriptors);
	vector<cv::DMatch> filtered;
	
	wxTheApp->Yield(true);
	
	Mat homography = refineHomography(slide_keypoints[0], frame_keypoints, matches, filtered);
	
	wxTheApp->Yield(true);
	
	if (homography.empty()) {
		std::cerr << "Can't find a robust matching" << std::endl;
		processor = &SyncLoop::idle;
		
		return;
	}
	
	// locate the presentation in the footage frame
	
	double slidewidth  = (*slides)[0].cols;
	double slideheight = (*slides)[0].rows;
	
	Quad slidepose = quadperspective(Quad(         0,           0,
	                                               0, slideheight,
	                                      slidewidth, slideheight,
	                                      slidewidth,           0), homography);
	
	//  DEBUG The next section is only for visual inspection purposes, it is not executed in production
	// Mat display;
	// 
	// cv::drawMatches((*slides)[0], slide_keypoints[0], firstframe, frame_keypoints, filtered,
	//                 display, cv::Scalar(255, 0, 0), cv::Scalar(0, 0, 255));
	// 
	// drawquad(display, slidepose, cv::Scalar(125, 255, 42), footage->get(cv::CAP_PROP_FRAME_WIDTH), 0);
	// cv::imwrite("display_init.png", display);
	// END DEBUG
	
	ref_frame             = firstframe;
	ref_frame_keypoints   = frame_keypoints;
	ref_frame_descriptors = frame_descriptors;
	ref_slidepose         = slidepose;
	prev_slidepose        = slidepose;
	ref_quad_indices      = quadfilter(frame_keypoints, frame_descriptors, slidepose,
	                                   ref_quad_keypoints, ref_quad_descriptors);
	
	processor = &SyncLoop::track;
}

void SyncLoop::track()
{
	const double largedeviation   = 10;
	const double largedeformation = 7;
	const double largecost        = 1000;
	const double reasonablecost   = 40;
	
	std::cout << "Frame " << coarse_index << " (" << frame_index << " / "
	                                              << index2timestamp(frame_index, 24) << ")";
	
	Mat  frame     = next_frame();
	Quad slidepose = ref_slidepose;  // approximate the current Quad with the reference one, which should be
	                                 // close, and therefore, have most of the slide keypoints inside; if it
	                                 // turns out the real one is too far away from the reference one, the
	                                 // reference will be updated to point to this one to reduce future errors
	Mat  display;
	bool hardframe     = false;
	bool make_keyframe = false;  // make this the reference frame
	bool goodmatch     = true;   // the match is good enough to be a keyframe (it will become one if other
	                             // conditions also apply such as slide change or a large camera movement)
	
	if (frame.empty()) {
		std::cout << std::endl;
		processor = &SyncLoop::idle;
		
		std::ofstream file(cachefname);
		file << sync_instructions.ToString();
		
		LoopEvent loopfinished(LoopFinishedEvent);
		wxPostEvent(this, loopfinished);
		
		return;
	}
	
	cv::cvtColor(frame, display, cv::COLOR_BGR2RGBA);
	cv::cvtColor(frame, frame,   cv::COLOR_BGR2GRAY);
	
	vector<cv::KeyPoint> frame_keypoints;
	Mat                  frame_descriptors;
	
	detector->detectAndCompute(frame, cv::noArray(), frame_keypoints, frame_descriptors);
	
	wxTheApp->Yield(true);
	
	vector<cv::DMatch> matches   = match(ref_frame_descriptors, frame_descriptors);
	vector<cv::DMatch> filtered;
	
	wxTheApp->Yield(true);
	
	Mat homography = refineHomography(ref_frame_keypoints, frame_keypoints, matches, filtered);
	slidepose      = quadperspective(ref_slidepose, homography);
	
	vector<cv::KeyPoint> quad_keypoints;
	Mat                  quad_descriptors;
	vector<cv::DMatch>   quad_matches;
	
	vector<int> quad_indices = quadfilter(frame_keypoints, frame_descriptors, slidepose,
	                                      quad_keypoints, quad_descriptors);
	
	for (unsigned int i = 0; i < matches.size(); i++) {
		int ref_index  = ref_quad_indices[matches[i].queryIdx];
		int quad_index = quad_indices    [matches[i].trainIdx];
		if (ref_index >= 0 && quad_index >= 0) {
			quad_matches.push_back(cv::DMatch(ref_index, quad_index, matches[i].distance));
		}
	}
	
	drawquad(display, ref_slidepose, cv::Scalar(20, 40, 255, 255));
	
	wxTheApp->Yield(true);
	
	unsigned int new_slide_index = slide_index;
	
	if (homography.empty() ||
	    !slidematch(ref_quad_keypoints, quad_keypoints,
	                quad_matches, homography,
	                ref_slidepose, slidepose)) {
		// the match is weak, check if other slides work better
		
		hardframe = true;
		
		unsigned int         candidate_indices[7] = {slide_index, slide_index + 1, slide_index - 1,
		                                             slide_index + 2, slide_index - 2,
		                                             slide_index + 3, slide_index - 3};
		vector<unsigned int> candidates;
		
		unsigned int         bestslide = slide_index;
		Mat                  besthomography;
		vector<cv::DMatch>   bestmatches;
		Quad                 bestslidepose;
		double               bestcost = std::numeric_limits<double>::infinity();
		
		double slidewidth  = (*slides)[bestslide].cols;
		double slideheight = (*slides)[bestslide].rows;
		
		if (badcount < 7) {
			for (unsigned int i = 0; i < 7; i++) {
				if (candidate_indices[i] >= 0 && candidate_indices[i] < slides->size()) {
					candidates.push_back(candidate_indices[i]);
				}
			}
		}
		else {
			for (unsigned int i = 0; i < slides->size(); i++) {
				candidates.push_back(i);
			}
			
			// the first time, 7 bad frames are required. If there is still nothing good enough
			// repeat this process every 4 bad frames
			badcount -= 4;
		}
		
		wxTheApp->Yield(true);
		
		for (unsigned int i = 0; i < candidates.size(); i++) {
			matches    = match           (slide_descriptors[candidates[i]], frame_descriptors);
			
			wxTheApp->Yield(true);
			
			homography = refineHomography(slide_keypoints  [candidates[i]], frame_keypoints,
			                              matches, filtered);
			
			wxTheApp->Yield(true);
			
			slidepose = quadperspective(Quad(         0,           0,
			                                          0, slideheight,
			                                 slidewidth, slideheight,
			                                 slidewidth,           0), homography);
			
			double cost = matchcost(slide_keypoints[candidates[i]], frame_keypoints,
			                        filtered, homography, ref_slidepose, slidepose);
			
			if (cost < bestcost) {
				bestslide      = candidates[i];
				bestslidepose  = slidepose;
				besthomography = homography;
				bestmatches    = filtered;
				bestcost       = cost;
			}
			
			wxTheApp->Yield(true);
		}
		
		if (bestcost >= largecost) {
			double cost_alt = matchcost(slide_keypoints[bestslide], frame_keypoints,
			                            bestmatches, besthomography, prev_slidepose, bestslidepose);
			
			if (cost_alt < reasonablecost) {
				nearcount += 1;
				
				if (nearcount >= 3) {
					bestcost = cost_alt;
				}
			}
			else {
				nearcount = 0;
			}
		}
		else {
			nearcount = 0;
		}
		
		cv::Scalar linecolor;
		
		if (bestcost < largecost) {
			linecolor = cv::Scalar(125, 255, 42, 255);
			badcount  = 0;
		}
		else {
			// this frame is too bad, skip it and hope the next one is better
			linecolor     = cv::Scalar(255, 85, 42, 255);
			make_keyframe = false;
			goodmatch     = false;
			badcount     += 1;
		}
		
		new_slide_index = bestslide;
		slidepose       = bestslidepose;
		
		if (goodmatch && bestslide != slide_index) {
			make_keyframe = true;
			
			if (bestslide == slide_index + 1) {
				sync_instructions.Next(frame_index);
			}
			else if (bestslide == slide_index - 1) {
				sync_instructions.Previous(frame_index);
			}
			else {
				sync_instructions.GoTo(frame_index, bestslide);
			}
		}
		
		// TODO make these HUDs interactive so the user can edit them if necessary
		drawquad(display, bestslidepose, linecolor);
		
		wxTheApp->Yield(true);
		
		//DEBUG
		//Mat display2;
		//
		//cv::drawMatches((*slides)[bestslide], slide_keypoints[bestslide], frame, quad_keypoints,
		//                bestmatches, display2, cv::Scalar(255, 0, 0), cv::Scalar(0, 0, 255));
		//cv::imwrite("displayX.png", display2);
		// END DEBUG
	}
	else {
		badcount  = 0;
		nearcount = 0;
		
		drawquad(display, slidepose, cv::Scalar(125, 255, 42, 255));
		
		//DEBUG
		//Mat display2;
		//
		//cv::drawMatches(ref_frame, ref_frame_keypoints, frame, frame_keypoints, filtered,
		//                display2, cv::Scalar(255, 0, 0), cv::Scalar(0, 0, 255));
		//cv::imwrite("display_diff.png", display2);
		// END DEBUG
	}
	
	if (IsRunning()) {
		// if someone stopped the timer, it is probable the window has been destroyed in one of the Yield()
		// calls above, so canvas is no longer valid and this would segfault; bail out
		canvas->UpdateGL(display);
	}
	
	double deformation;
	double deviation = quaddeviation(ref_slidepose, slidepose, deformation);
	
	if (goodmatch && (deviation > largedeviation || deformation > largedeformation)) {
		make_keyframe = true;
	}
	
	std::cout << " -- Slide " << (slide_index + 1);
	
	if (make_keyframe) {
		std::cout << "    KF";
		
		slide_index           = new_slide_index;
		ref_frame             = frame;
		ref_frame_keypoints   = frame_keypoints;
		ref_frame_descriptors = frame_descriptors;
		ref_slidepose         = slidepose;
		ref_quad_indices      = quadfilter(frame_keypoints, frame_descriptors, slidepose,
		                                   ref_quad_keypoints, ref_quad_descriptors);
	}
	
	if (hardframe) {
		std::cout << "    H";
	}
	
	prev_slidepose = slidepose;
	
	std::cout << std::endl;
}

void SyncLoop::idle()
{
}

}
