#include "term_set.h"
#include "path_handle.h"
#include "search.h"
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

  if (E.state == Config::State::Browser) {
    drawRows();

    // Put the cursor on the correct row with E.cx
    std::string seq = "\x1b[" + std::to_string(E.cx) + ";1H";
    write(STDOUT_FILENO, seq.c_str(), seq.size());

  } else if (E.state == Config::State::Rename) {
    E.cx = E.screen_rows;
    E.hidden = false;

    drawRows();

    std::string line = "Rename '" + E.entries[E.cur_row - 1].path().filename().string() + "' to: " + E.new_name;
    int name_offset = E.new_name.size() + 15 + E.entries[E.cur_row - 1].path().filename().string().size();
    // Put the cursor on the correct row with E.cx
    line += "\x1b[" + std::to_string(E.cx) + ";" + std::to_string(name_offset) + "H";
    write(STDOUT_FILENO, line.c_str(), line.size());

  } else if (E.state == Config::State::Delete) {
    E.hidden = false;
    drawRows();
    std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H" + "Are you sure you want to delete '" +
                       E.entries[E.cur_row - 1].path().filename().string() + E.new_name + ": [y/n]";
    // Put the cursor on the correct row with E.cx
    line += "\x1b[" + std::to_string(E.cx) + ";1H";
    write(STDOUT_FILENO, line.c_str(), line.size());

  } else if (E.state == Config::State::Search) {
    if (!E.search_selector) {
      E.cx = E.screen_rows;
      E.window_offset = 0;
    }
    E.hidden = false;
    drawSearchRows();
    std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H" + "Search for: " + E.search_in;
    // Put the cursor on the correct row with E.cx
    int search_offset = E.search_in.size() + 13;
    line += "\x1b[" + std::to_string(E.cx) + ";" + std::to_string(search_offset) + "H";
    write(STDOUT_FILENO, line.c_str(), line.size());
  }

  if (E.hidden) {
    std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H";
    line += "Folders are hidden";
    write(STDOUT_FILENO, line.c_str(), line.size());
    std::string seq = "\x1b[" + std::to_string(E.cx) + ";1H";
    write(STDOUT_FILENO, seq.c_str(), seq.size());
  }
}

void initExplorer() {
  E.state = Config::State::Browser;
  E.cx = 1;
  E.hidden = true;
  E.search_selector = false;

  // If the window comes back as -1 or invalid then "die"
  if (getWinSize(&E.screen_rows, &E.screen_cols) == -1)
    die("getWinSize");

  // If it isnt an invalid screen size then load the path into the entries
  loadEntriesFrPath(E.full_path);
}

void setPathsForBaseSearch() {
  fs::recursive_directory_iterator cur_dir(E.base_dir);
  fs::recursive_directory_iterator done;
  while (cur_dir != done) {
    if (!E.hidden || (*cur_dir).path().string().find("/.") == std::string::npos) {
      E.all_paths.push_back(*cur_dir);
    }
    std::error_code ec;
    cur_dir.increment(ec);
    if (ec)
      E.skipped_paths++;
  }
}
