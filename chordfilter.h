#pragma once
#include <ntddkbd.h>

enum KeyAction
{
    Action_Passthrough,
    Action_Consume,
    Action_PressRelease
};

enum KeyAction Update(PKEYBOARD_INPUT_DATA data);