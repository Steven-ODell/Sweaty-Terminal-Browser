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

  if (getWinSize(&rows, &cols) == 0 &&
      (rows != Pos.screen_rows || (cols != Pos.screen_cols))) {
    Pos.screen_rows = rows;
    Pos.screen_cols = cols;
    int visible = Pos.screen_rows - 2;
    if (Pos.cur_row - Pos.window_offset >= visible)
      Pos.window_offset = Pos.cur_row - visible + 1;
    term.need_refresh = true;
  }

  if (c == 0) {
    if (term.message_to_display != DrawMessageCode::none) {
      term.message_ticks++;
      if (term.message_ticks >= 12) {
        term.message_to_display = DrawMessageCode::none;
        term.need_refresh = true;
        term.message_ticks = 0;
      }
    }
    return;
  }

  term.message_to_display = DrawMessageCode::none;
  term.message_ticks = 0;
  term.need_refresh = true;
  term.fs_error_message = "";

  switch (term.state) {

  case State::Browser: {
    switch (c) {
    // Quit
    case 'q':
    case 'Q':
    // Esc
    case '\x1b': {
      write(STDOUT_FILENO, term.clear_and_to_corner.c_str(),
            term.clear_and_to_corner.size());
      exit(0);
      break;
    }

    case 'H': {
      if (term.hidden) {
        term.hidden = false;
      } else {
        term.hidden = true;
      }
      loadEntriesFrPath(paths, Pos);
      break;
    }

    // Delete
    case 'd': {
      if (paths.entries.empty()) {
        term.message_to_display = DrawMessageCode::deleting_in_empty_folder;
        break;
      }
      term.hidden_holder = term.hidden;
      term.hidden = false;
      term.state = State::Delete;
      break;
    }

    // Add
    case 'a': {
      term.hidden_holder = term.hidden;
      term.hidden = false;
      term.state = State::Add;
      break;
    }

    // Set State to Rename
    case 'r': {
      if (paths.entries.empty()) {
        term.message_to_display = DrawMessageCode::renaming_in_empty_folder;
        break;
      }
      term.hidden_holder = term.hidden;
      term.hidden = false;
      term.state = State::Rename;
      break;
    }

    case 's': {
      term.hidden_holder = term.hidden;
      term.hidden = false;
      term.state = State::Search;
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
        write(STDOUT_FILENO, term.clear_and_to_corner.c_str(),
              term.clear_and_to_corner.size());
        exit(0);
      } else {
        std::ofstream out("/tmp/cexp-cd");
        out << paths.entries[Pos.cur_row].path().parent_path().string();
        out.close();
        write(STDOUT_FILENO, term.clear_and_to_corner.c_str(),
              term.clear_and_to_corner.size());
        exit(0);
      }
    }

    // Open
    case ARROW_RIGHT:
    case 'l':
    // Enter
    case '\r': {
      if (paths.entries.empty()) {
        term.message_to_display = DrawMessageCode::opening_in_empty_fodler;
        break;
      }
      openCurrentPath(paths.entries[Pos.cur_row], paths, Pos);
      break;
    }

    // Up
    case ARROW_UP:
    case 'k': {
      Pos.moveCursorUp();
      break;
    }

    // Down
    case ARROW_DOWN:
    case 'j': {
      Pos.moveCursorDown(paths);
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
      term.hidden_holder = term.hidden;
      term.previous_state = term.state;
      term.hidden = false;
      term.state = State::Keys;
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
      if (term.search_selector) {
        selectSearchPath(paths, Pos);
        term.search_selector = false;
      } else {
        Pos.window_offset = 0;
        Pos.cur_row = 0;
        if (paths.search_in == "") {
          term.message_to_display = DrawMessageCode::entered_empty_field;
          break;
        }
        term.search_selector = true;
        setSearchPath(paths);
      }
      break;
    }

    case 'l': {
      if (term.search_selector) {
        selectSearchPath(paths, Pos);
        term.search_selector = false;
      } else {
        Pos.window_offset = 0;
        paths.search_in += c;
        setSearchPath(paths);
      }
      break;
    }

    case ARROW_RIGHT: {
      if (term.search_selector) {
        selectSearchPath(paths, Pos);
        term.search_selector = false;
      }
      break;
    }

    // Up during search selection
    case ARROW_UP: {
      if (term.search_selector) {
        moveCursorUpSearch(Pos);
      }
      break;
    }

    case ARROW_LEFT: {
      break;
    }

    // Up during search selection
    case 'k': {
      if (term.search_selector) {
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
      term.previous_state = term.state;
      term.hidden = false;
      term.state = State::Keys;
      break;
    }

    // Down during path selection
    case ARROW_DOWN: {
      if (term.search_selector) {
        moveCursorDownSearch(paths, Pos);
      }
      break;
    }

    // Down during path selection
    case 'j': {
      if (term.search_selector) {
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
      if (term.search_selector) {
        term.search_selector = false;
      } else {
        Pos.window_offset = 0;
        term.hidden = term.hidden_holder;
        term.search_selector = false;
        loadEntriesFrPath(paths, Pos);
        term.state = State::Browser;
        paths.search_in = "";
      }
      break;
    }

    // Backspace
    case '\x7f': {
      term.search_selector = false;
      Pos.window_offset = 0;
      if (paths.search_in.size() > 0) {
        paths.search_in.pop_back();
        setSearchPath(paths);
      }
      break;
    }

    default: {
      term.search_selector = false;
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
      term.hidden = term.hidden_holder;
      deletePath(paths, Pos);
      break;
    }

    // Esc
    case '\x1b':
    case 'n':
    case 'N': {
      term.hidden = term.hidden_holder;
      loadEntriesFrPath(paths, Pos);
      term.state = State::Browser;
      term.del_choice = "";
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
      term.hidden = term.hidden_holder;
      Pos.cur_row = 0;
      loadEntriesFrPath(paths, Pos);
      term.state = State::Browser;
      term.brand_new_name = "";
      break;
    }

    // Esc
    case '\x1b': {
      term.hidden = term.hidden_holder;
      Pos.cur_row = 0;
      loadEntriesFrPath(paths, Pos);
      term.state = State::Browser;
      term.brand_new_name = "";
      break;
    }

    // Backspace
    case '\x7f': {
      if (term.brand_new_name.size() > 0) {
        term.brand_new_name.pop_back();
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
      term.brand_new_name += c;
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
      term.state = term.previous_state;
      term.hidden = term.hidden_holder;
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
      if (term.new_name == "") {
        term.message_to_display = DrawMessageCode::entered_empty_field;
        term.state = State::Rename;
        term.new_name = "";
      } else {
        renamePath(paths, Pos);
        term.hidden = term.hidden_holder;
        loadEntriesFrPath(paths, Pos);
        term.state = State::Browser;
        term.new_name = "";
      }
      break;
    }

    // Esc
    case '\x1b': {
      term.hidden = term.hidden_holder;
      loadEntriesFrPath(paths, Pos);
      term.state = State::Browser;
      term.new_name = "";
      break;
    }

    // Backspace
    case '\x7f': {
      if (term.new_name.size() > 0) {
        term.new_name.pop_back();
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
      term.new_name += c;
      break;
    }
    }
    break;
  }
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
