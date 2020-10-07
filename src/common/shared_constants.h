#pragma once
#include "common.h"

namespace CommonSharedConstants
{
    // Flag that can be set on an input event so that it is ignored by Keyboard Manager
    const ULONG_PTR KEYBOARDMANAGER_INJECTED_FLAG = 0x1;

    // Fake key code to represent VK_WIN.
    inline const DWORD VK_WIN_BOTH = 0x104;

    // Path to the event used to invoke the PowerLauncher
    const wchar_t POWER_LAUNCHER_INVOKE_SHARED_EVENT[] = L"Local\\PowerToysRunInvokeEvent-30f26ad7-d36d-4c0e-ab02-68bb5ff3c4ab";

    // Path to the event used to close the PowerLauncher
    const wchar_t POWER_LAUNCHER_CLOSE_SHARED_EVENT[] = L"Local\\PowerToysRunCloseEvent-7736b9a2-4ebc-4527-9a47-a4bbadfab2f7";

    // Max DWORD for key code to disable keys.
    const DWORD VK_DISABLED = 0x100;
}
