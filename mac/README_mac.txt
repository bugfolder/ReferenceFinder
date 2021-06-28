BUILDING and INSTALLING ReferenceFinder 4 for Macintosh OS X
  by Robert J. Lang

1. INTRODUCTION

This file describes basic procedures for building from source, installing and
running ReferenceFinder 4.0 for Macintosh (RF4). It is not needed for
installing and using the ReferenceFinder application.

This and all other files in this directory are subject to the same license
conditions mentioned in the README.txt file, here referred to as the main
README.

Macintosh is a registered trademark of Apple Computer, Inc.

2. BEFORE YOU START

Requisites for building RF4:
- ReferenceFinder 4.0 requisites (see main README)
- Macintosh OS X Version 10.4.2 or higher
- Apple Developer Tools, including Xcode 2.2 or higher
- wxWidgets (wxMac version), as described below

RF4 was programmed and tested on a 500 MHz PowerBook G4 with 512 MB RAM. You
should have 1.2 GB of disk space available to build all targets.

3. ReferenceFinder CONFIGURATIONS

RF4 may be built in three configurations, which are set up as project build
configurations within the ReferenceFinder.xcodeproj file.

- configuration Deployment: production version
- configuration Development: partial logging and error reporting
- configuration Debug: full logging and error reporting

4. BUILDING AND INSTALLING RF4 FROM SOURCE

4.1. Overview

ReferenceFinder/mac comes with several scripts to automate the build of
ReferenceFinder. Any of the scripts supplied in ReferenceFinder/mac should be
run from the ReferenceFinder/mac directory. 

4.2. Compiling wxWidgets

ReferenceFinder 4.0's user interface uses the multiplatform open-source library
wxWidgets. For maximum portability, RF4 is statically linked. Mac OS X 10.4.2
(Tiger) comes with version 2.5 of wxWidgets preinstalled. RF4 requires
wxWidgets 2.7.2 or higher, so you must build your own version and not use the
preinstalled version.

ReferenceFinder comes with Terminal scripts for setting up and building
wxWidgets.

  checkout-wxw -- downloads the wxMac and wxEtc modules of wxWidgets from the
  CVS repository to the directory build/wxMac-2.7. You can also obtain
  wxWidgets on CD-ROM from the wxWidgets web site, www.wxwidgets.org or by
  downloading it directly from the wxWidgets web site. If you obtain wxWidgets
  by some other means, you should rename and move the wxWidgets folder to
  build/wxMac-2.7, because all the scripts assume this name and location.

  rebuild-wxw -- configures and builds the release static libraries for
  wxWidgets, which are used by the ReferenceFinder Xcode project.

4.3. Building ReferenceFinder

Once you have built the wxWidgets libraries, you are ready to build
ReferenceFinder. You can build ReferenceFinder 4 interactively using Apple's
Xcode IDE, or build it from Terminal using scripts (which call xcodebuild, the
command-line version of Xcode).

The file ReferenceFinder.xcodeproj is an Xcode project that you can open with
Apple's Xcode IDE to build any configuration of any target and/or to
interactively debug ReferenceFinder. You can also perform the entire build
process, or portions thereof, by running the following scripts in Terminal.

  build-rf-help -- builds the ReferenceFinder help archive. This should be run
  after any changes to any of the help files. You MUST run this script before
  building ReferenceFinder whether you are using Xcode or xcodebuild. It is
  automatically run as part of the next two scripts.

  build-rf-all -- builds all configurations of all executables and test
  programs. If you supply an argument (e.g., "./build-rf-all 20061118") the
  second argument will replace the build number in file RFVersion.h.

  build-rf-pkg -- build the disk image for the binary distribution.

A complete build of ReferenceFinder will consume about 1.2 GB of disk space, of
which about 600MB is taken up by the wxWidgets libraries and about 600 MB is
taken up by ReferenceFinder's object files.

The executables are all built within individual directories within the build
directory, namely build/Debug, build/Development, and build/Deployment. When a
build is complete, you can drag the application out of its build directory to
your preferred final location. The build/ReferenceFinder directory contains the
files of the binary distribution. The file ReferenceFinder.dmg is a compressed
disk image of the binary distribution.

4.4 Installing ReferenceFinder

After building, you can drag the build/Deployment/ReferenceFinder application to
your Applications folder and, if you wish, delete the entire mac/build
directory (thereby reclaiming that 1.2 GB of disk space). There are no other
files in the ReferenceFinder installation; all documentation is stored within
the ReferenceFinder application. The first time ReferenceFinder is run, it will
create a preferences file at ~/Library/Preferences/ReferenceFinder 4
Preferences.

4.5. Uninstalling ReferenceFinder

To uninstall ReferenceFinder, drag the ReferenceFinder application to the Trash
and empty the Trash. You can also delete the ReferenceFinder 4 Preferences
file, which is found at ~/Library/Preferences/ReferenceFinder 4 Preferences.
