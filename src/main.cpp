#include "inputs.h"
#include "term_set.h"
#include <csignal>

int main(int argc, char *argv[]) {

  // construct the pos and paths struct variable
  Placement Pos;
  Paths paths;

  // Set up the signal for the nvim/image viewer triggers
  signal(SIGCHLD, SIG_IGN);

  const char *home_env = std::getenv("HOME");

  loadConfig(home_env);

  initProgram(home_env, paths, Pos);

  if (argc > 1) {
    std::string argument = argv[1];
    handle_arg(argument, paths);
  }

  // Make sure start path isnt empty and correct if so
  check_start_path(paths, Pos);

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
