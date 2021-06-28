/* Simple calculator for testing the expression parser.
 */
#include "../parser.h"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  bool interactive = 1;
  std::string inputLine;
  std::string prompt = "? ";
  Parser parser;
  if (argc > 1 && ! strcmp (argv [1], "-b"))
    // batch mode, useful for reading from a file (use I/O redirection)
    interactive = 0;

  for ( ; ; ) {
    if (interactive)
      std::cerr << prompt;
    if (! getline (std::cin, inputLine))
      break;

    double res;
    Parser::Status status = parser.evaluate (inputLine, res);
    if (! status.isOk ())
      std::cerr << status << "\n";
    else {
      if (interactive)
	std::cout << "  ";
      std::cout << res << "\n";
    }
  }
}
