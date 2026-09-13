#include "term_set.h"
#include "path_handle.h"
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>

Config E;
std::string die_string = "\x1b[2J\x1b[H";

void die(const char *s) {
  write(STDOUT_FILENO, die_string.c_str(), die_string.size());

  perror(s);
  exit(1);
}

void disableRawMode() {
  write(STDOUT_FILENO, "\x1b[?1049l", 8);
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &E.orig_termios);
  atexit(disableRawMode);

  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(IXON | ICRNL);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

void drawRows() {
  for (int i = 0; i < E.screen_rows; i++) {
    int index = i + E.window_offset;
    if (index >= E.entries.size())
      break;
    std::string buf;
    buf = "» " + E.entries[index].path().filename().string();
    write(STDOUT_FILENO, buf.c_str(), buf.size());
    if (i < E.screen_rows - 1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

int getWinSize(int *rows, int *cols) {
  struct winsize ws;

  // If the window doesnt exist or is invalid then exit
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;

    // Pull the terminal window dimensions
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

void refreshScreen() {
  // Clear screen and set cursor to top corner
  write(STDOUT_FILENO, die_string.c_str(), die_string.size());

  drawRows();

  if (E.state == Config::State::Rename) {
    std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H";
    line += "Rename '" + E.entries[E.cur_row - 1].path().filename().string() + "' to: " + E.new_name;
    write(STDOUT_FILENO, line.c_str(), line.size());
  }

  else if (E.state == Config::State::Delete) {
    std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H";
    line += "Are you sure you want to delete '" + E.entries[E.cur_row - 1].path().filename().string() + E.new_name +
            ": [y/n]";
    write(STDOUT_FILENO, line.c_str(), line.size());
  }

  if (E.hidden) {
    std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H";
    line += "Folders are hidden";
    write(STDOUT_FILENO, line.c_str(), line.size());
  }

  // Put the cursor on the correct row with E.cx
  std::string seq = "\x1b[" + std::to_string(E.cx) + ";1H";
  write(STDOUT_FILENO, seq.c_str(), seq.size());
}

void initExplorer() {
  E.state = Config::State::Browser;
  E.cx = 1;
  E.hidden = true;

  // If the window comes back as -1 or invalid then "die"
  if (getWinSize(&E.screen_rows, &E.screen_cols) == -1)
    die("getWinSize");

  // If it isnt an invalid screen size then load the path into the entries
  loadEntriesFrPath(E.full_path);
}

void setPathsForBaseSearch() {
  for (const auto &entry : fs::recursive_directory_iterator(E.base_dir)) {
    // if (entry.path().string().find("/.") != std::string::npos)
    // continue;
    E.all_paths.push_back(entry);
  }
}
