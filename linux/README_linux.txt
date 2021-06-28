BUILDING and INSTALLING ReferenceFinder 4.0 for Linux
  by Carlos A. Furuti

1. INTRODUCTION

This file describes basic procedures about building from source,
installing and running ReferenceFinder 4.0 for Linux (RF4L for
short). It is also valid for most Unix-like environments with GNU
tools. It is not needed for installing and using a RF5L binary bundle.

This and all other files in this directory are subject to the same
license conditions mentioned in the Source/README.txt file, here
referred to as the main README.

Linux is a registered trademark of Linus Torvalds.

2. BEFORE YOU START

Requisites for building RF4L:
- ReferenceFinder 4.0 requisites (see main README)
- a reasonably recent Linux platform with an installed X11 subsystem
  and its development libraries
- GCC 4.0.0 or better, with C++ libraries
- the GTK2 library
- GNU Make or compatible
- wxWidgets (RF4L was tested with the wxGTK version), as described below

RF4L was mainly ported and tested on a Fedora Core 4 distribution,
running on a i386-compatible system. Figures for disk space requisites
below are valid for this platform only.

3. BUILDING AND INSTALLING RF4L FROM SOURCE

3.1. Compiling wxWidgets

ReferenceFinder 4.0's user interface uses the multiplatform open-source
library wxWidgets. See the "wxwidgets" subdirectory about properly
building the library, _even_ if you already have wxWidgets installed.

3.2. Configuring your Build

After wxWidgets is ready, edit the Makefile in this directory and, if
necessary, change the few initial variables:

- WXHOME is the wxWidgets source directory (it should contain at least
  one of the build-debug and build-release subdirectories)
- PROGRAM is the name the user will type for invoking RF4L's GUI version.
  There's also a command-line version of RF4L, but at present you cannot
  change its name, it's always "referencefinder"
- INSTALL_PREFIX is the directory prefix for installation. Usually it
  is /usr/local, meaning RF4L's binary program will be installed in
  /usr/local/bin, and its auxiliary files somewhere in /usr/local/share.
  Auxiliary files may be shared by all configuration types.

3.3. Building ReferenceFinder

After configuring the Makefile, just type "make". Only while building
the RF4L program and auxiliary files, a "build" subdirectory will be
created, requiring additional disk space (about 6MB).

3.4 Installing ReferenceFinder

After building RF4L, repeat the "make" command with an "install" 
target (e.g., type "make install") for installing RF4L in the proper
directories. You'll need write permission for those directories and
additional (permanent this time) disk space (roughly 7MB)

3.5. Building a Distribution Bundle

After building RF4L, you may optionally create a binary distribution
bundle, i.e., a single-file, self-contained, self-extracting shell 
script able to install RF4L in any platform compatible with your own
(it will present a graphic interface if Tcl/Tk is available).
This step is not needed if you installed RF4L in your own system as 
described above.

Build a bundle typing "make" with a target "dist" (e.g., "make dist").
Only while building a distribution bundle, additional disk space is
required beyond that needed when building the program, roughly 8MB.

3.6. Cleaning the Build Directories

After installing RF4L (and/or backing up its distribution bundle) you
may run "make" (with the appropriate configuration) and the "clean"
target to remove temporary files while building RF4L, or more simply
just remove the whole "build" subdirectory.

After testing the installed application, you may of course remove the
whole source directory too.

4. RUNNING RF4L

4.1. If You Chose the Default Settings

Just type the application name configured in your Makefile (like
"ReferenceFinder"), adding a pathname if needed. Add "-h" to see the
available command-line options.

4.2. A Note on Installation Prefixes

Apparently, wxWidgets's machinery for standard paths on Linux assumes
all auxiliary files are in a "shared" directory. RF4L defines a
subdirectory of "shared" where its auxiliary files are stored. If for
some reason you couldn't install RF4L on a standard prefix like /usr,
/opt or /usr/local, you can set it at runtime with the "-d" option or
the environment variable REFERENCEFINDER_PREFIX. Note that you can't
specify the exact subdirectory, only the prefix.

5. UNINSTALLING RF4L

There is yet no smart procedure for uninstalling RF4L, but a small
"uninstall" script is installed together with RF4L's auxiliary files.
Just run it.

