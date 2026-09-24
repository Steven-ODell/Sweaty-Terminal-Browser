#pragma once
#include "term_set.h"
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void processKeypress(Placement Pos);

char readKey();

void moveCursorDown(Placement Pos);
void moveCursorUp(Placement Pos);
