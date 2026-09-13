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
  std::string del_choice;
  std::string new_name;
  std::filesystem::path full_path;
  struct termios orig_termios;
  std::vector<std::filesystem::directory_entry> entries;
  enum class State { Browser, Rename, Search, Preview, Delete };
  bool hidden;
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
