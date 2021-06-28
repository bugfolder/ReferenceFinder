BUILDING and INSTALLING ReferenceFinder 4 for Microsoft Windows XP
  by Robert J. Lang

1. INTRODUCTION

This file describes basic procedures for building from source, installing and
running ReferenceFinder 4.0 for Windows (RF4). It is not needed for simply
installing and using the ReferenceFinder application.

This and all other files in this directory are subject to the same license
conditions mentioned in the README.txt file, here referred to as the main
README.

ReferenceFinder 4 builds under Visual Studio Express Edition 2005. It will not
build under Visual Studio 6 because VS6 doesn't properly support the standard
libraries. I don't know about 7 (aka VS 2003).

In order to build RF4 with VSEE2005 from this distribution, you'll have to do
the following.

You will need to move a copy of the source directory into the msw directory
because the VS projects expect it to be there.

Create an empty msw/build directory; all build products are placed there. (That
makes it easy for me to create the source distribution (everything in the msw
folder except the build folder).)

Download wxMSW-2.7.x, and move/rename the wxWidgets directory to
msw/build/wxMSW-2.7.

At this point, you should have:

msw/
  source/
    ...all the source code
  build/
    wxMSW-2.7/
      ...all the wxWidgets

Now build wxWidgets. Open msw/boud/wxMSW-2.7/build/msw/wx.dsw in VS. VS will
ask if you want to convert the project to the new format; say yes (which will
create a bunch of .vcproj files and wx.sln, which is what you will use in the
future.) 

ReferenceFinder 4 under MSW is statically linked to the C/C++ runtime libraries,
because the dynamic libraries may not be available on all systems. But that
means you need to build wxWidgets for static linking, so in the wx.sln solution,
change all projects to use the statically-linked libraries (options
/MTd, /MT) rather than the current settings of /MD and /MDd.

You can do this with search-and-replace by acting on the build/msw directory;
search over all of the .vcproj files, and make the substitutions

RuntimeLibrary="3"   to    RuntimeLibrary="1"
RuntimeLibrary="2"   to    RuntimeLibrary="0"

This changes all of the configurations, but you'll only need two of them.

Build all of the Debug|Win32 and Release|Win32 projects (which you can
do with Batch Build...).

Now it's time to build ReferenceFinder. Open ReferenceFinder.sln in VS. it
contains two projects: ReferenceFinder, which is the GUI application, and
reffinder4, which is the console version. It's a good idea to build the console
version first, because it's simpler, just to verify that things are working so
far.

Build the Debug|Win32 and Release|Win32 configurations.

If so far so good, you can build the GUI. Build the ReferenceFinder projects,
again, Debug|Win32 and Release|Win32 configurations.

Build the help archive, by zipping the contents of the source/help/ folder
to a file help.zip.

Copy the following files into msw/build/
    help.zip
    source/about/about.htm
    source/images/SplashScreen.png

This is necessary to run the Debug version from within VS. To run a release
version, you'll have to put copies of these same four files in the same
directory as the ReferenceFinder.exe executable, or it will complain about
missing resources when you try to run it.

When everything is built, you'll have this directory structure:

msw/
  source/
    ...all the source code folders
  build/
    wxMSW-2.7/
      ...all the wxWidgets folders
    ReferenceFinder-debug/
        ReferenceFinder.exe
        ... other stuff
    ReferenceFinder-release/
        ReferenceFinder.exe
        about.htm
        help.zip
        SplashScreen.png
        ... other stuff
    reffinder4-debug/
    reffinder4-release/
    about.htm
    help.zip
    SplashScreen.png

And you should be able to run either the Debug or Release builds. If you move
the ReferenceFinder.exe application to another directory, be sure you move the
about.htm, help.zip, and SplashScreen.png files along with it.

Robert J. Lang
