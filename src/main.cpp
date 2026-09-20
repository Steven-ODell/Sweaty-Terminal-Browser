#include "term_set.h"
#include <csignal>
#include <cstdlib>
#include <iostream>

/*
 # Create a file explorer that works with ansi codes for terminal and cursor
   controls
 [x]Functional file browser
 [x]Nvim controls

 [x]Display files in 1 column
 [x]Make column have a cursor on the left side

 [x]When you select a file, check type and then open in nvim if applicable
 [x]Make a way for search for files/folders
 [x]Rename folders
 [x]Delete folders
 [x]Add and name a new folder
 [x]Open a starting folder as an argument(basic now, bugs with it)
 [x]Multiple lines for search items breaks curor count
 [x]Bug when leaving nvim you lose track of your hidden state
 [ ]Clean up search
 [ ]Preview Mode
 [ ]Tree View
 [ ]Work on color implementations
 */

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {

  // Set up the signal for the nvim/image viewer triggers
  signal(SIGCHLD, SIG_IGN);

  // Set the path of the folder you are in to the browser directory
  E.full_path = fs::current_path().string();

  // Get the $HOME value and set it as the base_dir
  const char *home_env = std::getenv("HOME");
  E.base_dir = home_env;

  // Loop through and set the initial search array for searching later
  setPathsForBaseSearch();

  std::cout << "Set " << E.all_paths.size() << " paths" << std::endl;
  std::cout << "Skipped " << E.skipped_paths << " paths" << std::endl;
  sleep(1);

  if (argc > 1) {
    if (argv[1][0] != '/') {
      E.full_path = E.full_path.string() + "/" + argv[1];
    } else {
      E.full_path = E.full_path.string() + argv[1];
    }
    if (!(fs::exists(E.full_path))) {
      std::cout << "NOT A VALID PATH" << std::endl;
      sleep(1);
      E.full_path = fs::current_path().string();
    }
  }

  // Set the terminal to "Raw" mode
  enableRawMode();
  // Init the explorer sceen
  initExplorer();

  // Wait for user input
  while (1) {
    refreshScreen();
    processKeypress();
  }

  return 0;
}
