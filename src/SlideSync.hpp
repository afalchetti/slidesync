/// @file SlideSync.hpp
/// @brief Slide-video synchronizer header file
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

#ifndef SLIDESYNC_HPP
#define SLIDESYNC_HPP 1

#include <string>
#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>

#include <wx/wxprec.h>
 
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "CVCanvas.hpp"
#include "ProcessLoop.hpp"

using std::string;
using cv::Mat;

namespace slidesync
{

/// @brief Application state. Stage in the process
enum class SyncAppState
{
	Initializing,
	Synchronizing,
	GeneratingVideo
};

/// @brief Custom event identification numbers
enum class EventID
{
	SlideSyncID
};

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
	
	/// @brief Destroy the window and stop the timer
	virtual bool Destroy() override;
	
private:
	/// @brief Close the application on exit
	void onexit(wxCommandEvent& event);
	
	/// @brief Show about dialog box
	void onabout(wxCommandEvent& event);
	
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
	
	/// @brief Main window
	SlideSyncWindow* window;
	
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
	
	/// @brief Event handler for the LoopFinished event raised by the synchronization loop
	void OnSyncFinished(wxEvent& event);
	
	/// @brief Event handler for the LoopFinished event raised by the video generation loop
	void OnGenFinished(wxEvent& event);
};

}

#endif
