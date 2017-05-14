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
class Quad
{
public:
	// Vertices
	
	/// @brief First vertex's X coordinate
	double X1;
	
	/// @brief First vertex's Y coordinate
	double Y1;
	
	/// @brief Second vertex's X coordinate
	double X2;
	
	/// @brief Second vertex's Y coordinate
	double Y2;
	
	/// @brief Third vertex's X coordinate
	double X3;
	
	/// @brief Third vertex's Y coordinate
	double Y3;
	
	/// @brief Fourth vertex's X coordinate
	double X4;
	
	/// @brief Fourth vertex's Y coordinate
	double Y4;
	
private:
	// Edge normals
	
	/// @brief First (non-unitary) normal's X coordinate
	double nx1;
	
	/// @brief First (non-unitary) normal's Y coordinate
	double ny1;
	
	/// @brief Second (non-unitary) normal's X coordinate
	double nx2;
	
	/// @brief Second (non-unitary) normal's Y coordinate
	double ny2;
	
	/// @brief Third (non-unitary) normal's X coordinate
	double nx3;
	
	/// @brief Third (non-unitary) normal's Y coordinate
	double ny3;
	
	/// @brief Fourth (non-unitary) normal's X coordinate
	double nx4;
	
	/// @brief Fourth (non-unitary) normal's Y coordinate
	double ny4;
	
	
	
public:
	/// @brief Construct a quad with every component set to zero
	Quad();
	
	/// @brief Construct a new from its vertices' coordinates
	/// 
	/// @param[in] x1 First vertex's X coordinate
	/// @param[in] y1 First vertex's Y coordinate
	/// @param[in] x2 Second vertex's X coordinate
	/// @param[in] y2 Second vertex's Y coordinate
	/// @param[in] x3 Third vertex's X coordinate
	/// @param[in] y3 Third vertex's Y coordinate
	/// @param[in] x4 Fourth vertex's X coordinate
	/// @param[in] y4 Fourth vertex's Y coordinate
	Quad(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);
	
	/// @brief Transform this Quad following a perspective homography matrix
	/// 
	/// @param[in] homography Homography matrix
	/// @returns Transformed version of this Quad
	Quad Perspective(Mat homography) const;
	
	/// @brief True if the specified point lies within the region defined by the Quad
	/// 
	/// @remarks This operation is only well-behaved for convex clockwise Quads.
	///          If the Quad is counterclockwise, the returned value will be inverted, i.e.
	///          it will return true if the point is outside the quad; if it is not convex,
	///          the result will be arbitrary (but it will not raise undefined behaviour in
	///          the language), i.e. there will be a region in space where this function
	///          returns true, but it may have nothing to do with the mathematical definition
	///          of the Quad and it may not even have properties such as continuity.
	/// 
	/// @param[in] x First coordinate of the point
	/// @param[in] y Second coordinate of the point
	bool Inside(double x, double y) const;
	
	/// @brief Get a string representation of this Quad
	string ToString() const;
};

}

#endif
