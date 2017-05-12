/// @file CVCanvas.cpp
/// @brief OpenCV + OpenGL canvas
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

#include "CVCanvas.hpp"

using std::string;
using cv::Mat;
using slidesync::CVCanvas;

/// @brief Event signal routing table for CVCanvas
wxBEGIN_EVENT_TABLE(CVCanvas, wxGLCanvas)
	EVT_PAINT(CVCanvas::render)
wxEND_EVENT_TABLE()

namespace slidesync
{

Mat CVCanvas::Frame()
{
	return frame;
}

void CVCanvas::SetFrame(const Mat& frame)
{
	this->frame = frame;
}

CVCanvas::CVCanvas(wxFrame* parent, wxWindowID id, wxGLAttributes canvas_attributes,
                   wxGLContextAttrs context_attributes)
	: wxGLCanvas(parent, canvas_attributes, id), frame(), width(0), height(0), textureID(0),
	  context(new wxGLContext(this, nullptr, &context_attributes))
{
	
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

void CVCanvas::Initialize(int width, int height)
{
	this->width  = width;
	this->height = height;
	
	SetCurrent(*context);
	
	frame.create(height, width, CV_8UC4);
	frame = cv::Vec4b(0, 0, 0, 255);
	
	glEnable(GL_TEXTURE_2D);
	
	GLuint textures[1];
	glGenTextures(1, textures);
	textureID = textures[0];
	
	glBindTexture(GL_TEXTURE_2D, textureID);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0); 
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.data);
}

bool CVCanvas::UpdateGL()
{
	SetCurrent(*context);
	
	if (frame.cols != width || frame.rows != height || frame.type() != CV_8UC4) {
		return false;
	}
	
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, frame.data);
	Refresh();
	Update();
	
	return true;
}

bool CVCanvas::UpdateGL(const Mat& frame)
{
	this->frame = frame;
	return UpdateGL();
}

void CVCanvas::prepare_viewport(int left, int top, int right, int bottom)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glViewport(left, top, right - left, bottom - top);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	gluOrtho2D(left, right, bottom, top);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glBindTexture(GL_TEXTURE_2D, textureID);
}

void CVCanvas::render(wxPaintEvent& evt)
{
    if(!IsShown()) {
		return;
	}
	
	SetCurrent(*context);
	wxPaintDC(this);
	
	wxSize size = GetSize();
	
	prepare_viewport(0, 0, size.x, size.y);
	
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(0,      0);
	glTexCoord2f(0, 1); glVertex2f(0,      size.y);
	glTexCoord2f(1, 1); glVertex2f(size.x, size.y);
	glTexCoord2f(1, 0); glVertex2f(size.x, 0);
	glEnd();
	
	SwapBuffers();
}

}

