#include "search.h"
#include "path_handle.h"
#include "term_set.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::vector<std::pair<uint32_t, uint32_t>> searchCurBuffer(std::string cur_buffer) {

  std::vector<int> sorted_array;
  fs::path home_dir;
  std::vector<std::pair<uint32_t, uint32_t>> hits;

  std::vector<uint32_t> indexes_of_sorted;

  // Go through each path inside the entire directory
  for (size_t cur_path = 0; cur_path < (E.all_paths.size()); cur_path++) {

    // Make the entry you are on a string
    std::string path_string = E.all_paths[cur_path].path().string();

    // Search for the exact input you typed in the string
    // if found add to the hits list.
    size_t position = path_string.find(cur_buffer);
    // != means it is found.
    if (position != std::string::npos) {
      hits.push_back({(uint32_t)position, (uint32_t)cur_path});
      continue;
    }

    // Set variables for finding the subsequence
    size_t query_index = 0, seq_start = 0, seq_end = 0;

    // Go through each character in the string
    for (int cur_char = 0; cur_char < path_string.size(); cur_char++) {
      // If the characters match then start sequence
      if (query_index < cur_buffer.size() && path_string[cur_char] == (cur_buffer)[query_index]) {
        if (query_index == 0) {
          seq_start = cur_char;
        }
        // Always putting the last character it walks to to the end
        seq_end = cur_char;
        query_index++;
      }
    }

    // If you are at the end of the input then the add the span(how long it too to get the substring)
    // to the number to push back to
    if (query_index == cur_buffer.size()) {
      uint32_t span = seq_end - seq_start + 1;
      hits.push_back({100000 + span, (uint32_t)cur_path});
    }
  }

  sort(hits.begin(), hits.end());
  return hits;
}

void setSearchPath() { E.hits = searchCurBuffer(E.search_in); }

void selectSearchPath() {
  fs::path selected_path = E.all_paths[E.hits[E.cur_row - 1].second].path();
  if (fs::exists(selected_path)) {
    E.state = Config::State::Browser;
    E.search_in = "";
    E.search_selector = false;
    openCurrentPath(selected_path);
  } else {
    std::cout << "This folder is empty" << std::endl;
    write(STDOUT_FILENO, "\x1b[H", 3);
    sleep(1);
  }
}

void drawSearchRows() {
  for (int i = 0; i < E.screen_rows - 1; i++) {
    int index = i + E.window_offset;
    if (index >= E.hits.size())
      break;
    std::string buf;
    buf = "» " + E.all_paths[E.hits[index].second].path().string();
    write(STDOUT_FILENO, buf.c_str(), buf.size());
    if (i < E.screen_rows - 1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

void moveCursorDownSearch() {
  if (E.cur_row < (E.hits.size())) {
    if (E.cx < E.screen_rows - (E.screen_rows / 2)) {
      E.cx++;
      E.cur_row++;
    } else if (E.window_offset + E.screen_rows < E.hits.size()) {
      E.window_offset++;
      E.cur_row++;
    } else if (E.cx < E.screen_rows) {
      E.cx++;
      E.cur_row++;
    }
  }
}

void moveCursorUpSearch() {
  if (E.cx > 1) {
    E.cx--;
    E.cur_row--;
  } else if (E.window_offset > 0) {
    E.window_offset--;
    E.cur_row--;
  }
}
