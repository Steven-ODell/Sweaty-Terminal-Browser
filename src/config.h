#pragma once
#include <string>

// Comment out the lines that have code and fill in preference

struct Config {

  // Base Directory limits how far the directory can go back
  // Also this is the directory that gets loaded on launch for search
  // By default it is $HOME if not set
  // Leave it as empty string to default to $HOME
  std::string base_dir = "";

  // Hide hidden files is on at launch by default
  // Move to false if you want to show hidden files on launch
  bool hidden = true;

  // Search algo choice
  // Choices are "fuzzy levenstein", "sweaty", or "sub"
  const std::string search_algo = "sweaty";

  // ----------COLORS----------

  // Main text color
  const std::string color_reset = "\x1b[0m";

  // Directory header color
  const std::string dir_color = "\x1b[31m";

  // Hidden flag color
  const std::string hidden_flag_color = "\x1b[35m";
};
