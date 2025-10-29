#include "system_state.h"
#include "config_storage.h"

bool SystemState::saveConfiguration() {
    return ConfigStorage::saveConfig(*this);
}

bool SystemState::loadConfiguration() {
    return ConfigStorage::loadConfig(*this);
}

void SystemState::resetConfiguration() {
    ConfigStorage::resetToDefaults(*this);
}
