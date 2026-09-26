#include "inputs.h"
#include "path_handle.h"
#include "search.h"
#include "term_set.h"
#include <filesystem>

std::string quit_escapes = "\x1b[2J\x1b[H";

char readKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  return c;
}

void processKeypress(Paths &paths, Placement &Pos) {

  char c = readKey();

  switch (Global.state) {

  case State::Browser: {
    switch (c) {
    // Quit
    case 'q':
    case 'Q':
    // Esc
    case '\x1b': {
      write(STDOUT_FILENO, quit_escapes.c_str(), quit_escapes.size());
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

    // Open
    case 'o':
    case 'l':
    // Enter
    case '\r': {
      openCurrentPath(paths.entries[Pos.cur_row], paths, Pos);
      break;
    }

    // Up
    case 'k': {
      moveCursorUp(Pos);
      break;
    }

    // Down
    case 'j': {
      moveCursorDown(paths, Pos);
      break;
    }

    // Back
    case 'h':
    // Backspace
    case '\x7f': {
      loadPreviousPath(paths.full_path, paths, Pos);
      break;
    }

    case '?': {
      Global.hidden_holder = Global.hidden;
      Global.previous_state = Global.state;
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
        if (paths.search_in == "") {
          std::string error_mes = "» Error: Field was empty \x1b[";
          error_mes += std::to_string(Pos.screen_rows) + ";13H";
          write(STDOUT_FILENO, error_mes.c_str(), error_mes.size());

          sleep(1);

          break;
        }
        Pos.cur_row = 0;
        Pos.window_offset = 0;
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

    case '?': {
      Global.hidden_holder = Global.hidden;
      Global.previous_state = Global.state;
      Global.state = State::Keys;
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
      if (paths.search_in.size() > 0) {
        paths.search_in.pop_back();
        setSearchPath(paths);
      }
      break;
    }

    default: {
      Global.search_selector = false;
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
      deletePath(paths.entries[Pos.cur_row], paths, Pos);
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
      addNewPath(paths.full_path, paths, Pos);
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
      renamePath(paths, Pos);
      Global.hidden = Global.hidden_holder;
      loadEntriesFrPath(paths, Pos);
      Global.state = State::Browser;
      Global.new_name = "";
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
