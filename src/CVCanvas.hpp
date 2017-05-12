/// @file CVCanvas.hpp
/// @brief OpenCV + OpenGL canvas header file
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

#ifndef CVCANVAS_HPP
#define CVCANVAS_HPP 1

#include <memory>

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
#include <wx/cmdline.h>
#include <wx/glcanvas.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif
 
#ifdef __WXMAC__
#include "OpenGL/glu.h"
#include "OpenGL/gl.h"
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif

using std::string;
using cv::Mat;

namespace slidesync
{

/// @brief OpenCV + OpenGL rendered inside wxWidgets
class CVCanvas : public wxGLCanvas
{
private:
	/// @brief Frame as an OpenCV matrix structure
	Mat frame;
	
	/// @brief Frame width
	int width;
	
	/// @brief Frame height
	int height;
	
	/// @brief OpenGL texture ID
	GLuint textureID;
	
	/// @brief OpenGL context
	std::unique_ptr<wxGLContext> context;
	
public:
	/// @brief Get a copy of the internal frame
	Mat Frame();
	
	/// @brief Set the internal frame to a given value
	void SetFrame(const Mat& frame);
	
	/// @brief Construct a CVCanvas
	///
	/// @param[in] parent Parent view element.
	/// @param[in] id View element ID.
	/// @param[in] canvas_attributes Arguments for the wxGLCanvas construction.
	/// @param[in] context_attributes Arguments for the wxGLContext construction.
	CVCanvas(wxFrame* parent, wxWindowID id, wxGLAttributes canvas_attributes,
	         wxGLContextAttrs context_attributes);
	
	/// @brief Initialize the OpenGL resources
	/// 
	/// @param[in] width Frame width.
	/// @param[in] height Frame height.
	void Initialize(int width, int height);
	
	/// @brief Update the OpenGL texture to display the current OpenCV frame
	/// 
	/// @returns True if successful, false otherwise. The operation will fail if the current frame
	///          has a different size than specified at initialization or if it is not 8-bit RGBA.
	bool UpdateGL();
	
	/// @brief Update the OpenCV frame and update the OpenGL texture to display it
	/// 
	/// @returns True if successful, false otherwise. The operation will fail if the frame
	///          has a different size than specified at initialization or if it is not 8-bit RGBA.
	bool UpdateGL(const Mat& frame);
	
private:
	/// @brief Prepare the OpenGL context for rendering
	void prepare_viewport(int left, int top, int right, int bottom);
	
	/// @brief Render the OpenGL texture to the screen
	void render(wxPaintEvent& evt);

protected:
	/// @brief Declaration of event signal routing table for CVCanvas
	wxDECLARE_EVENT_TABLE();
};

}

#endif
