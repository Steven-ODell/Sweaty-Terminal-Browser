#pragma once
#include <string>

// Comment out the lines that have code and fill in preference

struct Config {

  // ----------DEFAULTS----------

  // Base Directory limits how far the explorer can go back
  // Also this is the directory that gets loaded on launch for search
  // By default it is $HOME if not set
  // Leave it as empty string to default to $HOME
  std::string base_dir = "";

  // Hide hidden files is on at launch by default
  // Move to false if you want to show hidden files on launch
  bool hidden = true;

  /* TODO:
  // Search algo choice
  // Choices are "fuzzy levenstein", "sweaty", or "sub"
  const std::string search_algo = "sweaty";
   */

  // ----------COLORS----------

  // ----------CODES----------

  /*
  Color    | Text | Bright | BG | Bright BG
  ---------+------+--------+----+----------
  Black    |  30  |   90   | 40 |   100
  Red      |  31  |   91   | 41 |   101
  Green    |  32  |   92   | 42 |   102
  Yellow   |  33  |   93   | 43 |   103
  Blue     |  34  |   94   | 44 |   104
  Magenta  |  35  |   95   | 45 |   105
  Cyan     |  36  |   96   | 46 |   106
  White    |  37  |   97   | 47 |   107
  Default  |  39  |        | 49 |

  Code | Style         | Off
  -----+---------------+-----
  1   | Bold          | 22
  2   | Dim           | 22
  3   | Italic        | 23
  4   | Underline     | 24
  5   | Blink         | 25
  7   | Reverse       | 27
  8   | Hidden        | 28
  9   | Strikethrough | 29
  */

  // Main text color
  const std::string color_reset = "\x1b[0m";

  // Directory header color
  const std::string dir_color = "\x1b[7;35m";

  // Hidden flag color
  const std::string hidden_flag_color = "\x1b[35m";
};
