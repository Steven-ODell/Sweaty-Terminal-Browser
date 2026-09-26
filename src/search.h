#pragma once
#include "term_set.h"
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

std::vector<std::pair<uint32_t, uint32_t>>
searchCurBuffer(Paths &paths, std::string cur_buffer);

void setSearchPath(Paths &paths);

void selectSearchPath(Paths &paths, Placement &Pos);

std::string drawSearchRows(Paths &paths, Placement &Pos);

void moveCursorDownSearch(Paths &paths, Placement &Pos);

void moveCursorUpSearch(Placement &Pos);
