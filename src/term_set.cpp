#include "term_set.h"
#include "path_handle.h"
#include "search.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

Config E;
std::string move_cursor_corner = "\x1b[2J\x1b[H";

void die(const char *s) {
  write(STDOUT_FILENO, move_cursor_corner.c_str(), move_cursor_corner.size());
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

  // Check if hidden to check the amount of rows to draw
  // If it is hidden you have a bottom row and top row to account for
  E.rows_for_entry = E.screen_rows - 2;
  for (int i = 0; i < E.rows_for_entry; i++) {
    int index = i + E.window_offset;
    if (index >= E.entries.size())
      break;
    std::string buf = "» " + E.entries[index].path().filename().string();
    if (buf.size() > E.screen_cols - 2) {
      buf = buf.substr(0, E.screen_cols - 2) + "...";
    }
    write(STDOUT_FILENO, buf.c_str(), buf.size());

    if (i < E.rows_for_entry - 1) {
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
  // Clear screen and set cursor to top corner and then write the current path
  std::string path_header = move_cursor_corner + E.dir_color +
                            E.full_path.filename().string() + E.color_reset /*+
" E.cur_row:" + std::to_string(E.cur_row) +
" E.cx:" + std::to_string(E.cx) +
" w_o:" + std::to_string(E.window_offset) +
" Rows:" + std::to_string(E.screen_rows)*/
      ;

  // Set line to second row for drawRows()
  path_header += "\x1b[2;H";
  write(STDOUT_FILENO, path_header.c_str(), path_header.size());

  switch (E.state) {

  case Config::State::Browser: {
    drawRows();

    // Put the cursor on the correct row with E.cx

    std::string seq = "\x1b[" + std::to_string(E.screen_rows) + ";1H";
    seq += "'?' for Keys \x1b[" +
           std::to_string(E.cur_row - E.window_offset + 1) + ";1H";
    write(STDOUT_FILENO, seq.c_str(), seq.size());
    break;
  }

  case Config::State::Rename: {
    E.hidden = false;
    drawRows();
    std::string line =
        "\x1b[" + std::to_string(E.screen_rows) + ";1H" + "Rename '" +
        E.entries[E.cur_row - E.window_offset - 1].path().filename().string() +
        "' to: " + E.new_name;
    int name_offset =
        E.new_name.size() + 15 +
        E.entries[E.cur_row - 1].path().filename().string().size();
    // Put the cursor on the correct row with E.cx and column with offset
    line += "\x1b[" + std::to_string(E.screen_rows) + ";" +
            std::to_string(name_offset) + "H";
    write(STDOUT_FILENO, line.c_str(), line.size());
    break;
  }

  case Config::State::Add: {
    E.hidden = false;
    drawRows();
    std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H" +
                       "New folder name: " + E.brand_new_name;
    int name_offset = E.brand_new_name.size() + 18;
    // Put the cursor on the correct row and column with offset
    line += "\x1b[" + std::to_string(E.screen_rows) + ";" +
            std::to_string(name_offset) + "H";
    write(STDOUT_FILENO, line.c_str(), line.size());
    break;
  }

  case Config::State::Delete: {
    E.hidden = false;
    drawRows();
    std::string line = "\x1b[" + std::to_string(E.screen_rows) +
                       ";1H\x1b[31m"
                       "Are you sure you want to delete '" +
                       E.entries[E.cur_row - 1].path().filename().string() +
                       E.color_reset + "': [y/n]";
    // Put the cursor on the correct row with E.cx
    int name_offset =
        42 + E.entries[E.cur_row - 1].path().filename().string().size();
    line += "\x1b[" + std::to_string(E.screen_rows) + ";" +
            std::to_string(name_offset) + "H";
    write(STDOUT_FILENO, line.c_str(), line.size());
    break;
  }

  case Config::State::Keys: {
    E.hidden = false;
    if (E.previous_state == Config::State::Search) {
      std::string search_keys = "\x1b[2J\x1b[HSEARCH\r\n"
                                "\r\n"
                                "  typing:\r\n"
                                "    any key     add to the query\r\n"
                                "    Backspace   delete a character\r\n"
                                "    Enter       jump to the results\r\n"
                                "    Esc         cancel, back to browser\r\n"
                                "\r\n"
                                "  picking a result:\r\n"
                                "    j / k       down / up\r\n"
                                "    Enter       open it\r\n"
                                "    i / Esc     back to typing\r\n"
                                "\r\n"
                                "  ?             this screen\r\n"
                                "\r\n"
                                "  press any key to go back\r\n";
      // Put the cursor on the correct row with E.cx
      write(STDOUT_FILENO, search_keys.c_str(), search_keys.size());
    } else {
      std::string browser_keys = "\x1b[2J\x1b[HBROWSER\r\n"
                                 "\r\n"
                                 "  j / k         down / up\r\n"
                                 "  l / o / Enter open folder or file\r\n"
                                 "  h / Backspace back to parent folder\r\n"
                                 "\r\n"
                                 "  a             new folder\r\n"
                                 "  r             rename\r\n"
                                 "  d             delete\r\n"
                                 "\r\n"
                                 "  H             toggle hidden files\r\n"
                                 "  s             search\r\n"
                                 "  ?             this screen\r\n"
                                 "\r\n"
                                 "  q / Q / Esc   quit\r\n"
                                 "\r\n"
                                 "  press any key to go back\r\n";
      // Put the cursor on the correct row with E.cx
      write(STDOUT_FILENO, browser_keys.c_str(), browser_keys.size());
    }
    break;
  }

  case Config::State::Search: {
    write(STDOUT_FILENO, move_cursor_corner.c_str(), move_cursor_corner.size());
    if (!E.search_selector) {
      E.window_offset = 0;
      E.hidden = false;
      drawSearchRows();
      std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H" +
                         "Search for: " + E.search_in;
      // Put the cursor on the correct row with E.cx and column with offset
      int search_offset = E.search_in.size() + 13;
      line += "\x1b[" + std::to_string(E.screen_rows) + ";" +
              std::to_string(search_offset) + "H";
      write(STDOUT_FILENO, line.c_str(), line.size());
    } else {
      E.hidden = false;
      drawSearchRows();
      std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H" +
                         "Search for: " + E.search_in;
      // Put the cursor on the correct row and first comuln
      line += "\x1b[" + std::to_string(E.cur_row - E.window_offset) + ";1H";
      write(STDOUT_FILENO, line.c_str(), line.size());
    }
    break;
  }

  case Config::State::Preview: {
    break;
  }
  }

  if (E.hidden) {
    std::string line = "\x1b[" + std::to_string(E.screen_rows) + ";1H";
    line += "\x1b[31mHidden " + E.color_reset + std::to_string(E.hidden_count) +
            " | '?' for Keys \x1b[" +
            std::to_string(E.cur_row - E.window_offset + 1) + ";1H";
    write(STDOUT_FILENO, line.c_str(), line.size());
  }
}

void initExplorer() {
  E.state = Config::State::Browser;
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
    if (!E.hidden ||
        (*cur_dir).path().string().find("/.") == std::string::npos) {
      E.all_paths.push_back(*cur_dir);
    }
    std::error_code ec;
    cur_dir.increment(ec);
    if (ec)
      E.skipped_paths++;
  }
}

void check_start_path() {
  loadEntriesFrPath(E.full_path);
  if (E.entries.size() == 0) {
    std::cout << "Path doesnt contain anything - Loading parent path"
              << std::endl;
    sleep(1);
    if (E.full_path != E.base_dir) {
      E.full_path = E.full_path.parent_path();
      check_start_path();
    } else {
      loadEntriesFrPath(E.full_path);
    }
  }
}
