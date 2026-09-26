#include "inputs.h"
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
 [ ]Rework Search
 [ ]Preview Mode
 [ ]Tree View
 [ ]Work on color implementations
 [ ]Rework state so that it is one write to the buffer per input
 */

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {

  // construct the pos and paths struct variable
  Placement Pos;
  Paths paths;

  // Set up the signal for the nvim/image viewer triggers
  signal(SIGCHLD, SIG_IGN);

  // Set the path of the folder you are in to the browser directory
  paths.full_path = fs::current_path().string();

  check_start_path(paths, Pos);

  // Get the $HOME value and set it as the base_dir
  const char *home_env = std::getenv("HOME");
  paths.base_dir = home_env;
  int base_dir_len = paths.base_dir.size();
  // Loop through and set the initial search array for searching later
  setPathsForBaseSearch(paths);

  std::cout << "Set " << paths.all_paths.size() << " paths" << std::endl;
  std::cout << "Skipped " << paths.skipped_paths << " paths" << std::endl;

  if (argc > 1) {

    std::string argument = argv[1];
    std::string home_check = argument.substr(0, paths.base_dir.size());
    bool found_home = false;

    if (home_check == paths.base_dir) {
      paths.full_path = argument;
      found_home = true;
    }

    if (argv[1][0] == '/' && !found_home) {
      argv[1] = argv[1] + 1;
      std::cout << "changed " << argv[1] - 1 << " to " << argv[1] << std::endl;
    }
    paths.full_path = paths.full_path / argv[1];

    if (!(fs::exists(paths.full_path))) {
      std::cout << "NOT A VALID PATH: Loading current dir..." << std::endl;
      paths.full_path = fs::current_path().string();
    }
  }
  sleep(1);

  // Set the terminal to "Raw" mode
  enableRawMode();
  // Init the explorer sceen
  initExplorer(paths, Pos);

  // Wait for user input
  while (1) {
    refreshScreen(paths, Pos);
    processKeypress(paths, Pos);
  }

  return 0;
}
