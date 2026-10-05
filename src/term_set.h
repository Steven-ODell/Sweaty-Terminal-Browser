#pragma once
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

void die(const char *s);

enum State { Browser, Rename, Search, Preview, Keys, Delete, Add };

enum class DrawMessageCode {
  opening_empty_folder_warning,
  opening_in_empty_fodler,
  renaming_in_empty_folder,
  deleting_in_empty_folder,
  fix_base_dir,
  entered_empty_field,
  cant_be_opened_with_editor,
  cant_go_past_base_dir,
  cant_open_selection_search,
  fs_error,
  none
};

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
  int message_ticks = 0;
  std::string del_choice = "";
  std::string brand_new_name = "";
  std::string new_name = "";
  std::string clear_and_to_corner = "\x1b[2J\x1b[H";
  bool need_refresh = true;
  bool search_selector = false;
  bool hidden = true;
  bool hidden_holder = hidden;
  State state;
  State previous_state;
  DrawMessageCode message_to_display = DrawMessageCode::none;
  std::string fs_error_message;
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

std::string moveTo(const int row, const int col);

void drawBrowser(std::string &full_buf, Paths &paths, Placement &Pos);

void drawRename(std::string &full_buf, Paths &paths, Placement &Pos);

void drawAdd(std::string &full_buf, Paths &paths, Placement &Pos);

void drawDelete(std::string &full_buf, Paths &paths, Placement &Pos);

void drawSearch(std::string &full_buf, Paths &paths, Placement &Pos);

void drawKeys(std::string &full_buf, Paths &paths, Placement &Pos);
