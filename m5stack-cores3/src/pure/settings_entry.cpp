#include "settings_entry.h"

SettingsEntryAction settingsEntryAction(bool wifiConnected) {
    return wifiConnected ? SettingsEntryAction::ShowStaUrlGuide : SettingsEntryAction::StartApPortal;
}
