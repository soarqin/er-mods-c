#pragma once

extern void do_hook(void *target, void *detour, void **original);
extern void do_unhook(void *target);
