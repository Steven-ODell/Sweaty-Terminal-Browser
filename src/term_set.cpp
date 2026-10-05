#include "term_set.h"
#include "config.h"
#include "path_handle.h"
#include "search.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

Term Global;
Config config;

/*
 This termios setup was adapted from and uses the similar functions and names
 as: Build Your Own Text Editor by Jeremy Ruten
 https://viewsourcecode.org/snaptoken/kilo/
 */

void die(const char *s) {

  write(STDOUT_FILENO, Global.clear_and_to_corner.c_str(),
        Global.clear_and_to_corner.size());
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
  std::string full_buf = Global.clear_and_to_corner + config.dir_color +
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

    drawBrowser(full_buf, paths, Pos);

    break;
  }

  case State::Rename: {

    drawRename(full_buf, paths, Pos);

    break;
  }

  case State::Add: {

    drawAdd(full_buf, paths, Pos);

    break;
  }

  case State::Delete: {

    drawDelete(full_buf, paths, Pos);

    break;
  }

  case State::Keys: {

    drawKeys(full_buf, paths, Pos);

    break;
  }

  case State::Search: {

    drawSearch(full_buf, paths, Pos);

    break;
  }

  case State::Preview: {
    break;
  }
  }

  if (Global.hidden && !(Global.state == State::Search) &&
      Global.message_to_display == DrawMessageCode::none) {

    full_buf += moveTo(Pos.screen_rows, 1);

    full_buf += config.hidden_flag_color + "Hidden " + config.color_reset +
                std::to_string(paths.hidden_count) + " | '?' for Keys" +
                moveTo(Pos.cur_row - Pos.window_offset + 2, 1);
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

    argument = argument.substr(1, argument.size());
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

  std::string base_dir = "home=";
  std::string hidden = "hidden=";
  std::string hidden_flag = "hidden-flag-color=";
  std::string header = "header-color=";
  std::string default_color = "default-color=";
  std::string delete_prompt = "delete-prompt-color=";

  size_t base_dir_found = full_file.find(base_dir);
  size_t hidden_found = full_file.find(hidden);

  size_t hidden_flag_color_found = full_file.find(hidden_flag);
  size_t header_color_found = full_file.find(header);
  size_t default_color_found = full_file.find(default_color);
  size_t delete_prompt_color_found = full_file.find(delete_prompt);

  // if you find a keyword check the value

  // Set base_dir to the config file if it has a line and is a valid path
  if (base_dir_found != std::string::npos) {
    int cur_position = base_dir_found + base_dir.size();
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
      sleep(2);
      die("Config");
    }
  }

  if (hidden_found != std::string::npos) {

    size_t cur_position = hidden_found + hidden.size();
    size_t found = full_file.find("false");

    if (found != std::string::npos && found == cur_position) {
      config.hidden = false;
    }
  }

  if (hidden_flag_color_found != std::string::npos) {
    int cur_position = hidden_flag_color_found + hidden_flag.size();
    std::string color;
    while (full_file[cur_position] != '\n') {
      color += full_file[cur_position];
      cur_position++;
    }

    config.hidden_flag_color = "\x1b[" + color + "m";
  }

  if (header_color_found != std::string::npos) {

    int cur_position = header_color_found + header.size();
    std::string color;

    while (full_file[cur_position] != '\n') {
      color += full_file[cur_position];
      cur_position++;
    }

    config.dir_color = "\x1b[" + color + "m";
  }

  if (default_color_found != std::string::npos) {

    int cur_position = default_color_found + default_color.size();
    std::string color;

    while (full_file[cur_position] != '\n') {
      color += full_file[cur_position];
      cur_position++;
    }

    config.color_reset = "\x1b[" + color + "m";
  }

  if (delete_prompt_color_found != std::string::npos) {

    int cur_position = delete_prompt_color_found + delete_prompt.size();
    std::string color;

    while (full_file[cur_position] != '\n') {
      color += full_file[cur_position];
      cur_position++;
    }

    config.delete_prompt_color = "\x1b[" + color + "m";
  }
}

std::string moveTo(const int row, const int col) {
  std::string new_position;

  new_position =
      "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H";

  return new_position;
}

