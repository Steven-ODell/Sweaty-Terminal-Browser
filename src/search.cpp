#include "search.h"
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

std::vector<std::pair<uint32_t, uint32_t>> searchCurBuffer(std::string cur_buffer) {

  std::vector<int> sorted_array;
  fs::path home_dir;
  std::vector<std::pair<uint32_t, uint32_t>> hits;

  std::vector<uint32_t> indexes_of_sorted;
  std::string to_find = cur_buffer;

  for (size_t cur_path = 0; cur_path < (E.all_paths.size()); cur_path++) {

    std::string path_string = E.all_paths[cur_path].path().filename().string();

    size_t position = path_string.find(to_find);
    if (position != std::string::npos) {
      hits.push_back({(uint32_t)position, (uint32_t)cur_path});
      continue;
    }

    size_t query_index = 0, seq_start = 0, seq_end = 0;

    for (int cur_char = 0; cur_char < path_string.size(); cur_char++) {
      if (query_index < to_find.size() && path_string[cur_char] == to_find[query_index]) {
        if (query_index == 0) {
          seq_start = cur_char;
        }
        seq_end = cur_char;
        query_index++;
      }
    }

    if (query_index == to_find.size()) {
      uint32_t span = seq_end - seq_start + 1;
      hits.push_back({100000 + span, (uint32_t)cur_path});
    }
  }

  sort(hits.begin(), hits.end());
  return hits;
}

void setSearchPath() {
  if (E.search_in == "") {
    std::cout << "Error: Field was empty" << std::endl;
    sleep(1);
  } else {
    E.hits = searchCurBuffer(E.search_in);
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
