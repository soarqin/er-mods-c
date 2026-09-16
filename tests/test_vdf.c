#include "steam/vdf.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr, "Line %d: %s\n", __LINE__, #expr); return 1; } } while (0)

int main(void) {
    static const char input[] =
        "\"libraryfolders\"\r\n"
        "{\r\n"
        "  \"0\" { \"path\" \"C:\\\\Games\" \"apps\" { \"1245620\" \"1\" } }\r\n"
        "  // comment\r\n"
        "  \"1\" { \"path\" \"D:\\\\Steam\" \"apps\" { \"3655690\" \"1\" } }\r\n"
        "}\r\n";
    struct vdf_object *root = vdf_parse_buffer(input, sizeof(input) - 1);
    CHECK(root != NULL);
    CHECK(root->key != NULL && strcmp(root->key, "libraryfolders") == 0);
    CHECK(vdf_object_get_array_length(root) == 2);
    const struct vdf_object *folder = vdf_object_index_array_str(root, "1");
    CHECK(folder != NULL && folder->type == VDF_TYPE_ARRAY);
    const struct vdf_object *path = vdf_object_index_array_str(folder, "path");
    CHECK(path != NULL && path->type == VDF_TYPE_STRING);
    CHECK(strcmp(vdf_object_get_string(path), "D:\\Steam") == 0);
    CHECK(vdf_parse_buffer(input, 7) == NULL);
    CHECK(vdf_object_index_array(root, 99) == NULL);
    vdf_free_object(root);

    CHECK(vdf_parse_buffer(NULL, 0) == NULL);
    CHECK(vdf_parse_buffer("", 0) == NULL);
    CHECK(vdf_parse_buffer("}", 1) == NULL);
    CHECK(vdf_parse_buffer("[", 1) == NULL);
    CHECK(vdf_parse_buffer("\"root\" {", 8) == NULL);
    CHECK(vdf_object_get_array_length(NULL) == 0);
    CHECK(vdf_object_index_array(NULL, 0) == NULL);
    CHECK(vdf_object_get_string(NULL) == NULL);
    CHECK(vdf_object_get_int(NULL) == 0);
    vdf_free_object(NULL);

    /* A comment at the exact end of a non-NUL-terminated input. */
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    char *pages = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    CHECK(pages != NULL);
    DWORD old_protect;
    CHECK(VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old_protect));
    static const char comment[] = "\"root\" {} // final comment";
    char *start = pages + info.dwPageSize - (sizeof(comment) - 1);
    memcpy(start, comment, sizeof(comment) - 1);
    root = vdf_parse_buffer(start, sizeof(comment) - 1);
    CHECK(root != NULL);
    vdf_free_object(root);
    VirtualFree(pages, 0, MEM_RELEASE);
    return 0;
}
