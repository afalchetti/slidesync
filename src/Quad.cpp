/// @file Quad.cpp
/// @brief Two-dimensional quad descriptor (polygon with four vertices)
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

#include <opencv2/opencv.hpp>

#include "Quad.hpp"

using std::string;
using cv::Mat;

namespace slidesync
{

Quad::Quad()
	: X1(0), Y1(0), X2(0), Y2(0), X3(0), Y3(0), X4(0), Y4(0),
	  nx1(0), ny1(0), nx2(0), ny2(0), nx3(0), ny3(0), nx4(0), ny4(0),
	  area(0), convexclockwise(true) {}

Quad::Quad(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
	: X1(x1), Y1(y1), X2(x2), Y2(y2), X3(x3), Y3(y3), X4(x4), Y4(y4),
	  nx1(y2 - y1), ny1(x1 - x2), nx2(y3 - y2), ny2(x2 - x3),
	  nx3(y4 - y3), ny3(x3 - x4), nx4(y1 - y4), ny4(x4 - x1),
	  area(0), convexclockwise(false)
{
	// being clockwise means that every angle between edges should be in [pi, 2pi]
	// (counterclockwise means angles in [0, pi] and convexity means that every angle has the
	// same "clockwiseness")
	// 
	// to compute the clockwiseness the cross product is used, which will be negative for clockwise
	// angles and positive for counterclockwise ones. Zero is the degenerate case of one vertices lying
	// in the line connecting other two and will be considered acceptable as clockwise
	// 
	// note that this cross product is conserved when swapping the edges with their normals
	
	convexclockwise = (nx1 * ny2 - nx2 * ny1 <= 0) &&
	                  (nx2 * ny3 - nx3 * ny2 <= 0) &&
	                  (nx3 * ny4 - nx4 * ny3 <= 0) &&
	                  (nx4 * ny1 - nx1 * ny4 <= 0);
	
	
	// break the (clockwise convex) Quad into two triangles and sum their areas
	// 
	// to find the area of the triangles, just halve the cross product between their edges
	// (its magnitude is the area of the parallelogram implied by the vectors)
	// 
	// note that this cross product is conserved when swapping the edges with their normals
	// 
	// also, since the Quad is clockwise convex, all the cross product are negatives, so
	// abs(ei x ek) = -(ei x ek)
	
	area = -(nx1 * ny2 - nx2 * ny1) + -(nx3 * ny4 - nx4 * ny3);
}

Quad Quad::Perspective(Mat homography) const
{
	double quad[12] = { X1,  X2,  X3,  X4,
	                    Y1,  Y2,  Y3,  Y4,
	                   1.0, 1.0, 1.0, 1.0};
	
	Mat quadmat(3, 4, CV_64F, quad);
	
	Mat     transformed = homography * quadmat;
	double* tf          = transformed.ptr<double>();
	
	return Quad(tf[0]/tf[ 8], tf[4]/tf[ 8],
	            tf[1]/tf[ 9], tf[5]/tf[ 9],
	            tf[2]/tf[10], tf[6]/tf[10],
	            tf[3]/tf[11], tf[7]/tf[11]);
}

bool Quad::Inside(double x, double y) const
{
	// the dot product with every edge normal should be non-negative
	return ((x - X1) * nx1 + (y - Y1) * ny1 >= 0) &&
	       ((x - X2) * nx2 + (y - Y2) * ny2 >= 0) &&
	       ((x - X3) * nx3 + (y - Y3) * ny3 >= 0) &&
	       ((x - X4) * nx4 + (y - Y4) * ny4 >= 0);
}

bool Quad::ConvexClockwise() const
{
	return convexclockwise;
}

double Quad::Area() const
{
	return area;
}

string Quad::ToString() const
{
	return "[(" + std::to_string(X1) + ", " + std::to_string(Y1) + "); "
	        "(" + std::to_string(X2) + ", " + std::to_string(Y2) + "); "
	        "(" + std::to_string(X3) + ", " + std::to_string(Y3) + "); "
	        "(" + std::to_string(X4) + ", " + std::to_string(Y4) + ")]";
}

}
