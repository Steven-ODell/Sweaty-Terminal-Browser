#pragma once
#include "term_set.h"
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// Set the arrow enum higher than any of the 255 ascii characters
enum arrows_dir { ARROW_UP = 1000, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT };

void processKeypress(Paths &paths, Placement &Pos);

int readKey();

int handleEsc();

void moveCursorDown(Paths &paths, Placement &Pos);
void moveCursorUp(Placement &Pos);
