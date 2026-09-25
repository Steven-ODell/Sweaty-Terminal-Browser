#pragma once
#include "term_set.h"
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace fs = std::filesystem;

void loadEntriesFrPath(Paths &paths, Placement &Pos);

void loadPreviousPath(fs::path cur_path, Paths &paths, Placement &Pos);

void checkIfFile(fs::path path_to_check, Paths &paths, Placement &Pos);

void openInEditor(const fs::path &file, Paths &paths, Placement &Pos);
void openInViewer(const fs::path &file);

void openCurrentPath(fs::path cur_path, Paths &paths, Placement &Pos);

void renamePath(Paths &paths, Placement &Pos);
void deletePath(fs::path incoming_path, Paths &paths, Placement &Pos);
void addNewPath(fs::path current_dir, Paths &paths, Placement &Pos);
