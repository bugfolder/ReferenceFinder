#include "RFVersion.h"
#include <stdio.h>
#include <string.h>


/*
  Tiny helper application for installing ReferenceFinder for Linux
 */

#ifndef INSTALL_PREFIX
#define INSTALL_PREFIX "/usr/local"
#endif

int main(int argc, char **argv) {
  if (argc > 1) {
    if (! strcmp (argv [1], "-a"))
      puts (APP_V_NAME_STR);
    else
      if (! strcmp (argv [1], "-nl"))
	puts (APP_V_M_B_NAME_STR);
      else
	if (! strcmp (argv [1], "-nvb"))
          printf ("%s.%s\n", APP_V_M_B_NAME_STR, BUILD_CODE_STR);
	else
	  if (! strcmp (argv [1], "-nab"))
	    puts (APP_V_M_B_NAME_STR);
	  else
	    if (! strcmp (argv [1], "-b")) {
	      printf ("%s.%s", APP_V_M_B_NAME_STR, BUILD_CODE_STR);
#ifdef __WXDEBUG__
	      puts (" debug");
#elif defined (RFDEBUG)
	      puts (" devel");
#elif defined (RFPROFILE)
	      puts (" profile");
#else
	      putchar ('\n');
#endif
	    }
	    else
	      if (! strcmp (argv [1], "-p")) {
		puts (INSTALL_PREFIX);
	      }
  } else
    puts (APP_V_NAME_STR);
  return 0;
}
