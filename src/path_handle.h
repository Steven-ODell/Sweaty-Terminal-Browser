#pragma once
#include "term_set.h"
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace fs = std::filesystem;

void loadEntriesFrPath(fs::path full_path_entries, Placement &Pos);

void checkIfFile(fs::path path_to_check, Placement &Pos);

void openInEditor(const fs::path &file, Placement &Pos);
void openInViewer(const fs::path &file);

void loadPreviousPath(fs::path cur_path, Placement &Pos);

void openCurrentPath(fs::path path, Placement &Pos);

void renamePath();
void deletePath(fs::path incoming_path);
void addNewPath(fs::path current_dir);
