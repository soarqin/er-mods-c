/* Codes from: [libofdf](https://github.com/Jan200101/libofdf), licensed under MIT */

#include "vdf.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define CHAR_TAB '\t'
#define CHAR_NEWLINE '\n'
#define CHAR_DOUBLE_QUOTE '"'
#define CHAR_BACKSLASH '\\'

/* Each cursor operation is bounded by end; buffers need not be NUL-terminated. */
typedef struct {
    const char *pos;
    const char *end;
} vdf_cursor_t;

static void skip_space(vdf_cursor_t *cursor) {
    while (cursor->pos < cursor->end) {
        if (isspace((unsigned char)*cursor->pos)) {
            cursor->pos++;
        } else if (cursor->end - cursor->pos >= 2 &&
                   cursor->pos[0] == '/' && cursor->pos[1] == '/') {
            while (cursor->pos < cursor->end && *cursor->pos != '\n') cursor->pos++;
        } else {
            break;
        }
    }
}

static char *copy_escaped(const char *start, size_t length) {
    char *result = LocalAlloc(0, length + 1);
    if (result == NULL) return NULL;
    size_t written = 0;
    for (size_t i = 0; i < length; i++) {
        char ch = start[i];
        if (ch == '\\' && i + 1 < length) {
            switch (start[i + 1]) {
                case 'n': ch = '\n'; i++; break;
                case 't': ch = '\t'; i++; break;
                case '\\': case '"': ch = start[++i]; break;
                default: break;
            }
        }
        result[written++] = ch;
    }
    result[written] = '\0';
    return result;
}

static char *parse_string(vdf_cursor_t *cursor) {
    skip_space(cursor);
    if (cursor->pos == cursor->end || *cursor->pos != '"') return NULL;
    const char *start = ++cursor->pos;
    while (cursor->pos < cursor->end) {
        if (*cursor->pos == '\0') return NULL;
        if (*cursor->pos == '"') {
            char *result = copy_escaped(start, (size_t)(cursor->pos - start));
            cursor->pos++;
            return result;
        }
        if (*cursor->pos++ == '\\' && cursor->pos < cursor->end) {
            if (*cursor->pos == '\0') return NULL;
            cursor->pos++;
        }
    }
    return NULL;
}

static struct vdf_object *parse_object(vdf_cursor_t *cursor, unsigned int depth) {
    /* Bound recursion for both parsing and subsequent freeing/printing. */
    if (depth >= 128) return NULL;
    struct vdf_object *object = LocalAlloc(LPTR, sizeof(*object));
    if (object == NULL) return NULL;
    object->key = parse_string(cursor);
    if (object->key == NULL) goto fail;
    skip_space(cursor);
    if (cursor->pos == cursor->end) goto fail;
    if (*cursor->pos == '{') {
        cursor->pos++;
        object->type = VDF_TYPE_ARRAY;
        size_t capacity = 0;
        for (;;) {
            skip_space(cursor);
            if (cursor->pos == cursor->end) goto fail;
            if (*cursor->pos == '}') {
                cursor->pos++;
                break;
            }
            if (object->data.data_array.len == capacity) {
                if (capacity > SIZE_MAX / 2 / sizeof(struct vdf_object *)) goto fail;
                size_t new_capacity = capacity ? capacity * 2 : 8;
                struct vdf_object **values = object->data.data_array.data_value;
                struct vdf_object **new_values = values
                    ? LocalReAlloc(values, new_capacity * sizeof(*values), LMEM_MOVEABLE)
                    : LocalAlloc(0, new_capacity * sizeof(*values));
                if (new_values == NULL) goto fail;
                object->data.data_array.data_value = new_values;
                capacity = new_capacity;
            }
            struct vdf_object *child = parse_object(cursor, depth + 1);
            if (child == NULL) goto fail;
            child->parent = object;
            object->data.data_array.data_value[object->data.data_array.len++] = child;
        }
    } else {
        char *value = parse_string(cursor);
        if (value == NULL) goto fail;
        object->type = VDF_TYPE_STRING;
        object->data.data_string.str = value;
        object->data.data_string.len = strlen(value);
        /* Preserve the original parser's numeric-value representation. */
        size_t digits = strspn(value, "0123456789");
        if (digits > 0 && value[digits] == '\0') {
            errno = 0;
            int64_t number = strtoll(value, NULL, 10);
            if (errno != ERANGE) {
                LocalFree(value);
                object->type = VDF_TYPE_INT;
                object->data.data_int = number;
            }
        }
    }
    skip_space(cursor);
    if (cursor->pos < cursor->end && *cursor->pos == '[') {
        const char *start = ++cursor->pos;
        while (cursor->pos < cursor->end && *cursor->pos != ']') {
            if (*cursor->pos == '\0') goto fail;
            cursor->pos++;
        }
        if (cursor->pos == cursor->end) goto fail;
        object->conditional = copy_escaped(start, (size_t)(cursor->pos - start));
        if (object->conditional == NULL) goto fail;
        cursor->pos++;
    }
    return object;
fail:
    vdf_free_object(object);
    return NULL;
}

