/*******************************************************************************
ReferenceFinder 4.0

A Program for Origami Design

Contact:
  web: http://www.langorigami.com/reffinder/reffinder.php4
  email: reffinder@langorigami.com
  
Developers:
  Robert J. Lang (lead)
  Carlos Furuti (Linux port)
  Wlodzimierz 'ABX' Skiba (MSW port)
  
Special thanks:
  Stefan Csomer (wxMac guru)
  The wxWidgets Team

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program (in a file called LICENSE.txt); if not, go to
http://www.gnu.org/copyleft/gpl.html or write to 

  Free Software Foundation, Inc.
  59 Temple Place - Suite 330
  Boston, MA 02111-1307 USA

Source code to this program is always available; for more information visit my
website at:

  http://www.langorigami.com

/*******************************************************************************
Information for Programmers

ReferenceFinder consists of two projects that can be built individually or
together. Source code and headers for all projects is contained within the
Source directory. ReferenceFinder can be built as a command-line tool or as the
full application with GUI. All the code to build the model is in the included
files. However, you will need to download and build the wxWidgets libraries
(www.wxwidgets.org) in order to build the ReferenceFinder application with GUI.

source contains the following subdirectories:

source/model contains the complete ReferenceFinder mathematical model, no UI
code. 

source/console contains a simple console implementation of
ReferenceFinder, no GUI.

source/gui contains the GUI C++ classes, which requires wxWidgets.

source/images contains the GUI splash screen.

source/help contains HTML documentation for the GUI.

source/about contains the HTML about box for the GUI.

********************************************************************************
To build ReferenceFinder:

Detailed build instructions for Macintosh, Linux, and Windows are given within
the platform-specific directories (mac, linux, and msw, respectively). Your
best (and easiest) bet is to use one of the build systems we've already set up.

See the wxWidgets home page (www.wxwidgets.org) for more details on building
wxWidgets or see the platform-specific README files for more information.

Once you've built the wxWidgets libraries, create a new wxWidgets project and
add the ReferenceFinder source code to it. How you do this depends on what
development environment you are using (e.g., makefiles vs. IDE).

You will also need to build the help archive (help.zip). Essentially, this
consists of putting the contents of the source/help folder into a ZIP archive.
The location of these two files is, in general, platform-dependent.

You will also need to copy some contents of the images, resources, and about
folder to locations that are also platform-dependent. If you're using a
different build system from what we've set up, examine the code for function
RFApp::OnInit() and the wxWidgets documentation to figure out the appropriate
locations for your system.

********************************************************************************
Supported Platforms and environments

ReferenceFinder has been successfully built in the following environments:

Mac OS X -- Xcode 2.4 (Apple developer tools). Use ReferenceFinder.xcodeproj as
your project file. Note that the wxWidget libraries must be built with
Mac-specific options; for simplicity, use the script files in the mac
subdirectory. Check mac/README_mac.txt for further details about building.

Linux -- i386, GCC 4.0.0, GTK+-2.0. Instructions for compiling wxWidget are 
included.

Windows -- Visual Studio Express Edition 2005. Visual Studio 6 won't work
without modifying the code to accommodate the compiler's poor standards
compliance. Check msw/README_msw.txt for further details about building.

The mathematical model is contained entirely within the model folder, if you'd
like to make your own user interface. The console interface
(ReferenceFinder_console.cpp) provides a minimal example of how you interact
with the model.

*******************************************************************************/
