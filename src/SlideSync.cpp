/// @file SlideSync.cpp
/// @brief Slide-Video Synchronizer
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
#include <memory>
#include <iostream>
#include <vector>
#include <list>

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
#include <wx/cmdline.h>
#include <wx/sizer.h>
#include <wx/dir.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "SlideSync.hpp"
#include "CVCanvas.hpp"
#include "ProcessLoop.hpp"
#include "SyncLoop.hpp"
#include "GenLoop.hpp"
#include "IMhelpers.hpp"
#include "util.hpp"

using std::string;
using cv::Mat;

using slidesync::SlideSyncApp;
using slidesync::SlideSyncWindow;

/// @brief Event signal routing table for SlideSyncWindow
wxBEGIN_EVENT_TABLE(SlideSyncWindow, wxFrame)
	EVT_MENU(wxID_EXIT, SlideSyncWindow::onexit)
	EVT_MENU(wxID_ABOUT, SlideSyncWindow::onabout)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(SlideSyncApp);


namespace slidesync
{

// SlideSyncWindow definitions

SlideSyncWindow::SlideSyncWindow(const wxString& title, const wxPoint& pos,
                                 std::unique_ptr<ProcessLoop>* processloop)
	: wxFrame(NULL, wxID_ANY, title, pos, wxDefaultSize), canvas(nullptr), processloop(processloop)
{
	wxMenu* menufile = new wxMenu();
	menufile->Append(wxID_EXIT);
	
	wxMenu* menuhelp = new wxMenu();
	menuhelp->Append(wxID_ABOUT);
	
	wxMenuBar* menubar = new wxMenuBar();
	menubar->Append(menufile, "&File");
	menubar->Append(menuhelp, "&Help");
	
	SetMenuBar(menubar);
	CreateStatusBar();
	SetStatusText("Initializing");
	
	wxGLAttributes canvas_attributes;
	canvas_attributes.PlatformDefaults().RGBA().DoubleBuffer().Depth(16).EndList();
	
	wxGLContextAttrs context_attributes;
	context_attributes.CoreProfile().OGLVersion(3, 0).EndList();
	
	canvas = new CVCanvas(this, wxID_ANY, canvas_attributes, context_attributes);
	
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(canvas, 1, wxEXPAND);
	
	SetSizer(sizer);
	SetAutoLayout(true);
}

void SlideSyncWindow::SetProcessLoop(std::unique_ptr<ProcessLoop>* processloop)
{
	this->processloop = processloop;
}

bool SlideSyncWindow::Destroy()
{
	(*processloop)->Stop();
	return wxFrame::Destroy();
}

void SlideSyncWindow::onexit(wxCommandEvent& event)
{
	Close(true);
}

void SlideSyncWindow::onabout(wxCommandEvent& event)
{
	wxMessageBox("SlideSync 0.1\nby Angelo Falchetti", "About SlideSync", wxOK | wxICON_INFORMATION);
}

// SlideSyncApp definitions

/// @brief read a pdf file into OpenCV matrices
/// 
/// @param[in] filename PDF slides filename.
/// @param[in] framewidth Width of a footage frame for size reference.
/// @param[in] frameheight Height of a footage frame for size reference.
/// @param[in] cache_directory Directory to find/save a cache for this conversion
std::vector<Mat> readpdf(string filename, int framewidth, int frameheight, string cache_directory)
{
	std::vector<Mat> slides;
	
	// read from cache if possible
	
	if (!wxDir::Exists(cache_directory)) {
		wxDir::Make(cache_directory);
	}
	
	wxDir cachedir(cache_directory);
	
	if (cachedir.IsOpened()) {
		wxArrayString files;
		
		wxDir::GetAllFiles(cache_directory, &files, "*.png");
		
		files.Sort(compare_lexiconumerical);
		
		if (files.GetCount() > 0) {
			for (unsigned int i = 0; i < files.GetCount(); i++) {
				Mat slide = cv::imread(files.Item(i).ToStdString(), CV_LOAD_IMAGE_GRAYSCALE);
				
				slides.push_back(slide);
			}
			
			return slides;
		}
	}
	
	// otherwise go to the source
	
	std::vector<Magick::Image*> slides_im = readpdf_im(filename, framewidth, frameheight);
	
	Magick::Image* first = *(slides_im.begin());
	
	int width  = imageWidth(first);
	int height = imageHeight(first);
	
	std::vector<unsigned char> buffer(4 * width * height);
	Mat cv_frame(height, width, CV_8UC4, &buffer[0]);
	
	for (unsigned int i = 0; i < slides_im.size(); i++) {
		Magick::Image* slide_im = slides_im[i];
		
		if (width  != imageWidth(slide_im) ||
		    height != imageHeight(slide_im)) {
			// inconsistent page size, not supported
			continue;
		}
		
		Magick::imageWrite(slide_im, 0, 0, width, height, "RGBA",
		                   MagickCore::StorageType_::CharPixel, &buffer[0]);
		
		// cv_frame is only a OpenCV wrapper around the buffer.
		// It doesn't own the memory (which will be freed at the end of this function);
		// clone() or cvtColor() is required to copy the buffer into a memory-owning OpenCV matrix
		
		Mat gray;
		
		cv::cvtColor(cv_frame, gray, cv::COLOR_RGBA2GRAY);
		slides.push_back(gray);
	}
	
	for (unsigned int i = 0; i < slides_im.size(); i++) {
		Magick::imageDelete(slides_im[i]);
	}
	
	// only save if the cachedir was opened successfully
	
	if (cachedir.IsOpened()) {
		for (unsigned int i = 0; i < slides.size(); i++) {
			cv::imwrite(cache_directory + std::pathsep + "slide-" +
			            std::to_string(i + 1) + ".png", slides[i]);
		}
	}
	
	return slides;
}

bool SlideSyncApp::OnInit()
{
	wxApp::OnInit();
	
	std::cout << "Initializing..." << std::endl;
	appstate = SyncAppState::Initializing;
	
	std::cout << "Reading footage file '" << videofname << "'" << std::endl;
	footage.open(videofname, cv::CAP_ANY);
	
	if (!footage.isOpened()) {
		std::cerr << "Can't open footage video file" << std::endl;
		return false;
	}
	
	unsigned int width  = (unsigned int) footage.get(cv::CAP_PROP_FRAME_WIDTH);
	unsigned int height = (unsigned int) footage.get(cv::CAP_PROP_FRAME_HEIGHT);
	
	Magick::InitializeMagick(argv[0]);
	
	std::cout << "Reading PDF slides file '" << slidesfname << "'" << std::endl;
	slides = readpdf(slidesfname, width, height, intermediatedir + std::pathsep + "slides");
	std::cout << "PDF reading complete" << std::endl;
	
	window = new SlideSyncWindow("SlideSync", wxDefaultPosition, nullptr);
	
	window->SetClientSize(width, height);
	window->Show(true);
	
	window->canvas->Initialize(width, height);
	
	processloop = std::unique_ptr<ProcessLoop>(new SyncLoop(window->canvas, &footage, &slides,
	                                                        intermediatedir + std::pathsep + "raw.sync"));
	window->SetProcessLoop(&processloop);
	
	processloop->Bind(LoopFinishedEvent, &SlideSyncApp::OnSyncFinished, this);
	
	appstate = SyncAppState::Synchronizing;
	window->SetStatusText("Synchronizing");
	
	std::cout << "Synchronizing..." << std::endl;
	
	processloop->Start(40);
	
	return true;
}

void SlideSyncApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.AddLongOption("footage", "Input recording of the presentation", wxCMD_LINE_VAL_STRING, wxCMD_LINE_SPLIT_UNIX);
	parser.AddLongOption("slides",  "Input presentation slides file",      wxCMD_LINE_VAL_STRING, wxCMD_LINE_SPLIT_UNIX);
	parser.AddLongOption("sync",    "Output synchronization file",         wxCMD_LINE_VAL_STRING, wxCMD_LINE_SPLIT_UNIX);
	parser.AddLongOption("output",  "Output synchronized video file",      wxCMD_LINE_VAL_STRING, wxCMD_LINE_SPLIT_UNIX);
}

bool SlideSyncApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	wxString footage;
	wxString slides;
	wxString sync;
	wxString output;
	
	if (!parser.Found("footage", &footage) ||
	    !parser.Found("slides", &slides) ||
	    !parser.Found("sync", &sync) ||
	    !parser.Found("output", &output)) {
		return false;
	}
	
	videofname      = footage;
	slidesfname     = slides;
	outsyncfname    = sync;
	outvideofname   = output;
	intermediatedir = videofname + ".d";
	
	return true;
}

void SlideSyncApp::OnSyncFinished(wxEvent& event)
{
	processloop->Stop();
	processloop.reset(new GenLoop());
	processloop->Bind(LoopFinishedEvent, &SlideSyncApp::OnGenFinished, this);
	
	appstate = SyncAppState::GeneratingVideo;
	window->SetStatusText("Generating video");
	
	std::cout << "Generating video..." << std::endl;
}

void SlideSyncApp::OnGenFinished(wxEvent& event)
{
	processloop->Stop();
}

}