void drawBrowser(std::string &full_buf, Paths &paths, Placement &Pos) {

  full_buf += drawRows(paths, Pos);

  // Put the cursor on the correct row with Pos.cur_row

  if (Global.message_to_display == DrawMessageCode::opening_in_empty_fodler) {
    std::string error_mes =
        "Nothing to open " + moveTo(Pos.cur_row - Pos.window_offset + 2, 1);
    full_buf += error_mes;
  }

  if (Global.message_to_display ==
      DrawMessageCode::opening_empty_folder_warning) {
    std::string error_mes = "[Folder is empty or only contains hidden]" +
                            moveTo(Pos.cur_row - Pos.window_offset + 2, 1);
    full_buf += error_mes;
  }

  if (Global.message_to_display == DrawMessageCode::renaming_in_empty_folder) {
    std::string error_mes =
        "Nothing to rename " + moveTo(Pos.cur_row - Pos.window_offset + 2, 1);
    full_buf += error_mes;
  }

  if (Global.message_to_display == DrawMessageCode::deleting_in_empty_folder) {
    std::string error_mes =
        "Nothing to delete " + moveTo(Pos.cur_row - Pos.window_offset + 2, 1);
    full_buf += error_mes;
  }

  if (Global.message_to_display == DrawMessageCode::cant_go_past_base_dir) {
    std::string error_mes = moveTo(Pos.cur_row - Pos.window_offset + 2, 1) +
                            "[Cant go further back than the home directory]";
    full_buf += error_mes;
  }

  if (Global.message_to_display == DrawMessageCode::trying_add_duplicate) {
    std::string error_mes =
        "Error: Folder or file already exists with that name";
    full_buf += error_mes;
  }

  if (Global.message_to_display ==
      DrawMessageCode::cant_be_opened_with_editor) {
    std::string error_mes =
        moveTo(Pos.cur_row - Pos.window_offset + 2, 1) +
        "[Error this file type can not be opened with an editor]";
    full_buf += error_mes;
  }

  if (Global.message_to_display == DrawMessageCode::fs_error) {
    std::string error_mes = "Error: " + Global.fs_error_message;
    full_buf += error_mes;
  }

  full_buf += moveTo(Pos.screen_rows, 1);
  full_buf += "'?' for Keys \x1b[" +
              std::to_string(Pos.cur_row - Pos.window_offset + 2) + ";1H";
}

void drawRename(std::string &full_buf, Paths &paths, Placement &Pos) {

  full_buf += drawRows(paths, Pos);

  full_buf += moveTo(Pos.screen_rows, 1);

  std::string rename_string =
      "Rename '" + paths.entries[Pos.cur_row].path().filename().string() +
      "' to: " + Global.new_name;
  full_buf += rename_string;

  // Calculate cursor offset
  int name_offset = rename_string.size() + 1;

  // Put the cursor on the correct row at the bottom offset by the name
  full_buf += moveTo(Pos.screen_rows, name_offset);

  if (Global.message_to_display == DrawMessageCode::entered_empty_field) {
    std::string error_mes = "Error: Field was empty";
    full_buf += error_mes;
  }

  if (Global.message_to_display == DrawMessageCode::fs_error) {
    std::string error_mes = "Error: " + Global.fs_error_message;
    full_buf += error_mes;
  }
}

void drawAdd(std::string &full_buf, Paths &paths, Placement &Pos) {

  full_buf += drawRows(paths, Pos);

  full_buf += moveTo(Pos.screen_rows, 1);

  std::string add_string =
      "Create ('/'in the start for directory): " + Global.brand_new_name;

  full_buf += add_string;
  // Calculate cursor offset
  int name_offset = add_string.size() + 1;

  // Put the cursor on the correct row at the bottom offset by the name
  full_buf += moveTo(Pos.screen_rows, name_offset);

  if (Global.message_to_display == DrawMessageCode::entered_empty_field) {
    std::string error_mes = "Error: Field was empty";
    full_buf += error_mes;
  }

  if (Global.message_to_display == DrawMessageCode::trying_add_duplicate) {
    std::string error_mes =
        "Error: Folder or file already exists with that name";
    full_buf += error_mes;
  }

  if (Global.message_to_display == DrawMessageCode::fs_error) {
    std::string error_mes = "Error: " + Global.fs_error_message;
    full_buf += error_mes;
  }
}

void drawDelete(std::string &full_buf, Paths &paths, Placement &Pos) {

  full_buf += drawRows(paths, Pos);

  full_buf += moveTo(Pos.screen_rows, 1) + config.delete_prompt_color;

  std::string delete_string =
      "Are you sure you want to delete '" +
      paths.entries[Pos.cur_row].path().filename().string() + "': [y/n]";

  // Calculate cursor offset
  int name_offset = delete_string.size() + 1;

  full_buf += delete_string;

  // Put the cursor on the correct row and column with offset
  full_buf += config.color_reset + moveTo(Pos.screen_rows, name_offset);

  if (Global.message_to_display == DrawMessageCode::fs_error) {
    std::string error_mes = "Error: " + Global.fs_error_message;
    full_buf += error_mes;
  }
}

void drawSearch(std::string &full_buf, Paths &paths, Placement &Pos) {

  full_buf += Global.clear_and_to_corner;

  full_buf += drawSearchRows(paths, Pos);

  full_buf += moveTo(Pos.screen_rows, 1);

  std::string search_prompt = "Search for: ";

  full_buf += search_prompt + paths.search_in;

  if (!Global.search_selector) {

    // Calculate offset
    int search_offset = paths.search_in.size() + search_prompt.size() + 1;

    // Put the cursor on the correct row with screen_rows and offset
    full_buf += moveTo(Pos.screen_rows, search_offset);
  } else {

    // Put the cursor on the correct row and first comuln
    full_buf += moveTo(Pos.cur_row - Pos.window_offset + 1, 1);
  }

  if (Global.message_to_display == DrawMessageCode::entered_empty_field) {
    std::string error_mes = "Error: Field was empty";
    full_buf += error_mes;
  }

  if (Global.message_to_display ==
      DrawMessageCode::cant_open_selection_search) {
    std::string error_mes =
        moveTo(Pos.cur_row - Pos.window_offset + 2, 1) +
        "[Error this file type can not be opened: Loaded parent path]";
    full_buf += error_mes;
  }
}

void drawKeys(std::string &full_buf, Paths &paths, Placement &Pos) {

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
}
