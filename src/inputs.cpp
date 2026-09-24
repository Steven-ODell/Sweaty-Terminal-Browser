#include "inputs.h"
#include "path_handle.h"
#include "search.h"
#include "term_set.h"
#include <filesystem>
#include <iostream>

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

void processKeypress(Placement &Pos) {

  char c = readKey();

  switch (E.state) {

  case Config::State::Browser: {
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
      if (E.hidden) {
        E.hidden = false;
      } else {
        E.hidden = true;
      }
      loadEntriesFrPath(E.full_path, Pos);
      break;
    }

    // Delete
    case 'd': {
      E.hidden_holder = E.hidden;
      E.state = Config::State::Delete;
      break;
    }

    // Add
    case 'a': {
      E.hidden_holder = E.hidden;
      E.state = Config::State::Add;
      break;
    }

    // Set State to Rename
    case 'r': {
      E.hidden_holder = E.hidden;
      E.state = Config::State::Rename;
      break;
    }

    case 's': {
      E.hidden_holder = E.hidden;
      E.state = Config::State::Search;
      break;
    }

    // Open
    case 'o':
    case 'l':
    // Enter
    case '\r': {
      openCurrentPath(E.entries[Pos.cur_row - 1]);
      break;
    }

    // Up
    case 'k': {
      moveCursorUp(Pos);
      break;
    }

    // Down
    case 'j': {
      moveCursorDown(Pos);
      break;
    }

    // Back
    case 'h':
    // Backspace
    case '\x7f': {
      loadPreviousPath(E.full_path);
      break;
    }

    case '?': {
      E.hidden_holder = E.hidden;
      E.previous_state = E.state;
      E.state = Config::State::Keys;
      break;
    }
    }
    break;
  }
    //------------------------------------------------------------

  case Config::State::Search: {
    switch (c) {
    // Enter
    case '\r': {
      if (E.search_selector) {
        selectSearchPath();
        E.search_selector = false;
      } else {
        if (E.search_in == "") {
          std::cout << "Error: Field was empty" << std::endl;
          sleep(1);
          break;
        }
        Pos.cur_row = 1;
        E.window_offset = 0;
        E.search_selector = true;
        setSearchPath();
      }
      break;
    }

    case 'i': {
      if (E.search_selector) {
        E.search_selector = false;
      } else {
        E.search_in += c;
        setSearchPath();
      }
      break;
    }

    case 'k': {
      if (E.search_selector) {
        moveCursorUpSearch();
      } else {
        E.search_in += c;
        setSearchPath();
      }
      break;
    }

    case '?': {
      E.hidden_holder = E.hidden;
      E.previous_state = E.state;
      E.state = Config::State::Keys;
      break;
    }

    // Down during path selection
    case 'j': {
      if (E.search_selector) {
        moveCursorDownSearch();
      } else {
        E.search_in += c;
        setSearchPath();
      }
      break;
    }

    // Esc
    case '\x1b': {
      if (E.search_selector) {
        E.search_selector = false;
      } else {
        E.hidden = E.hidden_holder;
        E.search_selector = false;
        loadEntriesFrPath(E.full_path, Pos);
        E.state = Config::State::Browser;
        E.search_in = "";
      }
      break;
    }

    // Backspace
    case '\x7f': {
      E.search_selector = false;
      if (E.search_in.size() > 0) {
        E.search_in.pop_back();
        setSearchPath();
      }
      break;
    }

    default: {
      E.search_selector = false;
      E.search_in += c;
      setSearchPath();
      break;
    }
    }

    break;
  }
    //------------------------------------------------------------

  case Config::State::Delete: {
    switch (c) {

    case 'y':
    case 'Y': {
      E.hidden = E.hidden_holder;
      deletePath(E.entries[Pos.cur_row - 1]);
      break;
    }

    // Esc
    case '\x1b':
    case 'n':
    case 'N': {
      E.hidden = E.hidden_holder;
      loadEntriesFrPath(E.full_path, Pos);
      E.state = Config::State::Browser;
      E.del_choice = "";
      break;
    }
    }
    break;
  }
    //------------------------------------------------------------
    //
  case Config::State::Add: {
    switch (c) {
    // Enter
    case '\r': {
      addNewPath(E.full_path);
      E.hidden = E.hidden_holder;
      Pos.cur_row = 1;
      loadEntriesFrPath(E.full_path, Pos);
      E.state = Config::State::Browser;
      E.brand_new_name = "";
      break;
    }

    // Esc
    case '\x1b': {
      E.hidden = E.hidden_holder;
      Pos.cur_row = 1;
      loadEntriesFrPath(E.full_path, Pos);
      E.state = Config::State::Browser;
      E.brand_new_name = "";
      break;
    }

    // Backspace
    case '\x7f': {
      if (E.brand_new_name.size() > 0) {
        E.brand_new_name.pop_back();
      }
      break;
    }

    default: {
      E.brand_new_name += c;
      break;
    }
    }
    break;
  }

    //------------------------------------------------------------
  case Config::State::Preview: {
    break;
  }

    //------------------------------------------------------------
  case Config::State::Keys: {

    switch (c) {
    default: {
      E.state = E.previous_state;
      E.hidden = E.hidden_holder;
      loadEntriesFrPath(E.full_path, Pos);
      break;
    }
    }

    break;
  }
    //------------------------------------------------------------

  case Config::State::Rename: {

    switch (c) {
    // Enter
    case '\r': {
      renamePath();
      E.hidden = E.hidden_holder;
      loadEntriesFrPath(E.full_path, Pos);
      E.state = Config::State::Browser;
      E.new_name = "";
      break;
    }

    // Esc
    case '\x1b': {
      E.hidden = E.hidden_holder;
      loadEntriesFrPath(E.full_path, Pos);
      E.state = Config::State::Browser;
      E.new_name = "";
      break;
    }

    // Backspace
    case '\x7f': {
      if (E.new_name.size() > 0) {
        E.new_name.pop_back();
      }
      break;
    }

    default: {
      E.new_name += c;
      break;
    }
    }
    break;
  }
  }
}

void moveCursorDown(Placement &Pos) {
  if (Pos.cur_row < E.entries.size()) {
    if (Pos.cur_row - E.window_offset + 1 <
        Pos.screen_rows - (Pos.screen_rows / 2)) {
      Pos.cur_row++;
    } else if (Pos.window_offset + E.rows_for_entry < E.entries.size() + 2) {
      Pos.window_offset++;
      Pos.cur_row++;
    } else if (Pos.cur_row + 1 <= E.entries.size()) {
      Pos.cur_row++;
    }
  }
}

void moveCursorUp(Placement &Pos) {
  if (Pos.cur_row - E.window_offset + 1 > 2) {
    Pos.cur_row--;
  } else if (E.window_offset > 0) {
    Pos.window_offset--;
    Pos.cur_row--;
  }
}