struct vdf_object *vdf_parse_buffer(const char *buffer, const size_t size) {
    if (buffer == NULL || size == 0 || size > PTRDIFF_MAX) return NULL;
    vdf_cursor_t cursor = {buffer, buffer + size};
    if (size >= 3 && memcmp(buffer, "\xEF\xBB\xBF", 3) == 0) cursor.pos += 3;
    /* Accept callers that include the string terminator in size. */
    if (cursor.end[-1] == '\0') cursor.end--;
    struct vdf_object *root = parse_object(&cursor, 0);
    skip_space(&cursor);
    if (cursor.pos != cursor.end) {
        vdf_free_object(root);
        return NULL;
    }
    return root;
}

static void print_escaped(const char *s) {
    while (*s) {
        switch (*s) {
            case CHAR_DOUBLE_QUOTE:
                printf("\\\"");
                break;

            case CHAR_TAB:
                printf("\\t");
                break;

            case CHAR_NEWLINE:
                printf("\\n");
                break;

            case CHAR_BACKSLASH:
                printf("\\\\");
                break;

            default:
                printf("%c", *s);
                break;
        }

        ++s;
    }
}

struct vdf_object *vdf_parse_file(const wchar_t *path) {
    struct vdf_object *o = NULL;
    if (!path)
        return o;

    FILE *fd = _wfopen(path, L"rb");
    if (!fd)
        return o;

    if (fseek(fd, 0L, SEEK_END) != 0) {
        fclose(fd);
        return NULL;
    }
    const long file_length = ftell(fd);
    if (file_length <= 0) {
        fclose(fd);
        return NULL;
    }
    const size_t file_size = (size_t)file_length;
    rewind(fd);

    char *buffer = LocalAlloc(0, file_size);
    if (buffer && fread(buffer, 1, file_size, fd) == file_size) {
        o = vdf_parse_buffer(buffer, file_size);
    }
    if (buffer) {
        LocalFree(buffer);
    }

    fclose(fd);

    return o;
}


size_t vdf_object_get_array_length(const struct vdf_object *o) {
    if (!o || o->type != VDF_TYPE_ARRAY) return 0;
    return o->data.data_array.len;
}

struct vdf_object *vdf_object_index_array(const struct vdf_object *o, const size_t index) {
    if (!o || o->type != VDF_TYPE_ARRAY || index >= o->data.data_array.len) return NULL;

    return o->data.data_array.data_value[index];
}

struct vdf_object *vdf_object_index_array_str(const struct vdf_object *o, const char *str) {
    if (!o || !str || o->type != VDF_TYPE_ARRAY)
        return NULL;

    for (size_t i = 0; i < o->data.data_array.len; ++i) {
        struct vdf_object *k = o->data.data_array.data_value[i];
        if (k != NULL && k->key != NULL && !strcmp(k->key, str))
            return k;
    }
    return NULL;
}

const char *vdf_object_get_string(const struct vdf_object *o) {
    if (!o || o->type != VDF_TYPE_STRING) return NULL;

    return o->data.data_string.str;
}

int64_t vdf_object_get_int(const struct vdf_object *o) {
    if (!o || o->type != VDF_TYPE_INT) return 0;

    return o->data.data_int;
}

static void vdf_print_object_indent(const struct vdf_object *o, const int l) {
    if (!o)
        return;

    char *spacing = "\t";

    for (int k = 0; k < l; ++k)
        printf("%s", spacing);

    printf("\"");
    if (o->key != NULL) print_escaped(o->key);
    printf("\"");

    switch (o->type) {
        case VDF_TYPE_ARRAY:
            printf("\n");
            for (int k = 0; k < l; ++k)
                printf("%s", spacing);
            printf("{\n");
            for (size_t i = 0; i < o->data.data_array.len; ++i)
                vdf_print_object_indent(o->data.data_array.data_value[i], l + 1);

            for (int k = 0; k < l; ++k)
                printf("%s", spacing);
            printf("}");
            break;

        case VDF_TYPE_INT:
            printf("\t\t\"%lli\"\n", o->data.data_int);
            break;

        case VDF_TYPE_STRING:
            printf("\t\t\"");
            print_escaped(o->data.data_string.str);
            printf("\"");
            break;

        default:
        case VDF_TYPE_NONE:
            assert(0);
            break;
    }

    if (o->conditional)
        printf("\t\t[%s]", o->conditional);

    printf("\n");
}

void vdf_print_object(const struct vdf_object *o) {
    vdf_print_object_indent(o, 0);
}

void vdf_free_object(struct vdf_object *o) {
    if (!o)
        return;

    switch (o->type) {
        case VDF_TYPE_ARRAY:
            if (o->data.data_array.data_value != NULL) {
                for (size_t i = 0; i < o->data.data_array.len; ++i) {
                    vdf_free_object(o->data.data_array.data_value[i]);
                }
            }
            LocalFree(o->data.data_array.data_value);
            break;


        case VDF_TYPE_STRING:
            if (o->data.data_string.str)
                LocalFree(o->data.data_string.str);
            break;

        default:
        case VDF_TYPE_NONE:
            break;

    }

    if (o->key)
        LocalFree(o->key);

    if (o->conditional)
        LocalFree(o->conditional);
    LocalFree(o);
}
