#include "process/image.h"
#include "process/scanner.h"

#include <MinHook.h>

#include <windows.h>
#include <stddef.h>

void* exec_action_button_param_proxy = NULL;
uintptr_t execute_action_button_param_proxy_return = 0;

extern int exec_action_button_param_proxy_hook();

static void do_hook(void *target, void *detour, void **original) {
    MH_CreateHook(target, detour, original);
    MH_EnableHook(target);
}

static void do_unhook(void *target) {
    MH_DisableHook(target);
    MH_RemoveHook(target);
}

static void init(void) {
    MH_Initialize();
    size_t size;
    void *base = get_module_image_base(NULL, &size);
    if (base == NULL) {
        return;
    }
    exec_action_button_param_proxy = (void*)sig_scan(base, size, "48 89 5C 24 08 57 48 81 EC 90 00 00 00 48 8B 84 24 E0 00 00 00 41 0F B6 D9 48 8B 0D ?? ?? ?? ?? 8B FA 0F 29 B4 24 80 00 00 00");
    if (exec_action_button_param_proxy == NULL) {
        return;
    }
    do_hook(exec_action_button_param_proxy, exec_action_button_param_proxy_hook, (void**)&execute_action_button_param_proxy_return);
}


static void uninit(void) {
    do_unhook(exec_action_button_param_proxy);
    MH_Uninitialize();
}

extern int check_exec_action_button_param_filters(uintptr_t action_button_region_system_imp, int entry_id) {
    static const int filters[] = {
        /* Lost runes */
        1000,
        /* Base game Action Button Param Filters */
        7800, 7810, 7811, 7812, 7813, 7814, 7815, 7816, 7817, 7818, 7819, 7820, 7821, 7822,
        7823, 7824, 7825, 7826, 7827, 7828, 7850, 7860,
        7861, 7862, 7863, 7864, 7865, 7866, 7867, 7868, 7869, 7870, 7871, 7872, 7873, 7874,
        7875, 7876, 7877, 7878,
        /* DLC Action Button Param Filters */
        207800, 207810, 207811, 207812, 207813, 207814, 207815, 207816, 207817, 207818, 207819,
        207820, 207821, 207822, 207823, 207824, 207825, 207826, 207827, 207828, 207829, 207830,
        207831, 207832, 207833, 207834, 207835, 207836, 207837, 207838, 207839, 207840, 207841,
        207842, 207843, 207844,
    };

    // Binary search for entry_id in filters array
    int left = 0;
    int right = sizeof(filters) / sizeof(filters[0]) - 1;

    while (left <= right) {
        int mid = (left + right) / 2;
        int value = filters[mid];
        if (value == entry_id) {
            return 1;
        }
        if (value < entry_id) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
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
