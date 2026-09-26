#include "search.h"
#include <filesystem>
#include <iostream>

int main() {
  int fails = 0;

  {

    Paths paths;
    paths.all_paths.push_back(
        std::filesystem::directory_entry("/a/m_a_i_n.txt"));
    paths.all_paths.push_back(std::filesystem::directory_entry("/a/main.cpp"));
    auto hits = searchCurBuffer(paths, "main");
    if (hits.empty() ||
        paths.all_paths[hits[0].second].path().string() != "/a/main.cpp") {
      std::cout << "Failure on span penalty" << std::endl;
      fails++;
    }
  }

  {
    Paths paths;
    paths.all_paths.push_back(
        std::filesystem::directory_entry("/home/me/src/main.cpp"));
    paths.all_paths.push_back(
        std::filesystem::directory_entry("/home/me/main.cpp"));
    auto hits = searchCurBuffer(paths, "main");
    if (hits.empty() || paths.all_paths[hits[0].second].path().string() !=
                            "/home/me/main.cpp") {
      std::cout << "Failure on span penalty" << std::endl;
      fails++;
    }
  }

  {
    Paths paths;
    paths.all_paths.push_back(
        std::filesystem::directory_entry("/x/m/aaaa/c/p"));
    paths.all_paths.push_back(std::filesystem::directory_entry("/x/main.cpp"));
    auto hits = searchCurBuffer(paths, "main");
    if (hits.empty() ||
        paths.all_paths[hits[0].second].path().string() != "/x/main.cpp") {
      std::cout << "Failure on span penalty" << std::endl;
      fails++;
    }
  }

  {
    Paths paths;
    paths.all_paths.push_back(
        std::filesystem::directory_entry("/x/m/aaaa/c/p"));
    paths.all_paths.push_back(std::filesystem::directory_entry("/x/main.cpp"));
    auto hits = searchCurBuffer(paths, "zzz");
    if (!hits.empty()) {
      std::cout << "Failure no match" << std::endl;
      fails++;
    }
  }

  {
    Paths paths;
    paths.all_paths.push_back(
        std::filesystem::directory_entry("/x/m/aaaa/c/p"));
    paths.all_paths.push_back(std::filesystem::directory_entry("/x/main.cpp"));
    auto hits = searchCurBuffer(paths, "Main");
    if (!hits.empty()) {
      std::cout << "Failure on captilization match" << std::endl;
      fails++;
    }
  }

  if (fails == 0) {
    std::cout << "passed all tests" << std::endl;
  } else {
    std::cout << "Failed " << fails << " tests" << std::endl;
  }

  return fails;
}
