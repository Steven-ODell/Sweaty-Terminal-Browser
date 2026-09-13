#pragma once
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace fs = std::filesystem;

void loadEntriesFrPath(fs::path full_path_entries);

void checkIfFile(fs::path path_to_check);

void openInEditor(const fs::path &file);
void openInViewer(const fs::path &file);

void loadPreviousPath(fs::path cur_path);

void openCurrentPath(fs::path path);

void renamePath();

void deletePath(fs::path incoming_path);
