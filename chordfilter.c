#include "chordfilter.h"
/*
on press increment and decrement the row modifiers based on hand
on first release apply the current modifiers to the key and emit it
subsequent releases remove modifiers

-TOP/BOTTOM ROWS-
COMPATIBLE MODE -   passes through without modification
FORCED MODE -       blocks the keys
DEDICATED MODE -    moves the number keys down a row and the command keys up a row
*Also optionally swap capslock and tab because capslock is useless
*/

enum Mode
{
    Mode_Compatible,
    Mode_Forced,
    Mode_Dedicated
} mode;

enum ModifierType
{
    None,
    Up,
    Down,
    Shift
};

typedef signed char int8_t;

int8_t modifierDirection = 0;
USHORT lastUpDown = KEY_BREAK;

int IsTopRow(USHORT scancode)
{
    if (scancode >= 0x10 && scancode <= 0x1B)
        return TRUE;
    return FALSE;
}

int IsCentreRow(USHORT scancode)
{
    if (scancode >= 0x1E && scancode <= 0x29)
        return TRUE;
    return FALSE;
}

int IsBottomRow(USHORT scancode)
{
    if (scancode >= 0x2B && scancode <= 0x35)
        return TRUE;
    return FALSE;
}

/*
* in dedicated mode this will modify the top and bottom row keys to perform the number row and control row actions instead
*/
enum KeyAction ModifyTopBottomRows(PKEYBOARD_INPUT_DATA data)
{
    if (IsTopRow(data->MakeCode))
    {
        data->MakeCode -= 9;
    }
    else if (IsBottomRow(data->MakeCode))
    {
        switch (data->MakeCode)
        {
        case 0x2B:
            data->MakeCode = 0x2A;
            break;
        case 0x2C:
            data->MakeCode = 0x1D;
            break;
        case 0x2D:
            data->MakeCode = 0xE05B;
            break;
        case 0x2E:
            data->MakeCode = 0x38;
            break;
        case 0x35:
            data->MakeCode = 0x36;
            break;
        case 0x34:
            data->MakeCode = 0xE01D;
            break;
        case 0x33:
            data->MakeCode = 0xE05D;
            break;
        case 0x32:
            data->MakeCode = 0xE05C;
            break;
        case 0x31:
            data->MakeCode = 0xE038;
            break;
        default:
            return Action_Consume;
        }
    }
    else if (data->MakeCode == 0x3A)
        data->MakeCode = 0x0F;
    else if (data->MakeCode == 0x0F)
        data->MakeCode = 0x3A;
    return Action_Passthrough;
}

/*
* takes a scancode and returns the modifier type
*/
enum ModifierType GetModifier(USHORT scancode)
{
    if (scancode == 0x39) return Shift;
    else if (scancode <= 0x22) return Down;
    else if (scancode > 0x22) return Up;
    else return None;
}

/*
* applies a given modifier type to keypress data
*/
void ApplyModifier(PKEYBOARD_INPUT_DATA data, enum ModifierType modifier)
{
    switch (modifier)
    {
    case Up:
        data->MakeCode -= 0xE;
        break;
    case Down:
        data->MakeCode += 0xD;
        break;
    }
}

/*
* updates the current modifier tracking with new keypress data
*/
void UpdateModifier(PKEYBOARD_INPUT_DATA data)
{
    enum ModifierType KeyModifier = GetModifier(data->MakeCode);
    if ((KeyModifier == Up && data->Flags == KEY_MAKE) || (KeyModifier == Down && data->Flags == KEY_BREAK))++modifierDirection;
    else if ((KeyModifier == Down && data->Flags == KEY_MAKE) || (KeyModifier == Up && data->Flags == KEY_BREAK))--modifierDirection;
}

/*
* if it's a release and the last key was a press then emit a press and release of the current modified key, otherwise consume the keypress and update the modifier direction
*/
enum KeyAction UpdateCentreRow(PKEYBOARD_INPUT_DATA data)
{
    int result = Action_Consume;

    UpdateModifier(data);

    if (data->Flags == KEY_BREAK && lastUpDown == KEY_MAKE)
    {
        if (modifierDirection != 0)
            ApplyModifier(data, modifierDirection < 0 ? Down : Up);
        result = Action_PressRelease;
    }

    lastUpDown = data->Flags;
    return result;
}

enum KeyAction Update(PKEYBOARD_INPUT_DATA data)
{
    if (IsCentreRow(data->MakeCode))
    {
        return UpdateCentreRow(data);
    }
    else if (IsBottomRow(data->MakeCode) || IsTopRow(data->MakeCode))
    {
        switch (mode)
        {
        case Mode_Dedicated:
            return ModifyTopBottomRows(data);
        case Mode_Forced:
            return Action_Consume;
        case Mode_Compatible:
        default:
            return Action_Passthrough;
        }
    }
    else return Action_Passthrough;
}