#pragma once
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

void die(const char *s);

enum State { Browser, Rename, Search, Preview, Keys, Delete, Add };

struct Paths {
  int skipped_paths = 0;
  int rows_for_entry;
  std::vector<std::filesystem::directory_entry> all_paths;
  std::string base_dir;
  std::string previous_path;
  std::string search_in;
  std::filesystem::path full_path;
  std::vector<std::pair<uint32_t, uint32_t>> hits;
  std::vector<std::filesystem::directory_entry> entries;
};

struct Config {
  struct termios orig_termios;
  int hidden_count = 0;
  std::string del_choice;
  std::string brand_new_name;
  std::string dir_color = "\x1b[35m";
  std::string color_reset = "\x1b[0m";
  std::string new_name;
  bool search_selector;
  bool hidden = true;
  bool hidden_holder;
  State state;
  State previous_state;
};

struct Placement {
  int cur_row;
  int window_offset = 0;
  int screen_rows;
  int screen_cols;
};

extern Config E;

void disableRawMode();

void enableRawMode();

char readKey();

std::string drawRows(Paths &paths, Placement &Pos);

int getWinSize(int *rows, int *cols);

void refreshScreen(Paths &paths, Placement &Pos);

void initExplorer(Paths &paths, Placement &Pos);

void setPathsForBaseSearch(Paths &paths);

void check_start_path(Paths &paths, Placement &Pos);

void handle_arg(std::string argument, Paths &paths);
