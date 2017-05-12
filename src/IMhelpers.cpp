/// @file IMhelpers.cpp
/// @brief ImageMagick-based functions
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

#include <vector>

#include <Magick++.h>

#include "IMhelpers.hpp"

using std::string;

namespace Magick
{

void imageWrite(Image* image, const int x, const int y, const int cols, const int rows,
                const std::string& map, MagickCore::StorageType_ type, void* pixels)
{
	image->write(x, y, cols, rows, map, (MagickCore::StorageType) type, pixels);
}

int imageWidth(Image* image)
{
	return image->size().width();
}

int imageHeight(Image* image)
{
	return image->size().height();
}

void imageDelete(Image* image)
{
	delete image;
}

}

namespace slidesync
{

std::vector<Magick::Image*> readpdf_im(string filename, int framewidth, int frameheight)
{
	std::list<Magick::Image>    slides_list;
	std::vector<Magick::Image*> slides;
	
	// reading pdf with an appropriate resolution
	// 
	// options.density controls the quality of the result.
	// To have an appropiately antialised image, density should
	// be 2x or 4x the "normal" density (where normal is proportional to size)
	// but magick++ doesn't seem to give any way to read the
	// original density directly from file, so it's not
	// easy to make calculations with it.
	// 
	// To solve this, an artificial density will be used
	// to decode the first pdf page to obtain its corresponding
	// size, which allows solving for the page size and therefore
	// the resolution (aka density).
	// 
	// Ideally, the metadata should be available, or at least, there should
	// be a version of ping which accepts something akin to ReadOptions.
	// Sadly, the API and documentation leave a lot to be desired so
	// the page will have to be fully decoded to be able to specify a density.
	// If the PDF is ill-formed this function could explode both in running
	// time and memory, i.e. if the page is 100 meters x 100 meters, using
	// a 50 dpc will not make the hardware happy; or be uselessly small
	// if the original density was huge in comparison.
	// Workaround: fix your document to reasonable settings.
	Magick::ReadOptions      pdfoptions;
	std::list<Magick::Image> for_metadata;
	const int                testresolution = 50;
	
	pdfoptions.density(Magick::Geometry(testresolution, testresolution));
	
	Magick::readImages(&for_metadata, filename + "[0]", pdfoptions);
	Magick::Image first = *(for_metadata.begin());
	
	int testwidth  = first.size().width();
	int testheight = first.size().height();
	
	if (testwidth < 4 || testheight < 4) {  // ill-formed file (huge resolution), not supported
		return slides;
	}
	
	double resolution = 0;
	
	// fit the slides bounding box to the frame
	if (((double) framewidth) / testwidth < ((double) frameheight) / testheight) {
		// pagewidth = width / resolution is constant and then solve for resolution
		resolution = ((double) testresolution) * framewidth / testwidth;
	}
	else {
		resolution = ((double) testresolution) * frameheight / testheight;
	}
	
	const int antialias = 4;
	pdfoptions.density(Magick::Geometry(antialias * resolution, antialias * resolution));
	
	Magick::readImages(&slides_list, filename, pdfoptions);
	
	// turn the list into another one (vector for efficiency) of pointers instead
	// images, which are opaque to the user of this function, allowing them
	// to use this function without including the ImageMagick headers
	for (auto slide = slides_list.begin(); slide != slides_list.end(); slide++) {
		slide->resize(Magick::Geometry(framewidth, frameheight));
		
		slides.push_back(new Magick::Image(*slide));
	}
	
	return slides;
}

}
