#include "steam/api.h"
#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "Line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

int main(void) {
    /* No Steam DLL is loaded and no real stats or achievements are touched. */
    CHECK(steam_userstats() == NULL);
    CHECK(steam_apps() == NULL);
    CHECK(!isteam_userstats_store_stats(NULL));
    CHECK(!isteam_userstats_reset_all_stats(NULL, true));
    CHECK(isteam_apps_get_current_game_language(NULL) == NULL);
    CHECK(isteam_apps_check_dlc_installed(NULL, 2778580) == -1);
    steamapi_uninit();
    CHECK(steam_userstats() == NULL);
    return 0;
}
