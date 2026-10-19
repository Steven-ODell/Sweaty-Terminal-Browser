#pragma once
#include "config.h"
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

void die(const char *s);

enum State { Browser, Rename, Search, Preview, Keys, Delete, Add };

enum class DrawMessageCode {
  opening_empty_folder_warning,
  opening_in_empty_fodler,
  renaming_in_empty_folder,
  deleting_in_empty_folder,
  trying_add_duplicate,
  fix_base_dir,
  entered_empty_field,
  cant_be_opened_with_editor,
  cant_go_past_base_dir,
  cant_open_selection_search,
  fs_error,
  none
};

struct Paths {
  int skipped_paths = 0;
  int hidden_count = 0;
  std::vector<std::filesystem::directory_entry> all_paths;
  std::string base_dir = "";
  std::string previous_path = "";
  std::string search_in = "";
  std::filesystem::path full_path;
  std::vector<std::pair<uint32_t, uint32_t>> hits;
  std::vector<std::filesystem::directory_entry> entries;

  int entryCount() const { return entries.size(); }
};

struct Term {
  struct termios orig_termios;
  int message_ticks = 0;
  std::string del_choice = "";
  std::string brand_new_name = "";
  std::string new_name = "";
  std::string clear_and_to_corner = "\x1b[2J\x1b[H";
  bool need_refresh = true;
  bool search_selector = false;
  bool hidden = true;
  bool hidden_holder = hidden;
  State state;
  State previous_state;
  DrawMessageCode message_to_display = DrawMessageCode::none;
  std::string fs_error_message;
};

struct Placement {
  int cur_row = 0;
  int window_offset = 0;
  int rows_for_entry = 0;
  int screen_rows;
  int screen_cols;

  int getScreenRows() { return screen_rows; }
  int getScreenCols() { return screen_cols; }

  void moveCursorUp() {

    if (cur_row - window_offset + 2 > 2) {

      cur_row--;

    } else if (window_offset > 0) {

      window_offset--;
      cur_row--;
    }
  }

  void moveCursorDown(Paths &paths) {

    if (cur_row + 1 < paths.entryCount()) {

      if (cur_row - window_offset + 2 < screen_rows - (screen_rows / 2)) {

        cur_row++;

      } else if (window_offset + rows_for_entry < paths.entries.size() + 2) {

        window_offset++;
        cur_row++;

      } else if (cur_row + 1 <= paths.entries.size()) {

        cur_row++;
      }
    }
  }

  void moveCursorUpSearch() {

    if (cur_row - window_offset > 0) {

      cur_row--;

    } else if (window_offset > 0) {

      window_offset--;
      cur_row--;
    }
  }

  void moveCursorDownSearch(Paths &paths) {

    if (cur_row + 1 < (paths.hits.size())) {

      if (cur_row - window_offset + 2 < screen_rows - (screen_rows / 2)) {

        cur_row++;

      } else if (window_offset + screen_rows < paths.hits.size()) {

        window_offset++;
        cur_row++;

      } else if (cur_row + 1 < screen_rows) {

        cur_row++;
      }
    }
  }

  int getWinSize() {

    struct winsize ws;

    // If the window doesnt exist or is invalid then exit
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {

      return -1;

      // Pull the terminal window dimensions
    } else {

      screen_cols = ws.ws_col;
      screen_rows = ws.ws_row;
      return 0;
    }
  }
};

std::string moveTo(const int row, const int col);

struct Renderer {

  Paths &rendererPaths;
  Placement &rendererPos;

  Renderer(Paths &paths, Placement &Pos, Config &config)
      : rendererPaths(paths), rendererPos(Pos) {};

