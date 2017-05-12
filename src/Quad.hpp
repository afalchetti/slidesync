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

namespace slidesync
{

/// @brief Two-dimensional polygon with four vertices
struct Quad
{
public:
	/// @brief First vertex's X-coordinate
	int X1;
	
	/// @brief First vertex's Y-coordinate
	int Y1;
	
	/// @brief Second vertex's X-coordinate
	int X2;
	
	/// @brief Second vertex's Y-coordinate
	int Y2;
	
	/// @brief Third vertex's X-coordinate
	int X3;
	
	/// @brief Third vertex's Y-coordinate
	int Y3;
	
	/// @brief Fourth vertex's X-coordinate
	int X4;
	
	/// @brief Fourth vertex's Y-coordinate
	int Y4;
	
public:
	/// @brief Construct a quad with every component set to zero
	Quad();
	
	/// @brief Construct a new from its vertices' coordinates
	Quad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
};

}

#endif
