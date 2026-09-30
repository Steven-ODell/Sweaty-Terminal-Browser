#include "term_set.h"
#include "config.h"
#include "path_handle.h"
#include "search.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

Term Global;
Config config;
std::string move_cursor_corner = "\x1b[2J\x1b[H";

/*
 This termios setup was adapted from and uses the similar functions and names
 as: Build Your Own Text Editor by Jeremy Ruten
 https://viewsourcecode.org/snaptoken/kilo/
 */

void die(const char *s) {

  write(STDOUT_FILENO, move_cursor_corner.c_str(), move_cursor_corner.size());
  perror(s);
  exit(1);
}

void disableRawMode() {

  write(STDOUT_FILENO, "\x1b[?1049l", 8);
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &Global.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {

  tcgetattr(STDIN_FILENO, &Global.orig_termios);
  atexit(disableRawMode);

  struct termios raw = Global.orig_termios;
  raw.c_iflag &= ~(IXON | ICRNL);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
  write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

std::string drawRows(Paths &paths, Placement &Pos) {

  paths.rows_for_entry = Pos.screen_rows - 2;

  std::string full_buf;

  for (int i = 0; i < paths.rows_for_entry; i++) {

    int index = i + Pos.window_offset;

    if (index >= paths.entries.size())
      break;

    std::string buf = "» " + paths.entries[index].path().filename().string();

    if (buf.size() > Pos.screen_cols - 2) {

      buf = buf.substr(0, Pos.screen_cols - 2) + "...";
    }

    // If not at the last line keep writing a new line for the next row
    if (i < paths.rows_for_entry - 1) {

      buf += "\r\n";
    }

    // Add the new line to the previous ones
    full_buf += buf;
  }

  return full_buf;
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

void refreshScreen(Paths &paths, Placement &Pos) {

  // Clear screen and set cursor to top corner and then write the current path
  // Unwrap comment for "Debug" prints
  std::string full_buf = move_cursor_corner + config.dir_color +
                         paths.full_path.filename().string() +
                         config.color_reset

      /*+
    " paths.cur_row:" + std::to_string(Pos.cur_row) +
    " window_off:" + std::to_string(Pos.window_offset) +
    " Rows:" + std::to_string(Pos.screen_rows) +
    " Cols:" + std::to_string(Pos.screen_cols)
    */
      ;

  // Set line to second row for drawRows()
  full_buf += "\x1b[2;H";

  switch (Global.state) {

  case State::Browser: {
    full_buf += drawRows(paths, Pos);

    // Put the cursor on the correct row with Global.cx

    full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";1H";
    full_buf += "'?' for Keys \x1b[" +
                std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
    break;
  }

  case State::Rename: {

    full_buf += drawRows(paths, Pos);

    full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";1H" + "Rename '" +
                paths.entries[Pos.cur_row].path().filename().string() +
                "' to: " + Global.new_name;

    // Calculate cursor offset
    int name_offset =
        Global.new_name.size() + 15 +
        paths.entries[Pos.cur_row].path().filename().string().size();

    // Put the cursor on the correct row at the bottom offset by the name
    full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";" +
                std::to_string(name_offset) + "H";

    break;
  }

  case State::Add: {

    full_buf += drawRows(paths, Pos);

    full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";1H" +
                "New folder name: " + Global.brand_new_name;

    // Calculate cursor offset
    int name_offset = Global.brand_new_name.size() + 18;

    // Put the cursor on the correct row at the bottom offset by the name
    full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";" +
                std::to_string(name_offset) + "H";

    break;
  }

  case State::Delete: {

    full_buf += drawRows(paths, Pos);

    full_buf += "\x1b[" + std::to_string(Pos.screen_rows) +
                ";1H\x1b[31m"
                "Are you sure you want to delete '" +
                paths.entries[Pos.cur_row].path().filename().string() +
                config.color_reset + "': [y/n]";

    // Calculate cursor offset
    int name_offset =
        42 + paths.entries[Pos.cur_row].path().filename().string().size();

    // Put the cursor on the correct row and column with offset
    full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";" +
                std::to_string(name_offset) + "H";

    break;
  }

  case State::Keys: {

    Global.hidden = false;

    if (Global.previous_state == State::Search) {

      full_buf = "\x1b[2J\x1b[HSEARCH\r\n"
                 "\r\n"
                 "  typing:\r\n"
                 "    any key         add to the query\r\n"
                 "    Backspace       delete a character\r\n"
                 "    Enter           jump to the results\r\n"
                 "    Esc             cancel, back to browser\r\n"
                 "\r\n"
                 "  picking a result:\r\n"
                 "    j / k / ↓ / ↑   down / up\r\n"
                 "    l / → / Enter   open it\r\n"
                 "    Esc             back to typing\r\n"
                 "\r\n"
                 "  ?                 this screen\r\n"
                 "\r\n"
                 "  press any key to go back\r\n";
    } else {

      full_buf = "\x1b[2J\x1b[HBROWSER\r\n"
                 "\r\n"
                 "  j / k / ↓ / ↑       down / up\r\n"
                 "  l / o / → / Enter   open folder or file\r\n"
                 "  h / ← / Backspace   back to parent folder\r\n"
                 "\r\n"
                 "  a                   new folder\r\n"
                 "  r                   rename\r\n"
                 "  d                   delete\r\n"
                 "\r\n"
                 "  H                   toggle hidden files\r\n"
                 "  s                   search\r\n"
                 "  ?                   this screen\r\n"
                 "\r\n"
                 "  q / Q / Esc         quit\r\n"
                 "\r\n"
                 "  press any key to go back\r\n";
    }

    break;
  }

  case State::Search: {

    full_buf += move_cursor_corner;

    if (!Global.search_selector) {

      Pos.window_offset = 0;

      full_buf += drawSearchRows(paths, Pos);

      full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";1H" +
                  "Search for: " + paths.search_in;

      // Calculate offset
      int search_offset = paths.search_in.size() + 13;

      // Put the cursor on the correct row with screen_rows and offset
      full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";" +
                  std::to_string(search_offset) + "H";
    } else {

      full_buf += drawSearchRows(paths, Pos);

      full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";1H" +
                  "Search for: " + paths.search_in;

      // Put the cursor on the correct row and first comuln
      full_buf +=
          "\x1b[" + std::to_string(Pos.cur_row - Pos.window_offset + 1) + ";1H";
    }

    break;
  }

  case State::Preview: {
    break;
  }
  }

  if (Global.hidden) {

    full_buf += "\x1b[" + std::to_string(Pos.screen_rows) + ";1H";

    full_buf += config.hidden_flag_color + "Hidden " + config.color_reset +
                std::to_string(paths.hidden_count) + " | '?' for Keys \x1b[" +
                std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
  }

  write(STDOUT_FILENO, full_buf.c_str(), full_buf.size());
}

void initExplorer(Paths &paths, Placement &Pos) {

  Global.state = State::Browser;
  Global.hidden = config.hidden;

  // If the window comes back as -1 or invalid then "die"
  if (getWinSize(&Pos.screen_rows, &Pos.screen_cols) == -1)
    die("getWinSize");

  // If it isnt an invalid screen size then load the path into the entries
  loadEntriesFrPath(paths, Pos);
}

void setPathsForBaseSearch(Paths &paths) {

  fs::recursive_directory_iterator cur_dir(paths.base_dir);

  fs::recursive_directory_iterator done;

  while (cur_dir != done) {

    if (!Global.hidden ||
        (*cur_dir).path().string().find("/.") == std::string::npos) {

      paths.all_paths.push_back(*cur_dir);
    }

    std::error_code ec;
    cur_dir.increment(ec);

    if (ec)
      paths.skipped_paths++;
  }
}

void check_start_path(Paths &paths, Placement &Pos) {

  loadEntriesFrPath(paths, Pos);

  if (paths.entries.size() == 0) {

    std::string err_message =
        "Path doesnt contain anything - Loading parent path";

    write(STDOUT_FILENO, err_message.c_str(), err_message.size());

    sleep(1);

    if (paths.full_path != paths.base_dir) {

      paths.full_path = paths.full_path.parent_path();
      check_start_path(paths, Pos);

    } else {

      loadEntriesFrPath(paths, Pos);
    }
  }
}

void handle_arg(std::string &argument, Paths &paths) {

  std::string home_check = argument.substr(0, paths.base_dir.size());
  bool found_home = false;

  if (home_check == paths.base_dir) {

    paths.full_path = argument;
    found_home = true;
  }

  if (argument[0] == '/' && !found_home) {

    std::string err_message = "changing " + argument;
    argument = argument.substr(1, argument.size());
    err_message += " to " + argument;

    write(STDOUT_FILENO, err_message.c_str(), err_message.size());
  }

  paths.full_path = paths.full_path / argument;

  if (!(fs::exists(paths.full_path))) {

    std::string err_message = "NOT A VALID PATH: Loading current dir...";

    write(STDOUT_FILENO, err_message.c_str(), err_message.size());

    paths.full_path = fs::current_path().string();
  }
}

void initProgram(const char *home_env, Paths &paths, Placement &Pos) {

  // Set the path of the folder you are in to the browser directory
  paths.full_path = fs::current_path().string();

  if (!(config.base_dir == "")) {
    if (config.base_dir[config.base_dir.size() - 1] == '/') {
      config.base_dir.pop_back();
    }
    if (fs::is_directory(config.base_dir)) {
      paths.base_dir = config.base_dir;
    } else {
      std::string err_message =
          "Terminated because your config base_dir is invalid"
          "\nEither:\n\n1: Set up your $HOME env\n2: Edit the config "
          "'base_dir'";

      write(STDOUT_FILENO, err_message.c_str(), err_message.size());

      die("Improper config base_dir");
    }
    // Get the $HOME value and set it as the base_di
  } else if (home_env == nullptr || !fs::is_directory(home_env)) {

    std::string err_message =
        "Terminated because you have no $HOME env set "
        "up\nEither:\n\n1: Set up your $HOME env\n2: Edit the config "
        "'base_dir'";

    write(STDOUT_FILENO, err_message.c_str(), err_message.size());

    die("No HOME env set");
  } else {
    paths.base_dir = home_env;
  }

  if (!config.hidden) {
    Global.hidden = false;
  }

  // Loop through and set the initial search array for searching later
  setPathsForBaseSearch(paths);
}

void loadConfig(const char *home_env) {
  if (home_env != nullptr) {
    // Check if the config file exists in the correct path
    std::string config_tail = "/.config/Cexp/config.toml";
    fs::path config_path = home_env + config_tail;
    if (fs::exists(config_path)) {
      parseConfigFile(config_path);
    } else {
      // Load defaults
    }
  } else {
    // Load defaults
  }
}

void parseConfigFile(fs::path &config_path) {

  // load file
  std::ifstream file(config_path);
  std::string while_file;
  std::string full_file;

  // walk and look for keywords
  while (std::getline(file, while_file)) {
    full_file += while_file + '\n';
  }

  size_t base_dir_found = full_file.find("home=");
  size_t hidden_found = full_file.find("hidden=");

  size_t hidden_flag_color_found = full_file.find("hidden_flag_color=");
  size_t header_color_found = full_file.find("header_color=");
  size_t default_color_found = full_file.find("default_color=");

  // Set base_dir to the config file if it has a line and is a valid path
  if (base_dir_found != std::string::npos) {
    int cur_position = base_dir_found + 5;
    std::string new_home;

    while (full_file[cur_position] != '\n') {
      new_home += full_file[cur_position];
      cur_position++;
    }

    if (fs::exists(new_home)) {
      config.base_dir = new_home;
    } else {
      std::string err_message = "Invalid path set in config file";
      write(STDOUT_FILENO, err_message.c_str(), err_message.size());
      die("Config");
    }
  }

  if (hidden_found != std::string::npos) {
    size_t cur_position = hidden_found + 7;
    size_t found = full_file.find("false");
    if (found != std::string::npos && found == cur_position) {
      config.hidden = false;
    }
  }

  // if you find a keyword check the value

  // if the value is a valid choice set it at init
}
