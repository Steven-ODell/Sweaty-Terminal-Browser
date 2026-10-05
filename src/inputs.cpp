#include "inputs.h"
#include "path_handle.h"
#include "search.h"
#include "term_set.h"
#include <filesystem>
#include <fstream>
#include <unistd.h>

int readKey() {

  char c;

  int nread = read(STDERR_FILENO, &c, 1);

  if (nread == -1 && errno != EAGAIN) {
    die("read");
  }

  if (nread == 0) {
    return 0;
  }

  // Catch "esc"
  if (c == '\x1b') {
    return handleEsc();
  }
  return c;
}

void processKeypress(Paths &paths, Placement &Pos) {

  int c = readKey();
  int rows, cols;
  if (getWinSize(&rows, &cols) &&
      rows != (Pos.screen_rows || cols != Pos.screen_cols)) {
    Pos.screen_rows = rows;
    Pos.screen_cols = cols;
    int visible = Pos.screen_rows - 2;
    if (Pos.cur_row - Pos.window_offset >= visible)
      Pos.window_offset = Pos.cur_row - visible + 1;
    Global.need_refresh = true;
  }

  if (c == 0) {
    if (Global.message_to_display != DrawMessageCode::none) {
      Global.message_ticks++;
      if (Global.message_ticks >= 12) {
        Global.message_to_display = DrawMessageCode::none;
        Global.need_refresh = true;
        Global.message_ticks = 0;
      }
    }
    return;
  }

  Global.message_to_display = DrawMessageCode::none;
  Global.message_ticks = 0;
  Global.need_refresh = true;
  Global.fs_error_message = "";

  switch (Global.state) {

  case State::Browser: {
    switch (c) {
    // Quit
    case 'q':
    case 'Q':
    // Esc
    case '\x1b': {
      write(STDOUT_FILENO, Global.clear_and_to_corner.c_str(),
            Global.clear_and_to_corner.size());
      exit(0);
      break;
    }

    case 'H': {
      if (Global.hidden) {
        Global.hidden = false;
      } else {
        Global.hidden = true;
      }
      loadEntriesFrPath(paths, Pos);
      break;
    }

    // Delete
    case 'd': {
      if (paths.entries.empty()) {
        Global.message_to_display = DrawMessageCode::deleting_in_empty_folder;
        break;
      }
      Global.hidden_holder = Global.hidden;
      Global.hidden = false;
      Global.state = State::Delete;
      break;
    }

    // Add
    case 'a': {
      Global.hidden_holder = Global.hidden;
      Global.hidden = false;
      Global.state = State::Add;
      break;
    }

    // Set State to Rename
    case 'r': {
      if (paths.entries.empty()) {
        Global.message_to_display = DrawMessageCode::renaming_in_empty_folder;
        break;
      }
      Global.hidden_holder = Global.hidden;
      Global.hidden = false;
      Global.state = State::Rename;
      break;
    }

    case 's': {
      Global.hidden_holder = Global.hidden;
      Global.hidden = false;
      Global.state = State::Search;
      break;
    }

      // Load the shell to that directory and quit
    case 'o': {
      // open the temp file that is holding cd and add the current path to it
      // and then use the init set by config
      if (fs::is_directory(paths.entries[Pos.cur_row].path())) {
        std::ofstream out("/tmp/cexp-cd");
        out << paths.entries[Pos.cur_row].path().string();
        out.close();
        write(STDOUT_FILENO, Global.clear_and_to_corner.c_str(),
              Global.clear_and_to_corner.size());
        exit(0);
      } else {
        std::ofstream out("/tmp/cexp-cd");
        out << paths.entries[Pos.cur_row].path().parent_path().string();
        out.close();
        write(STDOUT_FILENO, Global.clear_and_to_corner.c_str(),
              Global.clear_and_to_corner.size());
        exit(0);
      }
    }

    // Open
    case ARROW_RIGHT:
    case 'l':
    // Enter
    case '\r': {
      if (paths.entries.empty()) {
        Global.message_to_display = DrawMessageCode::opening_in_empty_fodler;
        break;
      }
      openCurrentPath(paths.entries[Pos.cur_row], paths, Pos);
      break;
    }

    // Up
    case ARROW_UP:
    case 'k': {
      moveCursorUp(Pos);
      break;
    }

    // Down
    case ARROW_DOWN:
    case 'j': {
      moveCursorDown(paths, Pos);
      break;
    }

    // Back
    case ARROW_LEFT:
    case 'h':
    // Backspace
    case '\x7f': {
      loadPreviousPath(paths, Pos);
      break;
    }
    case '?': {
      Global.hidden_holder = Global.hidden;
      Global.previous_state = Global.state;
      Global.hidden = false;
      Global.state = State::Keys;
      break;
    }
    }
    break;
  }
    //------------------------------------------------------------

  case State::Search: {
    switch (c) {
    // Enter
    case '\r': {
      if (Global.search_selector) {
        selectSearchPath(paths, Pos);
        Global.search_selector = false;
      } else {
        Pos.window_offset = 0;
        Pos.cur_row = 0;
        if (paths.search_in == "") {
          Global.message_to_display = DrawMessageCode::entered_empty_field;
          break;
        }
        Global.search_selector = true;
        setSearchPath(paths);
      }
      break;
    }

    case 'l': {
      if (Global.search_selector) {
        selectSearchPath(paths, Pos);
        Global.search_selector = false;
      } else {
        Pos.window_offset = 0;
        paths.search_in += c;
        setSearchPath(paths);
      }
      break;
    }

    case ARROW_RIGHT: {
      if (Global.search_selector) {
        selectSearchPath(paths, Pos);
        Global.search_selector = false;
      }
      break;
    }

    // Up during search selection
    case ARROW_UP: {
      if (Global.search_selector) {
        moveCursorUpSearch(Pos);
      }
      break;
    }

    case ARROW_LEFT: {
      break;
    }

    // Up during search selection
    case 'k': {
      if (Global.search_selector) {
        moveCursorUpSearch(Pos);
      } else {
        Pos.window_offset = 0;
        paths.search_in += c;
        setSearchPath(paths);
      }
      break;
    }

    // Key Binds
    case '?': {
      Global.previous_state = Global.state;
      Global.hidden = false;
      Global.state = State::Keys;
      break;
    }

    // Down during path selection
    case ARROW_DOWN: {
      if (Global.search_selector) {
        moveCursorDownSearch(paths, Pos);
      }
      break;
    }

    // Down during path selection
    case 'j': {
      if (Global.search_selector) {
        moveCursorDownSearch(paths, Pos);
      } else {
        Pos.window_offset = 0;
        paths.search_in += c;
        setSearchPath(paths);
      }
      break;
    }

    // Esc
    case '\x1b': {
      if (Global.search_selector) {
        Global.search_selector = false;
      } else {
        Pos.window_offset = 0;
        Global.hidden = Global.hidden_holder;
        Global.search_selector = false;
        loadEntriesFrPath(paths, Pos);
        Global.state = State::Browser;
        paths.search_in = "";
      }
      break;
    }

    // Backspace
    case '\x7f': {
      Global.search_selector = false;
      Pos.window_offset = 0;
      if (paths.search_in.size() > 0) {
        paths.search_in.pop_back();
        setSearchPath(paths);
      }
      break;
    }

    default: {
      Global.search_selector = false;
      Pos.window_offset = 0;
      paths.search_in += c;
      setSearchPath(paths);
      break;
    }
    }

    break;
  }
    //------------------------------------------------------------

  case State::Delete: {
    switch (c) {

    case 'y':
    case 'Y': {
      Global.hidden = Global.hidden_holder;
      deletePath(paths, Pos);
      break;
    }

    // Esc
    case '\x1b':
    case 'n':
    case 'N': {
      Global.hidden = Global.hidden_holder;
      loadEntriesFrPath(paths, Pos);
      Global.state = State::Browser;
      Global.del_choice = "";
      break;
    }
    }
    break;
  }
    //------------------------------------------------------------

  case State::Add: {
    switch (c) {
    // Enter
    case '\r': {
      addNewPath(paths, Pos);
      Global.hidden = Global.hidden_holder;
      Pos.cur_row = 0;
      loadEntriesFrPath(paths, Pos);
      Global.state = State::Browser;
      Global.brand_new_name = "";
      break;
    }

    // Esc
    case '\x1b': {
      Global.hidden = Global.hidden_holder;
      Pos.cur_row = 0;
      loadEntriesFrPath(paths, Pos);
      Global.state = State::Browser;
      Global.brand_new_name = "";
      break;
    }

    // Backspace
    case '\x7f': {
      if (Global.brand_new_name.size() > 0) {
        Global.brand_new_name.pop_back();
      }
      break;
    }

    case ARROW_UP: {
      break;
    }
    case ARROW_DOWN: {
      break;
    }
    case ARROW_LEFT: {
      break;
    }
    case ARROW_RIGHT: {
      break;
    }

    default: {
      Global.brand_new_name += c;
      break;
    }
    }
    break;
  }

    //------------------------------------------------------------
  case State::Preview: {
    break;
  }

    //------------------------------------------------------------
  case State::Keys: {

    switch (c) {

    default: {
      Global.state = Global.previous_state;
      Global.hidden = Global.hidden_holder;
      loadEntriesFrPath(paths, Pos);
      break;
    }
    }

    break;
  }
    //------------------------------------------------------------

  case State::Rename: {

    switch (c) {
    // Enter
    case '\r': {
      if (Global.new_name == "") {
        Global.message_to_display = DrawMessageCode::entered_empty_field;
        Global.state = State::Rename;
        Global.new_name = "";
      } else {
        renamePath(paths, Pos);
        Global.hidden = Global.hidden_holder;
        loadEntriesFrPath(paths, Pos);
        Global.state = State::Browser;
        Global.new_name = "";
      }
      break;
    }

    // Esc
    case '\x1b': {
      Global.hidden = Global.hidden_holder;
      loadEntriesFrPath(paths, Pos);
      Global.state = State::Browser;
      Global.new_name = "";
      break;
    }

    // Backspace
    case '\x7f': {
      if (Global.new_name.size() > 0) {
        Global.new_name.pop_back();
      }
      break;
    }

    case ARROW_UP: {
      break;
    }
    case ARROW_DOWN: {
      break;
    }
    case ARROW_LEFT: {
      break;
    }
    case ARROW_RIGHT: {
      break;
    }

    default: {
      Global.new_name += c;
      break;
    }
    }
    break;
  }
  }
}

