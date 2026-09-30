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
  int rows_for_entry = 0;
  int hidden_count = 0;
  std::vector<std::filesystem::directory_entry> all_paths;
  std::string base_dir = "";
  std::string previous_path = "";
  std::string search_in = "";
  std::filesystem::path full_path;
  std::vector<std::pair<uint32_t, uint32_t>> hits;
  std::vector<std::filesystem::directory_entry> entries;
};

struct Term {
  struct termios orig_termios;
  std::string del_choice = "";
  std::string brand_new_name = "";
  std::string new_name = "";
  bool search_selector = false;
  bool hidden = true;
  bool hidden_holder = hidden;
  State state;
  State previous_state;
};

struct Placement {
  int cur_row = 0;
  int window_offset = 0;
  int screen_rows;
  int screen_cols;
};

extern Term Global;

void initProgram(const char *home_env, Paths &paths, Placement &Pos);

void disableRawMode();

void enableRawMode();

std::string drawRows(Paths &paths, Placement &Pos);

int getWinSize(int *rows, int *cols);

void refreshScreen(Paths &paths, Placement &Pos);

void initExplorer(Paths &paths, Placement &Pos);

void setPathsForBaseSearch(Paths &paths);

void check_start_path(Paths &paths, Placement &Pos);

void handle_arg(std::string &argument, Paths &paths);

void loadConfig(const char *home_env);

void parseConfigFile(std::filesystem::path &config_path);
