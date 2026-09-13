#include "term_set.h"
#include <csignal>
#include <iostream>

/*
 Create a file explorer that works with ansi codes for terminal and cursor
 controls
 -Functional file browser
 -Nvim controls
 -open into a starting folder as an argument

 -Display files in 1 column
 [x]Make column have a cursor on the left side

 -When you select a file, check type and then open in nvim if applicable
 -Make a way for search for files/folders
 -Rename folders
 m
 the terminal for the preview line pull length
 */

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {

  // Set up the signal for the nvim/image viewer triggers
  signal(SIGCHLD, SIG_IGN);

  // Set the path of the folder you are in to the browser directory
  E.full_path = fs::current_path().string();

  // Loop through and set the initial search array for searching later
  setPathsForBaseSearch();

  std::cout << "Set " << E.all_paths.size() << " paths" << std::endl;
  sleep(1);

  if (argc > 1) {
    E.full_path = E.full_path.string() + argv[1];
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
