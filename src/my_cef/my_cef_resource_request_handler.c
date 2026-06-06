#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../../cef/include/capi/cef_parser_capi.h"
#include "../../cef/include/capi/cef_resource_request_handler_capi.h"

#include "../../include/global.h"
#include "../../include/my_cef.h"

static char *read_data(const char *file_path, size_t data_offset, Resource *resource) {
    assert(file_path != NULL);
    assert(resource != NULL);

    FILE *file = fopen(file_path, "rb");
    char *arena = NULL;
    char *result = arena;

    if (file == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("fopen");
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("fseek");
        goto cleanup;
    }

    long data_len = ftell(file);

    if (data_len == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("ftell");
        goto cleanup;
    }

    rewind(file);

    size_t size = data_offset + data_len + 1;
    arena = malloc(size * sizeof(*arena));

    if (arena == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("malloc");
        goto cleanup;
    }

    char *data = arena + data_offset;
    long written = fread(data, sizeof(*arena), data_len, file);

    if (written < data_len && ferror(file) != 0) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("fread");
        goto cleanup;
    }

    data[data_len] = '\0';

    resource->arena.data = data;
    resource->data_len = data_len;

    result = arena;
    arena = NULL;

cleanup:
    if (fclose(file) == EOF) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("fclose");
    }

    free(arena);
    return result;
}

static bool read_file(const char *file_path, const char *file_name, Resource *resource) {
    assert(file_path != NULL);
    assert(file_name != NULL);
    assert(resource != NULL);

    size_t file_name_len = strlen(file_name);
    size_t file_name_size = file_name_len + 1;

    char *arena = read_data(file_path, file_name_size, resource);

    if (arena == NULL) {
        return false;
    }

    resource->arena.name = memcpy(arena, file_name, file_name_size);
    resource->name_len = file_name_len;

    char *ext = strrchr(file_name, '.');
    ext = ext != NULL ? ext + 1 : "txt";

    cef_string_utf16_t ext16 = {0};
    cef_string_utf8_to_utf16(ext, strlen(ext), &ext16);

    cef_string_userfree_utf16_t mime = cef_get_mime_type(&ext16);

    if (mime == NULL) {
        mime = cef_string_userfree_utf16_alloc();

        if (mime == NULL) {
            fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
            fprintf(stderr, "cef_string_userfree_utf16_alloc: Failed to allocate\n");
            return false;
        }

        const char default_mime[] = "text/plain";
        cef_string_ascii_to_utf16(default_mime, sizeof(default_mime) - 1, mime);
    }

    resource->mime = mime;

    return true;
}

static bool add_resource(my_cef_resource_request_handler_t *self, const Resource *resource) {
    assert(self != NULL);
    assert(resource != NULL);

    Resource **resources = &self->resources;
    size_t new_count = self->resources_count + 1;

    size_t size = new_count * sizeof(**resources);
    Resource *new_resources = *resources == NULL ? malloc(size) : realloc(*resources, size);

    if (new_resources == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("malloc/realloc");
        return false;
    }

    *resources = new_resources;
    (*resources)[self->resources_count] = *resource;
    self->resources_count = new_count;

    return true;
}

static bool load_files(my_cef_resource_request_handler_t *self, const char *path) {
    assert(self != NULL);
    assert(path != NULL);

    DIR *dir = opendir(path);

    if (dir == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("opendir");
        return false;
    }

    struct dirent *ent;
    char file[FILENAME_MAX];
    struct stat st;
    Resource resource;

    while ((ent = readdir(dir)) != NULL) {
        char *file_name = ent->d_name;

        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            continue;
        }

        snprintf(file, sizeof(file), "%s/%s", path, file_name);

        if (stat(file, &st) == -1) {
            fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
            perror("stat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            load_files(self, file);
        } else if (read_file(file, file_name, &resource)) {
            add_resource(self, &resource);
        }
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("closedir");
    }

    return true;
}

static bool strrncmp(const char *s1, const char *s2, size_t n) {
    assert(s1 != NULL);
    assert(s2 != NULL);

    for (size_t i = 0; i < n; ++i) {
        if (s1[-i] != s2[-i]) {
            return false;
        }
    }

    return true;
}

static Resource *find_resource(const my_cef_resource_request_handler_t *self, const char *url, size_t url_len) {
    assert(self != NULL);
    assert(url != NULL);

    for (size_t i = 0; i < self->resources_count; ++i) {
        Resource *resource = self->resources + i;
        const char *name = resource->arena.name;
        size_t name_len = resource->name_len;

        const char *url_end = (url + url_len) - 1;
        const char *name_end = (name + name_len) - 1;

        if (url_len >= name_len && strrncmp(url_end, name_end, name_len)) {
            const char *file = (url_end - name_len) + 1;

            if (file == url || *(file - 1) == '/') {
                return resource;
            }
        }
    }

    return NULL;
}

static struct _cef_resource_handler_t *get_resource_handler(
    struct _cef_resource_request_handler_t *_self,
    struct _cef_browser_t *browser,
    struct _cef_frame_t *frame,
    struct _cef_request_t *request
) {
    (void)browser;
    (void)frame;

    my_cef_resource_request_handler_t *self = (void *)_self;

    cef_string_userfree_utf16_t url16 = request->get_url(request);
    cef_string_utf8_t url8 = {0};

    cef_string_utf16_to_utf8(url16->str, url16->length, &url8);
    cef_string_userfree_utf16_free(url16);

    my_cef_resource_handler_t *my_resource = my_cef_resource_handler_create();

    if (my_resource != NULL) {
        my_resource->resource = find_resource(self, url8.str, url8.length);

        if (my_resource->resource != NULL) {
            return &my_resource->handler;
        }
    }

    return NULL;
}

extern bool my_cef_resource_request_handler_init(my_cef_resource_request_handler_t *self) {
    assert(self != NULL);

    self->resources = NULL;
    self->resources_count = 0;

    char root[FILENAME_MAX];
    const char resources[] = "/"G_DIR_RESOURCES;
    char cwd[sizeof(root) - (sizeof(resources) - 1)];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("getcwd");
        return false;
    }

    int printed = snprintf(root, sizeof(root), "%s%s", cwd, resources);
    assert(printed >= 0);

    if ((size_t)printed >= sizeof(root)) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "snprintf: Output was truncated\n");
        return false;
    }

    load_files(self, root);

    self->handler.base.size = sizeof(self->handler);
    self->handler.get_resource_handler = get_resource_handler;

    return true;
}

extern void my_cef_resource_request_handler_deinit(my_cef_resource_request_handler_t *self) {
    assert(self != NULL);

    for (size_t i = 0; i < self->resources_count; ++i) {
        Resource *resource = self->resources + i;
        free(*(char **)&resource->arena);
        cef_string_userfree_utf16_free(resource->mime);
    }

    free(self->resources);
    self->resources = NULL;
    self->resources_count = 0;
}