void moveCursorDown(Paths &paths, Placement &Pos) {

  if (Pos.cur_row + 1 < paths.entries.size()) {

    if (Pos.cur_row - Pos.window_offset + 2 <
        Pos.screen_rows - (Pos.screen_rows / 2)) {

      Pos.cur_row++;

    } else if (Pos.window_offset + paths.rows_for_entry <
               paths.entries.size() + 2) {

      Pos.window_offset++;
      Pos.cur_row++;

    } else if (Pos.cur_row + 1 <= paths.entries.size()) {

      Pos.cur_row++;
    }
  }
}

void moveCursorUp(Placement &Pos) {

  if (Pos.cur_row - Pos.window_offset + 2 > 2) {

    Pos.cur_row--;

  } else if (Pos.window_offset > 0) {

    Pos.window_offset--;
    Pos.cur_row--;
  }
}

int handleEsc() {

  // Set an array to track the next to chars
  char seq[2];

  // If nothing it is escape
  if (read(STDIN_FILENO, &seq[0], 1) != 1)
    return '\x1b';
  if (read(STDIN_FILENO, &seq[1], 1) != 1)
    return '\x1b';

  // If it is '[' or 'O' for some terminals need to check
  if (seq[0] == '[' || seq[0] == 'O') {

    switch (seq[1]) {

    // Up
    case 'A': {
      return ARROW_UP;
      break;
    }

    case 'B': {
      return ARROW_DOWN;
      break;
    }

    case 'C': {
      return ARROW_RIGHT;
      break;
    }

    case 'D': {
      return ARROW_LEFT;
      break;
    }
    }
  }
  return '\x1b';
}
