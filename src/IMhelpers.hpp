/// @file IMhelpers.hpp
/// @brief ImageMagick-based functions header file
/// 
/// ImageMagick is messy and includes many things into the global namespace
/// sometimes colliding with OpenCV calls and making their headers includes
/// sensitive to order. Instead of trying to maintain such a brittle configuration,
/// the Imagemagick-based functions have been consolidated in this file, which
/// will not leak the ImageMagick headers.
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

#ifndef IMHELPERS_HPP
#define IMHELPERS_HPP 1

#include <vector>
#include <string>

using std::string;

namespace MagickCore
{

/// @brief Size of each component in a pixel
/// @remarks Duplicate of MagickCore::StorageType, but on a different namespace
///          to avoid duplicate definitions, while maintaining compatibility
///          with including Magick++ or not. Since its an enum class, it has
///          its own namespace, so there's no type problem
enum class StorageType_
{
	UndefinedPixel,
	CharPixel,
	DoublePixel,
	FloatPixel,
	LongPixel,
	LongLongPixel,
	QuantumPixel,
	ShortPixel
};

}

namespace Magick
{

/// @brief Intialize ImageMagick (on Windows)
void InitializeMagick(const char* arg0);

/// @brief Opaque reference to ImageMagick image
class Image;

/// @brief Write an opaque ImageMagick image to a memory buffer
void imageWrite(Image* image, const int x, const int y, const int cols, const int rows,
                const std::string& map, MagickCore::StorageType_ type, void* pixels);

/// @brief Get the width of an opaque Imagemagick image
int imageWidth(Image* image);

/// @brief Get the height of an opaque Imagemagick image
int imageHeight(Image* image);

/// @brief Destruct an opaque ImageMagick image and free its resources
void imageDelete(Image* image);

}

namespace slidesync
{

/// @brief read a pdf file into ImageMagick images
/// @remarks Remember to delete all the images using the appropriate imageDelete().
/// 
/// @param[in] filename PDF slides filename.
/// @param[in] framewidth Width of a footage frame for size reference.
/// @param[in] frameheight Height of a footage frame for size reference.
std::vector<Magick::Image*> readpdf_im(const string& filename, int framewidth, int frameheight);

}

#endif
