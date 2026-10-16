#include "../desktop_version/src/Network.h"

int NETWORK_init(void) { return 0; }
void NETWORK_shutdown(void) {}
void NETWORK_update(void) {}
void NETWORK_unlockAchievement(const char *name) { (void)name; }
int32_t NETWORK_getAchievementProgress(const char *name) { (void)name; return 0; }
void NETWORK_setAchievementProgress(const char *name, int32_t value) { (void)name; (void)value; }
