#include "search.h"
#include "path_handle.h"
#include "term_set.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::vector<std::pair<uint32_t, uint32_t>>
searchCurBuffer(Paths &paths, std::string cur_buffer) {

  std::vector<std::pair<uint32_t, uint32_t>> hits;

  // Go through each path inside the entire directory
  for (size_t cur_path = 0; cur_path < (paths.all_paths.size()); cur_path++) {

    // Make the entry you are on a string
    std::string path_string = paths.all_paths[cur_path].path().string();

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
      if (query_index < cur_buffer.size() &&
          path_string[cur_char] == (cur_buffer)[query_index]) {
        if (query_index == 0) {
          seq_start = cur_char;
        }
        // Always putting the last character it walks to to the end
        seq_end = cur_char;
        query_index++;
      }
    }

    // If you are at the end of the input then the add the span(how long it too
    // to get the substring) to the number to push back to
    if (query_index == cur_buffer.size()) {
      uint32_t span = seq_end - seq_start + 1;
      hits.push_back({100000 + span, (uint32_t)cur_path});
    }
  }

  sort(hits.begin(), hits.end());
  return hits;
}

void setSearchPath(Paths &paths) {
  paths.hits = searchCurBuffer(paths, paths.search_in);
}

void selectSearchPath(Paths &paths, Placement &Pos) {
  fs::path selected_path =
      paths.all_paths[paths.hits[Pos.cur_row].second].path();
  if (fs::exists(selected_path)) {
    Global.state = State::Browser;
    paths.search_in = "";
    Global.search_selector = false;
    Global.hidden = Global.hidden_holder;
    openCurrentPath(selected_path, paths, Pos);
  } else {
    std::cout << "This folder is empty" << std::endl;
    write(STDOUT_FILENO, "\x1b[H", 3);
    sleep(1);
  }
}

std::string drawSearchRows(Paths &paths, Placement &Pos) {

  std::string full_buf;

  for (int i = 0; i < Pos.screen_rows - 1; i++) {
    if (paths.search_in.size() < 1) {
      break;
    }
    int index = i + Pos.window_offset;
    if (index >= paths.hits.size())
      break;
    std::string buf;
    buf += "» " + paths.all_paths[paths.hits[index].second].path().string();
    if (buf.size() > Pos.screen_cols - 2) {
      buf = buf.substr(0, Pos.screen_cols - 2) + "...";
    }
    if (i < Pos.screen_rows - 2) {
      buf += "\r\n";
    }
    full_buf += buf;
  }
  return full_buf;
}

void moveCursorDownSearch(Paths &paths, Placement &Pos) {
  if (Pos.cur_row + 1 < (paths.hits.size())) {
    if (Pos.cur_row - Pos.window_offset + 2 <
        Pos.screen_rows - (Pos.screen_rows / 2)) {
      Pos.cur_row++;
    } else if (Pos.window_offset + Pos.screen_rows < paths.hits.size()) {
      Pos.window_offset++;
      Pos.cur_row++;
    } else if (Pos.cur_row + 1 < Pos.screen_rows) {
      Pos.cur_row++;
    }
  }
}

void moveCursorUpSearch(Placement &Pos) {
  if (Pos.cur_row - Pos.window_offset > 0) {
    Pos.cur_row--;
  } else if (Pos.window_offset > 0) {
    Pos.window_offset--;
    Pos.cur_row--;
  }
}
