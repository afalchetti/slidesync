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
	: X1(0), Y1(0), X2(0), Y2(0), X3(0), Y3(0), X4(0), Y4(0) {}

Quad::Quad(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
	: X1(x1), Y1(y1), X2(x2), Y2(y2), X3(x3), Y3(y3), X4(x4), Y4(y4) {}

Quad Quad::Perspective(Mat homography)
{
	double quad[12] = { X1,  X2,  X3,  X4,
	                    Y1,  Y2,  Y3,  Y4,
	                   1.0, 1.0, 1.0, 1.0};
	
	Mat quadmat(3, 4, CV_64F, &quad);
	
	Mat     transformed = homography * quadmat;
	double* tf          = transformed.ptr<double>();
	
	return Quad(tf[0]/tf[ 8], tf[4]/tf[ 8],
	            tf[1]/tf[ 9], tf[5]/tf[ 9],
	            tf[2]/tf[10], tf[6]/tf[10],
	            tf[3]/tf[11], tf[7]/tf[11]);
}

string Quad::ToString()
{
	return "[(" + std::to_string(X1) + ", " + std::to_string(Y1) + "); "
	        "(" + std::to_string(X2) + ", " + std::to_string(Y2) + "); "
	        "(" + std::to_string(X3) + ", " + std::to_string(Y3) + "); "
	        "(" + std::to_string(X4) + ", " + std::to_string(Y4) + ")]";
}

}
