#define MAX_SCAN_CODES 256
#define KEYBOARD_CONTROLLER_OUTPUT_BUFFER 0x60
#define KEYBOARD_CONTROLLER_STATUS_REGISTER 0x64
#define PIC_OPERATION_COMMAND_PORT 0x20
#define KEYBOARD_INTERRUPT_VECTOR 0x09

// PPI stands for Programmable Peripheral Interface (which is the Intel 8255A chip)
// The PPI ports are only for IBM PC and XT, however port A is mapped to the same
// I/O address as the Keyboard Controller's (Intel 8042 chip) output buffer for compatibility.
#define PPI_PORT_A 0x60
#define PPI_PORT_B 0x61
#define PPI_PORT_C 0x62
#define PPI_COMMAND_REGISTER 0x63

#include <dos.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>

#include "keyboard.h"

void interrupt (far *oldKeyboardIsr)() = 0L;

unsigned char keyStates[MAX_SCAN_CODES];

static unsigned char isPreviousCodeExtended = 0;

unsigned char KeyboardGetKey(unsigned int scanCode)
{
    // Check for the extended code
    if(scanCode >> 8 == 0xE0)
    {
        // Get rid of the extended code
        scanCode &= 0xFF;
        return keyStates[scanCode + 0x7F];
    }
    else
    {
        return keyStates[scanCode];
    }
}

void KeyboardClearKeys()
{
    memset(&keyStates[0], 0, MAX_SCAN_CODES);
}

void interrupt far KeyboardIsr()
{
    static unsigned char scanCode;
    unsigned char ppiPortB;

    /* The keyboard controller, by default, will send scan codes
    // in Scan Code Set 1 (reference the IBM Technical References
    // for a complete list of scan codes).
    //
    // Scan codes in this set come as make/break codes. The make
    // code is the normal scan code of the key, and the break code
    // is the make code bitwise "OR"ed with 0x80 (the high bit is set).
    //
    // On keyboards after the original IBM Model F 83-key, an 0xE0
    // is prepended to some keys that didn't exist on the original keyboard.
    //
    // Some keys have their scan codes affected by the state of
    // the shift, and num-lock keys. These certain
    // keys have, potentially, quite long scan codes with multiple
    // possible 0xE0 bytes along with other codes to indicate the
    // state of the shift, and num-lock keys.
    //
    // There are two other Scan Code Sets, Set 2 and Set 3. Set 2
    // was introduced with the IBM PC AT, and Set 3 with the IBM
    // PS/2. Set 3 is by far the easiest and most simple set to work
    // with, but not all keyboards support it.
    //
    // Note:
    // The "keyboard controller" chip is different depending on
    // which machine is being used. The original IBM PC uses the
    // Intel 8255A-5, while the IBM PC AT uses the Intel 8042 (UPI-42AH).
    // On the 8255A-5, port 0x61 can be read and written to for various
    // things, one of which will clear the keyboard and disable it or
    // re enable it. There is no such function on the AT and newer, but
    // it is not needed anyways. The 8042 uses ports 0x60 and 0x64. Both
    // the 8255A-5 and the 8042 give the scan codes from the keyboard
    // through port 0x60.

    // On the IBM PC and XT and compatibles, you MUST clear the keyboard
    // after reading the scancode by reading the value at port 0x61,
    // flipping the 7th bit to a 1, and writing that value back to port 0x61.
    // After that is done, flip the 7th bit back to 0 to re-enable the keyboard.
    //
    // On IBM PC ATs and newer, writing and reading port 0x61 does nothing (as far
    // as I know), and using it to clear the keyboard isn't necessary.*/

    scanCode = 0;
    ppiPortB = 0;

    ppiPortB = inp(PPI_PORT_B); // get the current settings in PPI port B
    scanCode = inp(KEYBOARD_CONTROLLER_OUTPUT_BUFFER); // get the scancode waiting in the output buffer
    outp(PPI_PORT_B, ppiPortB | 0x80); // set the 7th bit of PPI port B (clear keyboard)
    outp(PPI_PORT_B, ppiPortB); // clear the 7th bit of the PPI (enable keyboard)

    // Check to see what the code was.
    // Note that we have to process the scan code one byte at a time.
    // This is because we can't get another scan code until the current
    // interrupt is finished.
    if (scanCode == 0xE0)
    {
        // Extended scancode
        isPreviousCodeExtended = 1;
    }
    else
    {
        // Regular scancode
        // Check the high bit, if set, then it's a break code.
        if(isPreviousCodeExtended)
        {
            isPreviousCodeExtended = 0;
            if(scanCode & 0x80)
            {
                scanCode &= 0x7F;
                keyStates[scanCode + 0x7F] = KEY_RELEASED;
            }
            else
            {
                keyStates[scanCode + 0x7F] = KEY_PRESSED;
            }
        }
        else if(scanCode & 0x80)
        {
            scanCode &= 0x7F;
            keyStates[scanCode] = KEY_RELEASED;
        }
        else
        {
            keyStates[scanCode] = KEY_PRESSED;
        }
    }

    // Send a "Non Specific End of Interrupt" command to the PIC.
    // See Intel 8259A datasheet for details.
    outp(PIC_OPERATION_COMMAND_PORT, 0x20);
}

void KeyboardInstallDriver(void)
{
    // Make sure the new ISR isn't already in use.
    if(oldKeyboardIsr == 0L)
    {
        oldKeyboardIsr = getvect(KEYBOARD_INTERRUPT_VECTOR);
        setvect(KEYBOARD_INTERRUPT_VECTOR, KeyboardIsr);
        KeyboardClearKeys();
    }
}

void KeyboardUninstallDriver(void)
{
    // Make sure the new ISR is in use.
    if(oldKeyboardIsr != (void *)0)
    {
        setvect(KEYBOARD_INTERRUPT_VECTOR, oldKeyboardIsr);
        oldKeyboardIsr = (void *)0;
    }
}

