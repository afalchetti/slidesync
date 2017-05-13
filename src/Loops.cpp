/// @file Loops.cpp
/// @brief Video processing loops
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

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
#include <wx/cmdline.h>
#include <wx/timer.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "CVCanvas.hpp"
#include "Loops.hpp"

using std::vector;
using std::string;
using cv::Mat;

namespace slidesync
{

SyncLoop::SyncLoop(CVCanvas* canvas, cv::VideoCapture* footage, vector<Mat>* slides)
	: canvas(canvas),
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
	  prev_frame(),
	  prev_frame_keypoints(),
	  prev_frame_descriptors(),
	  prev_slidepose(),
	  sync_instructions(slides->size(), (unsigned int) round(footage->get(cv::CAP_PROP_FPS))),
	  processor(&SyncLoop::initialize) {}

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
	(this->*processor)();
}

SyncInstructions SyncLoop::GetSyncInstructions()
{
	return sync_instructions;
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
	
	vector<vector<cv::DMatch>> matches;
	vector<cv::KeyPoint>       match_slide;
	vector<cv::KeyPoint>       match_frame;
	vector<cv::Point2f>        match_slide_pt;
	vector<cv::Point2f>        match_frame_pt;
	vector<cv::DMatch>         filtered;
	
	matcher->knnMatch(slide_descriptors[0], frame_descriptors, matches, 2);
	
	for (unsigned int i = 0, k = 0; i < matches.size(); i++) {
		if (matches[i][0].distance < max_matchratio * matches[i][1].distance) {
			match_slide.push_back(slide_keypoints[0][matches[i][0].queryIdx]);
			match_frame.push_back(frame_keypoints   [matches[i][0].trainIdx]);
			
			match_slide_pt.push_back(match_slide[k].pt);
			match_frame_pt.push_back(match_frame[k].pt);
			
			k += 1;
		}
	}
	
	Mat homography;
	Mat inliers;
	
	if (match_slide.size() > 3) {
		homography = cv::findHomography(match_slide_pt, match_frame_pt,
		                                cv::RANSAC, RANSAC_threshold, inliers);
	}
	
	vector<cv::KeyPoint> inliers_slide;
	vector<cv::KeyPoint> inliers_frame;
	
	for (unsigned int i = 0, k = 0; i < match_slide.size(); i++) {
		if (inliers.at<uchar>(i)) {
			inliers_slide.push_back(match_slide[i]);
			inliers_frame.push_back(match_frame[i]);
			filtered.push_back(cv::DMatch(k, k, 0));
			k += 1;
		}
	}
	
	if (homography.empty()) {
		std::cerr << "Can't find a robust matching" << std::endl;
		processor = &SyncLoop::idle;
		
		return;
	}
	
	double slidewidth  = (*slides)[0].cols;
	double slideheight = (*slides)[0].rows;
	
	Quad slidepose = Quad(         0,           0,
	                               0, slideheight,
	                      slidewidth, slideheight,
	                      slidewidth,           0).Perspective(homography);
	
	prev_frame             = firstframe;
	prev_frame_keypoints   = frame_keypoints;
	prev_frame_descriptors = frame_descriptors;
	prev_slidepose         = slidepose;
	
	processor = &SyncLoop::idle;
}

void SyncLoop::track()
{
	Mat frame = next_frame();
	Mat display;
	
	if (frame.empty()) {
		processor = &SyncLoop::idle;
		return;
	}
	
	cv::cvtColor(frame, display, cv::COLOR_BGR2RGBA);
	cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
	
	canvas->UpdateGL(display);
	
	prev_frame = frame;
}

void SyncLoop::idle()
{
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

}
