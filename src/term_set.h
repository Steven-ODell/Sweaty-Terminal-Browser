#pragma once
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

void die(const char *s);

struct Config {
  std::string base_dir = "/home/sao";
  std::vector<std::filesystem::directory_entry> all_paths;
  int cx;
  int screen_rows;
  int screen_cols;
  int window_offset = 0;
  int cur_row;
  int skipped_paths;
  std::string del_choice;
  std::string brand_new_name;
  std::string new_name;
  std::string search_in;
  std::filesystem::path full_path;
  std::vector<std::pair<uint32_t, uint32_t>> hits;
  struct termios orig_termios;
  std::vector<std::filesystem::directory_entry> entries;
  enum class State { Browser, Rename, Search, Preview, Delete, Add };
  bool search_selector;
  bool hidden = true;
  bool hidden_holder;
  State state;
};

extern Config E;

void disableRawMode();

void enableRawMode();

char readKey();

void processKeypress();

void drawRows();

int getWinSize(int *rows, int *cols);

void refreshScreen();

void initExplorer();

void setPathsForBaseSearch();
