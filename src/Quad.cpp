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
	: x1(0), y1(0), x2(0), y2(0), x3(0), y3(0), x4(0), y4(0),
	  nx1(0), ny1(0), nx2(0), ny2(0), nx3(0), ny3(0), nx4(0), ny4(0),
	  area(0), convexclockwise(true) {}

Quad::Quad(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
	: x1(x1), y1(y1), x2(x2), y2(y2), x3(x3), y3(y3), x4(x4), y4(y4),
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

double Quad::X1() const
{
	return x1;
}

double Quad::Y1() const
{
	return y1;
}

double Quad::X2() const
{
	return x2;
}

double Quad::Y2() const
{
	return y2;
}

double Quad::X3() const
{
	return x3;
}

double Quad::Y3() const
{
	return y3;
}

double Quad::X4() const
{
	return x4;
}

double Quad::Y4() const
{
	return y4;
}

Quad Quad::Perspective(Mat homography) const
{
	double quad[12] = { x1,  x2,  x3,  x4,
	                    y1,  y2,  y3,  y4,
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
	return ((x - x1) * nx1 + (y - y1) * ny1 >= 0) &&
	       ((x - x2) * nx2 + (y - y2) * ny2 >= 0) &&
	       ((x - x3) * nx3 + (y - y3) * ny3 >= 0) &&
	       ((x - x4) * nx4 + (y - y4) * ny4 >= 0);
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
	return "[(" + std::to_string(x1) + ", " + std::to_string(y1) + "); "
	        "(" + std::to_string(x2) + ", " + std::to_string(y2) + "); "
	        "(" + std::to_string(x3) + ", " + std::to_string(y3) + "); "
	        "(" + std::to_string(x4) + ", " + std::to_string(y4) + ")]";
}

}
