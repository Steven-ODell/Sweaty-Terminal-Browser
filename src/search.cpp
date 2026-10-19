#include "search.h"
#include "path_handle.h"
#include "term_set.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<std::pair<uint32_t, uint32_t>>
searchCurBuffer(Paths &paths, const std::string &cur_buffer) {
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

  if (fs::is_directory(selected_path)) {

    term.state = State::Browser;
    paths.search_in = "";
    term.search_selector = false;
    term.hidden = term.hidden_holder;

    openCurrentPath(selected_path, paths, Pos);

  } else if (fs::is_regular_file(selected_path)) {

    term.state = State::Browser;
    paths.search_in = "";
    term.search_selector = false;
    term.hidden = term.hidden_holder;

    checkIfFile(selected_path, paths, Pos);

  } else {

    term.message_to_display = DrawMessageCode::cant_open_selection_search;
  }
}
