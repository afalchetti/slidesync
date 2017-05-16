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
#include "Loops.hpp"
#include "IMhelpers.hpp"

using std::string;
using cv::Mat;

namespace std
{

/// @brief Folder separator in paths
#if defined(_WIN32) || defined(__CYGWIN__)
const char pathsep = '\\';
#else
const char pathsep = '/';
#endif

}

namespace slidesync
{

/// @brief Main synchronization window
class SlideSyncWindow : public wxFrame
{
public:
	/// @brief OpenGL canvas observer reference
	CVCanvas* canvas;
	
private:
	/// @brief Observer reference to the application's processing loop
	std::unique_ptr<ProcessLoop>* processloop;
	
public:
	/// @brief Construct a SlideSyncWindow with standard parameters
	SlideSyncWindow(const wxString& title, const wxPoint& pos, std::unique_ptr<ProcessLoop>* processloop);
	
	/// @brief Set the internal processing loop observer
	void SetProcessLoop(std::unique_ptr<ProcessLoop>* processloop);
	
private:
	/// @brief Close the application on exit
	void OnExit(wxCommandEvent& event);
	
	/// @brief Show about dialog box
	void OnAbout(wxCommandEvent& event);
	
protected:
	/// @brief Declaration of event signal routing table for SlideSyncWindow
	wxDECLARE_EVENT_TABLE();
};

/// @brief Main application
class SlideSyncApp : public wxApp
{
private:
	/// @brief Stage in the process
	SyncAppState appstate;
	
	/// @brief Filename of the recording footage
	string videofname;
	
	/// @brief Filename of the presentation slides
	string slidesfname;
	
	/// @brief Filename for the synchronization data that will be output
	string outsyncfname;
	
	/// @brief Filename for the video data that will be output
	string outvideofname;
	
	/// @brief Directory path for files contaning intermediate and cached results
	string intermediatedir;
	
	/// @brief Captured video of the presentation
	cv::VideoCapture footage;
	
	/// @brief Presentation slides
	std::vector<Mat> slides;
	
	/// @brief Recurrent event based processing loop
	std::unique_ptr<ProcessLoop> processloop;
	
public:
	/// @brief Main entry point
	virtual bool OnInit();
	
	/// @brief Configure command line parser
	/// 
	/// @param[in] parser Command line parser.
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	
	/// @brief Process command line arguments
	/// 
	/// @param[in] parser Command line parser.
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
};

}

using slidesync::SlideSyncApp;
using slidesync::SlideSyncWindow;

/// @brief Event signal routing table for SlideSyncWindow
wxBEGIN_EVENT_TABLE(SlideSyncWindow, wxFrame)
	EVT_MENU(wxID_EXIT, SlideSyncWindow::OnExit)
	EVT_MENU(wxID_ABOUT, SlideSyncWindow::OnAbout)
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

void SlideSyncWindow::OnExit(wxCommandEvent& event)
{
	Close(true);
}

void SlideSyncWindow::OnAbout(wxCommandEvent& event)
{
	wxMessageBox("SlideSync 0.1\nby Angelo Falchetti", "About SlideSync", wxOK | wxICON_INFORMATION);
}

// SlideSyncApp definitions

/// @brief True if the (possibly wide) character c is an ASCII number
bool isnumeric(int c)
{
	return '0' <= c && c <= '9';
}

/// @brief Compare strings lexicographically, but considering numbers as
///        indivisible units, so "a" < "b", "1" < "2" and "frame-5" < "frame-23"
/// 
/// @param[in] a First string
/// @param[in] b Second string
/// @returns Zero if a == b; < 0 if a < b; > 0 if a > b
int compare_lexiconumerical(const wxString& a, const wxString& b)
{
	unsigned int i;
	unsigned int k;
	int          diff;
	
	for (i = 0, k = 0; i < a.length() && k < b.length(); i++, k++) {
		if (isnumeric(a[i]) && isnumeric(b[k])) {
			unsigned int p;
			unsigned int q;
			
			// p and q will point to the end of the number
			for (p = i + 1; p < a.length() && isnumeric(a[p]); p++) {}
			for (q = k + 1; q < b.length() && isnumeric(b[q]); q++) {}
			
			// char lengths of the numbers
			int alen = p - i;
			int blen = q - k;
			
			if (alen != blen) {
				return alen - blen;
			}
			
			// the numbers have the same length, they can be compared lexicographically
			for (; i < p; i++, k++) {
				diff = (int) a[i] - (int) b[k];
				
				if (diff != 0) {
					return diff;
				}
			}
			
			i = p - 1;
			k = q - 1;
		}
		else {
			diff = (int) a[i] - (int) b[k];
			
			if (diff != 0) {
				return diff;
			}
		}
	}
	
	int aremaining = a.length() - i;
	int bremaining = b.length() - k;
	
	return aremaining - bremaining;
}

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
	
	SlideSyncWindow* window = new SlideSyncWindow("SlideSync", wxDefaultPosition, nullptr);
	
	window->SetClientSize(width, height);
	window->Show(true);
	
	window->canvas->Initialize(width, height);
	
	processloop = std::unique_ptr<ProcessLoop>(new SyncLoop(window->canvas, &footage, &slides));
	window->SetProcessLoop(&processloop);
	
	appstate = SyncAppState::Synchronizing;
	window->SetStatusText("Synchronizing");
	
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

}

