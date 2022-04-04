#ifndef KEYBOARD_H_INCLUDED
#define KEYBOARD_H_INCLUDED

#include "scancode.h"
#define KEY_PRESSED 1
#define KEY_RELEASED 0

unsigned char   KeyboardGetKey(unsigned int scanCode);
void            KeyboardClearKeys();
void            KeyboardInstallDriver();
void            KeyboardUninstallDriver();

#endif // KEYBOARD_H_INCLUDED
