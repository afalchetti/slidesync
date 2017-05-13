/// @file Quad.hpp
/// @brief Two-dimensional quad descriptor (polygon with four vertices) header file
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

#ifndef QUAD_HPP
#define QUAD_HPP 1

#include <string>

#include <opencv2/opencv.hpp>

using std::string;
using cv::Mat;

namespace slidesync
{

/// @brief Two-dimensional polygon with four vertices
struct Quad
{
public:
	/// @brief First vertex's X-coordinate
	double X1;
	
	/// @brief First vertex's Y-coordinate
	double Y1;
	
	/// @brief Second vertex's X-coordinate
	double X2;
	
	/// @brief Second vertex's Y-coordinate
	double Y2;
	
	/// @brief Third vertex's X-coordinate
	double X3;
	
	/// @brief Third vertex's Y-coordinate
	double Y3;
	
	/// @brief Fourth vertex's X-coordinate
	double X4;
	
	/// @brief Fourth vertex's Y-coordinate
	double Y4;
	
public:
	/// @brief Construct a quad with every component set to zero
	Quad();
	
	/// @brief Construct a new from its vertices' coordinates
	Quad(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);
	
	/// @brief Transform this Quad following a perspective homography matrix
	Quad Perspective(Mat homography);
	
	/// @brief Get a string representation of this Quad
	string ToString();
};

}

#endif
