#pragma once
#include <asm-generic/ioctls.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void processKeypress();

char readKey();

void moveCursorDown();
