#ifndef KEYBOARD_H_INCLUDED
#define KEYBOARD_H_INCLUDED

#include "scancode.h"
#define KEY_PRESSED 1
#define KEY_RELEASED 0

unsigned char keyboardGetKey(unsigned int scanCode);
void          keyboardClearKeys(void);
void          keyboardInstallDriver(void);
void          keyboardUninstallDriver(void);

#endif // KEYBOARD_H_INCLUDED

