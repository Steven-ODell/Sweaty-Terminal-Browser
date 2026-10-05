#include "inputs.h"
#include "term_set.h"
#include <csignal>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {

  // construct the pos and paths struct variable
  Placement Pos;
  Paths paths;

  // Set up the signal for the nvim/image viewer triggers
  signal(SIGCHLD, SIG_IGN);

  const char *home_env = std::getenv("HOME");

  if (argc > 2 && std::string(argv[1]) == "--init") {
    std::string shell = argv[2];
    if (shell == "bash" || shell == "zsh") {
      // Function used with shell to allow exp launch and cd on 'o'
      std::cout << R"FN(
cexp() {
  rm -f /tmp/cexp-cd
  command cexp "$@"
  if [[ -f /tmp/cexp-cd ]]; then
    cd "$(cat /tmp/cexp-cd)"
    rm -f /tmp/cexp-cd
  fi
}
)FN";
      return 0;
    } else {
      std::cerr << "Unsupported shell entered:" << shell << std::endl;
      return 1;
    }
  }
  if (argc == 2) {
    std::string argument = argv[1];
    handle_arg(argument, paths);
  }

  loadConfig(home_env);

  initProgram(home_env, paths, Pos);

  // Make sure start path isnt empty and correct if so
  check_start_path(paths, Pos);

  // Set the terminal to "Raw" mode
  enableRawMode();

  // Init the explorer sceen
  initExplorer(paths, Pos);

  // Wait for user input
  while (1) {
    if (Global.need_refresh) {
      refreshScreen(paths, Pos);
      Global.need_refresh = false;
    }
    processKeypress(paths, Pos);
  }

  return 0;
}
