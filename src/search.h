#pragma once
#include "term_set.h"
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

std::vector<std::pair<uint32_t, uint32_t>> searchCurBuffer(std::string);

void setSearchPath();

void drawSearchRows();
