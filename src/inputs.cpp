#include "inputs.h"
#include "path_handle.h"
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

void processKeypress() {

  char c = readKey();

  switch (E.state) {

  case Config::State::Browser: {
    switch (c) {
    // Quit
    case 'q': {
      write(STDOUT_FILENO, quit_escapes.c_str(), quit_escapes.size());
      exit(0);
      break;
    }
    case 'Q': {
      write(STDOUT_FILENO, quit_escapes.c_str(), quit_escapes.size());
      exit(0);
      break;
    }

    case 'H': {
      E.state = Config::State::BrowserHidden;
      loadEntriesFrPath(E.full_path);
      break;
    }

    // Delete
    case 'd': {
      E.state = Config::State::Delete;
      break;
    }

    // Set State to Rename
    case 'r': {
      E.state = Config::State::Rename;
      break;
    }

    // Open
    case 'o': {
      openCurrentPath(E.entries[E.cur_row - 1]);
      break;
    }
    case 'l': {
      openCurrentPath(E.entries[E.cur_row - 1]);
      break;
    }
    // Enter
    case '\r': {
      openCurrentPath(E.entries[E.cur_row - 1]);
    }

    // Up
    case 'k': {
      if (E.cx > 1) {
        E.cx--;
        E.cur_row--;
      } else if (E.window_offset > 0) {
        E.window_offset--;
        E.cur_row--;
      }
      break;
    }

    // Down
    case 'j': {
      if (E.cur_row < (E.entries.size())) {
        if (E.cx < E.screen_rows - (E.screen_rows / 2)) {
          E.cx++;
          E.cur_row++;
        } else if (E.window_offset + E.screen_rows < E.entries.size()) {
          E.window_offset++;
          E.cur_row++;
        } else if (E.cx < E.screen_rows) {
          E.cx++;
          E.cur_row++;
        }
      }
      break;
    }

    // Back
    case 'h': {
      loadPreviousPath(E.full_path);
      break;
    }

    // Esc
    case '\x1b': {
      write(STDOUT_FILENO, quit_escapes.c_str(), quit_escapes.size());
      exit(0);
      break;
    }

    // Backspace
    case '\x7f': {
      loadPreviousPath(E.full_path);
      break;
    }
    }
    break;
  }

  case Config::State::BrowserHidden: {
    switch (c) {
    // Quit
    case 'q': {
      write(STDOUT_FILENO, quit_escapes.c_str(), quit_escapes.size());
      exit(0);
      break;
    }
    case 'Q': {
      write(STDOUT_FILENO, quit_escapes.c_str(), quit_escapes.size());
      exit(0);
      break;
    }

    case 'H': {
      E.state = Config::State::Browser;
      loadEntriesFrPath(E.full_path);
      break;
    }

    // Delete
    case 'd': {
      E.state = Config::State::Delete;
      break;
    }

    // Set State to Rename
    case 'r': {
      E.state = Config::State::Rename;
      break;
    }

    // Open
    case 'o': {
      openCurrentPath(E.entries[E.cur_row - 1]);
      break;
    }
    case 'l': {
      openCurrentPath(E.entries[E.cur_row - 1]);
      break;
    }
    // Enter
    case '\r': {
      openCurrentPath(E.entries[E.cur_row - 1]);
    }

    // Up
    case 'k': {
      if (E.cx > 1) {
        E.cx--;
        E.cur_row--;
      } else if (E.window_offset > 0) {
        E.window_offset--;
        E.cur_row--;
      }
      break;
    }

    // Down
    case 'j': {
      if (E.cur_row < (E.entries.size())) {
        if (E.cx < E.screen_rows - (E.screen_rows / 2)) {
          E.cx++;
          E.cur_row++;
        } else if (E.window_offset + E.screen_rows < E.entries.size()) {
          E.window_offset++;
          E.cur_row++;
        } else if (E.cx < E.screen_rows) {
          E.cx++;
          E.cur_row++;
        }
      }
      break;
    }

    // Back
    case 'h': {
      loadPreviousPath(E.full_path);
      break;
    }

    // Esc
    case '\x1b': {
      write(STDOUT_FILENO, quit_escapes.c_str(), quit_escapes.size());
      exit(0);
      break;
    }

    // Backspace
    case '\x7f': {
      loadPreviousPath(E.full_path);
      break;
    }
    }
    break;
  }

  case Config::State::Search: {
    break;
  }

  case Config::State::Delete: {
    switch (c) {

    case 'y': {
      deletePath(E.entries[E.cur_row - 1]);
      break;
    }

    case 'Y': {
      deletePath(E.entries[E.cur_row - 1]);
      break;
    }

    case 'n': {
      loadEntriesFrPath(E.full_path);
      E.state = Config::State::Browser;
      E.del_choice = "";
      break;
    }

    case 'N': {
      loadEntriesFrPath(E.full_path);
      E.state = Config::State::Browser;
      E.del_choice = "";
      break;
    }
    }
    break;
  }

  case Config::State::Preview: {
    break;
  }

  case Config::State::Rename: {

    switch (c) {
    // Enter
    case '\r': {
      renamePath();
    }

    // Esc
    case '\x1b': {
      loadEntriesFrPath(E.full_path);
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
