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

#include "Quad.hpp"

namespace slidesync
{

Quad::Quad()
	: X1(0), Y1(0), X2(0), Y2(0), X3(0), Y3(0), X4(0), Y4(0) {}

Quad::Quad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	: X1(x1), Y1(y1), X2(x2), Y2(y2), X3(x3), Y3(y3), X4(x4), Y4(y4) {}

}