  std::string drawRows() {
    std::string full_buf;

    rendererPos.rows_for_entry = rendererPos.screen_rows - 2;

    for (int i = 0; i < rendererPos.rows_for_entry; i++) {

      int index = i + rendererPos.window_offset;

      if (index >= rendererPaths.entries.size())
        break;

      std::string buf =
          "» " + rendererPaths.entries[index].path().filename().string();

      if ((buf.size() > rendererPos.screen_cols - 3)) {
        buf = buf.substr(0, rendererPos.screen_cols - 3) + "...";
      }

      buf.insert(0, config.list_color);
      buf.append(config.color_reset);

      // If not at the last line keep writing a new line for the next row
      if (i < rendererPos.rows_for_entry - 1) {
        buf += "\r\n";
      }

      // Add the new line to the previous ones
      full_buf += buf;
    }

    return full_buf;
  }

  void drawBrowser(std::string &full_buf, Term &term) {

    full_buf += drawRows();

    // Put the cursor on the correct row with Pos.cur_row

    if (term.message_to_display == DrawMessageCode::opening_in_empty_fodler) {
      std::string error_mes =
          "Nothing to open " +
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1);
      full_buf += error_mes;
    }

    if (term.message_to_display ==
        DrawMessageCode::opening_empty_folder_warning) {
      std::string error_mes =
          "[Folder is empty or only contains hidden]" +
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1);
      full_buf += error_mes;
    }

    if (term.message_to_display == DrawMessageCode::renaming_in_empty_folder) {
      std::string error_mes =
          "Nothing to rename " +
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1);
      full_buf += error_mes;
    }

    if (term.message_to_display == DrawMessageCode::deleting_in_empty_folder) {
      std::string error_mes =
          "Nothing to delete " +
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1);
      full_buf += error_mes;
    }

    if (term.message_to_display == DrawMessageCode::cant_go_past_base_dir) {
      std::string error_mes =
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1) +
          "[Cant go further back than the home directory]";
      full_buf += error_mes;
    }

    if (term.message_to_display == DrawMessageCode::trying_add_duplicate) {
      std::string error_mes =
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1) +
          "Error: Folder or file already exists with that name";
      full_buf += error_mes;
    }

    if (term.message_to_display ==
        DrawMessageCode::cant_be_opened_with_editor) {
      std::string error_mes =
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1) +
          "[Error this file type can not be opened with an editor]";
      full_buf += error_mes;
    }

    if (term.message_to_display == DrawMessageCode::fs_error) {
      std::string error_mes = "Error: " + term.fs_error_message;
      full_buf += error_mes;
    }

    full_buf += moveTo(rendererPos.screen_rows, 1);
    full_buf +=
        "'?' for Keys \x1b[" +
        std::to_string(rendererPos.cur_row - rendererPos.window_offset + 2) +
        ";1H";
  }

  void drawRename(std::string &full_buf, Term &term) {

    full_buf += drawRows();

    full_buf += moveTo(rendererPos.screen_rows, 1);

    std::string rename_string =
        "Rename '" +
        rendererPaths.entries[rendererPos.cur_row].path().filename().string() +
        "' to: " + term.new_name;
    full_buf += rename_string;

    // Calculate cursor offset
    int name_offset = rename_string.size() + 1;

    // Put the cursor on the correct row at the bottom offset by the name
    full_buf += moveTo(rendererPos.screen_rows, name_offset);

    if (term.message_to_display == DrawMessageCode::entered_empty_field) {
      std::string error_mes = "Error: Field was empty";
      full_buf += error_mes;
    }

    if (term.message_to_display == DrawMessageCode::fs_error) {
      std::string error_mes = "Error: " + term.fs_error_message;
      full_buf += error_mes;
    }
  }

  void drawAdd(std::string &full_buf, Term &term) {

    full_buf += drawRows();

    full_buf += moveTo(rendererPos.screen_rows, 1);

    std::string add_string =
        "Create ('/'in the start for directory): " + term.brand_new_name;

    full_buf += add_string;
    // Calculate cursor offset
    int name_offset = add_string.size() + 1;

    // Put the cursor on the correct row at the bottom offset by the name
    full_buf += moveTo(rendererPos.screen_rows, name_offset);

    if (term.message_to_display == DrawMessageCode::entered_empty_field) {
      std::string error_mes = "Error: Field was empty";
      full_buf += error_mes;
    }

    if (term.message_to_display == DrawMessageCode::trying_add_duplicate) {
      std::string error_mes =
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1) +
          "Error: Folder or file already exists with that name";
      full_buf += error_mes;
    }

    if (term.message_to_display == DrawMessageCode::fs_error) {
      std::string error_mes = "Error: " + term.fs_error_message;
      full_buf += error_mes;
    }
  }

  void drawDelete(std::string &full_buf, Term &term, Config config) {

    full_buf += drawRows();

    full_buf += moveTo(rendererPos.screen_rows, 1) + config.delete_prompt_color;

    std::string delete_string =
        "Are you sure you want to delete '" +
        rendererPaths.entries[rendererPos.cur_row].path().filename().string() +
        "': [y/n]";

    // Calculate cursor offset
    int name_offset = delete_string.size() + 1;

    full_buf += delete_string;

    // Put the cursor on the correct row and column with offset
    full_buf +=
        config.color_reset + moveTo(rendererPos.screen_rows, name_offset);
    if (term.message_to_display == DrawMessageCode::fs_error) {
      std::string error_mes = "Error: " + term.fs_error_message;
      full_buf += error_mes;
    }
  }

  std::string drawSearchRows() {

    std::string full_buf;

    for (int i = 0; i < rendererPos.screen_rows - 1; i++) {

      if (rendererPaths.search_in.size() < 1) {
        break;
      }

      int index = i + rendererPos.window_offset;

      if (index >= rendererPaths.hits.size())
        break;

      std::string buf;

      buf += "» " + rendererPaths.all_paths[rendererPaths.hits[index].second]
                        .path()
                        .string();

      if (buf.size() > rendererPos.screen_cols - 3) {
        buf = buf.substr(0, rendererPos.screen_cols - 3) + "...";
      }

      buf.insert(0, config.list_color);
      buf.append(config.color_reset);

      if (i < rendererPos.screen_rows - 2) {
        buf += "\r\n";
      }

      full_buf += buf;
    }

    return full_buf;
  }

  void drawSearch(std::string &full_buf, Term &term) {

    full_buf += term.clear_and_to_corner;

    full_buf += drawSearchRows();

    full_buf += moveTo(rendererPos.screen_rows, 1);

    std::string search_prompt = "Search for: ";

    full_buf += search_prompt + rendererPaths.search_in;

    if (!term.search_selector) {

      // Calculate offset
      int search_offset =
          rendererPaths.search_in.size() + search_prompt.size() + 1;

      // Put the cursor on the correct row with screen_rows and offset
      full_buf += moveTo(rendererPos.screen_rows, search_offset);
    } else {

      // Put the cursor on the correct row and first comuln
      full_buf +=
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 1, 1);
    }

    if (term.message_to_display == DrawMessageCode::entered_empty_field) {
      std::string error_mes = "Error: Field was empty";
      full_buf += error_mes;
    }

    if (term.message_to_display ==
        DrawMessageCode::cant_open_selection_search) {
      std::string error_mes =
          moveTo(rendererPos.cur_row - rendererPos.window_offset + 2, 1) +
          "[Error this file type can not be opened: Loaded parent path]";
      full_buf += error_mes;
    }
  }
};

extern Term term;

void initProgram(const char *home_env, Paths &paths, Placement &Pos);

void disableRawMode();

void enableRawMode();

void refreshScreen(Paths &paths, Placement &Pos);
void initExplorer(Paths &paths, Placement &Pos);

void setPathsForBaseSearch(Paths &paths);

void check_start_path(Paths &paths, Placement &Pos);

void handle_arg(std::string &argument, Paths &paths);

void loadConfig(const char *home_env);

void parseConfigFile(std::filesystem::path &config_path);

void drawKeys(std::string &full_buf, Paths &paths, Placement &Pos);
