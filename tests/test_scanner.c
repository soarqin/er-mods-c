#include "process/scanner.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "Line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

int main(void) {
    uint8_t bytes[] = {0x12, 0x34, 0xAB, 0xCD, 0x0F};
    CHECK(sig_scan(bytes, sizeof(bytes), "12 34") == bytes);
    CHECK(sig_scan(bytes, sizeof(bytes), "cd 0f") == bytes + 3);
    CHECK(sig_scan(bytes, sizeof(bytes), "12 34 AB CD 0F") == bytes);
    CHECK(sig_scan(bytes, sizeof(bytes), "?B C? ?") == bytes + 2);
    CHECK(sig_scan(bytes, sizeof(bytes), "12 ?? AB") == bytes);
    CHECK(sig_scan(bytes, sizeof(bytes), "F") == bytes + 4);
    CHECK(sig_scan(bytes, sizeof(bytes), "FF") == NULL);
    CHECK(sig_scan(bytes, sizeof(bytes), "GG") == NULL);
    CHECK(sig_scan(bytes, sizeof(bytes), "\xFF") == NULL);
    CHECK(sig_scan(bytes, sizeof(bytes), "") == NULL);
    CHECK(sig_scan(bytes, sizeof(bytes), " \t\r\n") == NULL);
    CHECK(sig_scan(bytes, 0, "12") == NULL);
    CHECK(sig_scan(NULL, 0, "12") == NULL);
    CHECK(sig_scan(bytes, sizeof(bytes), NULL) == NULL);

    /* End the supplied buffer directly before an inaccessible page. */
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    uint8_t *pages = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    CHECK(pages != NULL);
    DWORD old_protect;
    CHECK(VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old_protect));
    uint8_t *last = pages + info.dwPageSize - 1;
    *last = 0x12;
    CHECK(sig_scan(last, 1, "12 34") == NULL);
    CHECK(sig_scan(last, 1, "?? ??") == NULL);
    CHECK(sig_scan(last, 1, "12") == last);
    CHECK(sig_scan(last, 1, "34") == NULL);
    VirtualFree(pages, 0, MEM_RELEASE);

    /* Pattern buffer growth and a match at the final possible position. */
    uint8_t long_bytes[128];
    char pattern[3 * 100 + 1];
    memset(long_bytes, 0xAB, sizeof(long_bytes));
    long_bytes[127] = 0xCD;
    for (size_t i = 0; i < 99; i++) memcpy(pattern + i * 3, "AB ", 3);
    memcpy(pattern + 99 * 3, "CD", 3);
    CHECK(sig_scan(long_bytes, sizeof(long_bytes), pattern) == long_bytes + 28);
    return 0;
}
