/*
 * Copyright (C) 2026, Soar Qin<soarchin@gmail.com>

 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "process/image.h"
#include "process/scanner.h"

#include "steam/api.h"

#include <windows.h>

static HANDLE reset_achievements_on_new_game_thread_handle = NULL;
static bool game_running = false;

DWORD WINAPI reset_achievements_on_new_game_thread(LPVOID arg) {
    (void)arg;
    size_t image_size;
    void *image_base = get_module_image_base(NULL, &image_size);
    if (image_base == NULL) {
        return 1;
    }

    uint8_t *addr = sig_scan(image_base, image_size, "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 05 48 8B 40 58 C3 C3");
    if (!addr) return 1;
    addr += *(int32_t*)(addr + 3) + 7;
    HANDLE process = GetCurrentProcess();
    uint32_t last_igt = UINT32_MAX;
    while (game_running) {
        uint8_t *addr2;
        if (ReadProcessMemory(process, addr, &addr2, sizeof(uint8_t*), NULL) && addr2) {
            uint32_t igt;
            if (ReadProcessMemory(process, addr2 + 0xA0, &igt, sizeof(uint32_t), NULL) && igt > 0) {
                if (igt < last_igt && igt < 5000) {
                    isteam_userstats *ustats = steam_userstats();
                    if (ustats) {
                        isteam_userstats_reset_all_stats(ustats, true);
                        isteam_userstats_store_stats(ustats);
                    }
                }
                last_igt = igt;
            }
        }
        Sleep(1000);
    }
    return 0;
}

static void init(void) {
    game_running = true;
    reset_achievements_on_new_game_thread_handle = CreateThread(NULL, 0, reset_achievements_on_new_game_thread, NULL, 0, NULL);
}

static void uninit(void) {
    game_running = false;
    if (reset_achievements_on_new_game_thread_handle && WaitForSingleObject(reset_achievements_on_new_game_thread_handle, 5000) == WAIT_TIMEOUT)
        TerminateThread(reset_achievements_on_new_game_thread_handle, 0);
    if (reset_achievements_on_new_game_thread_handle != NULL) {
        CloseHandle(reset_achievements_on_new_game_thread_handle);
        reset_achievements_on_new_game_thread_handle = NULL;
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD ul_reason_for_call, LPVOID reserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(module);
            init();
            break;
        case DLL_PROCESS_DETACH:
            uninit();
            break;
    }
    return TRUE;
}
