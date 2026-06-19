#pragma once

#include "PluginInterface.h"

// Plugin identity. The menu/display name is kept identical to the plugin's
// folder-name ("FixParser") so it matches the Notepad++ Plugins Admin entry.
const wchar_t NPP_PLUGIN_NAME[] = L"FixParser";

// Number of menu commands (including separators rendered as disabled items).
const int nbFunc = 4;

void pluginInit(HANDLE hModule);
void pluginCleanUp();
void commandMenuInit();
void commandMenuCleanUp();
bool setCommand(size_t index, const wchar_t* cmdName, PFUNCPLUGINCMD pFunc,
                ShortcutKey* sk, bool check0nInit);

// Menu command handlers.
void cmdPrettyPrint();
void cmdTogglePanel();
void cmdAbout();

// Notification / message hub helpers (called from the exported functions).
void handleNotification(SCNotification* notify);
