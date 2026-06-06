#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../../cef/include/capi/cef_load_handler_capi.h"

#include "../../include/global.h"
#include "../../include/my_cef.h"

typedef struct _FileInfo {
    char *data;
    char *name;
    size_t data_len;
    size_t name_len;
} FileInfo;

typedef enum _ExtKind {
    EXT_ERROR = -1,
    EXT_JS = 0,
    EXT_CSS,
    EXT_HTML,
} ExtKind;

typedef struct _ExtInfo {
    const char *prefix;
    const char *suffix;
    const size_t prefix_len;
    const size_t suffix_len;
} ExtInfo;

#define EXTINFO(_prefix, _suffix) { \
        .prefix = (_prefix), \
        .suffix = (_suffix), \
        .prefix_len = (sizeof(_prefix) - 1), \
        .suffix_len = (sizeof(_suffix) - 1), \
    }

static bool is_js(const FileInfo *file_info) {
    const char js[] = ".js";
    const size_t js_len = sizeof(js) - 1;

    const char *file_name = file_info->name;
    const size_t file_name_len = file_info->name_len;

    if (file_name_len < js_len) {
        return false;
    }

    const char *file_ext = (file_name + file_name_len) - js_len;
    return strcmp(file_ext, js) == 0;
}

static bool is_css(const FileInfo *file_info) {
    const char css[] = ".css";
    const size_t css_len = sizeof(css) - 1;

    const char *file_name = file_info->name;
    const size_t file_name_len = file_info->name_len;

    if (file_name_len < css_len) {
        return false;
    }

    const char *file_ext = (file_name + file_name_len) - css_len;
    return strcmp(file_ext, css) == 0;
}

static bool is_html(const FileInfo *file_info) {
    const char htm[] = ".htm";
    const size_t htm_len = sizeof(htm) - 1;

    const char html[] = ".html";
    const size_t html_len = sizeof(html) - 1;

    const char *file_name = file_info->name;
    const size_t file_name_len = file_info->name_len;

    if (file_name_len < htm_len) {
        return false;
    }

    const char *file_end = file_name + file_name_len;
    return strcmp(file_end - htm_len, htm) == 0 || strcmp(file_end - html_len, html) == 0;
}

static bool read_file(const char *file_path, FileInfo *file_info) {
    assert(file_path != NULL);
    assert(file_info != NULL);

    const ExtInfo ext_infos[] = {
        [EXT_JS] = EXTINFO(
            "",
            "\n/* pseudoical */"
        ),
        [EXT_CSS] = EXTINFO(
            "{let d=document,s=d.createElement`style`;s.textContent=String.raw`\n",
            "\n`;addEventListener('load',_=>d.head.append(s),{once:1})}"
            "\n\n/* pseudoical */"
        ),
        [EXT_HTML] = EXTINFO(
            "{addEventListener('load',_=>document.body.insertAdjacentHTML('beforeend',String.raw`\n",
            "\n`),{once:1})}"
            "\n\n/* pseudoical */"
        ),
    };

    ExtKind ext_kind = EXT_ERROR;

    if (is_js(file_info)) {
        ext_kind = EXT_JS;
    } else if (is_css(file_info)) {
        ext_kind = EXT_CSS;
    } else if (is_html(file_info)) {
        ext_kind = EXT_HTML;
    } else {
        return false;
    }

    assert(ext_kind != EXT_ERROR);

    const ExtInfo *ext_info = ext_infos + ext_kind;

    FILE *file = fopen(file_path, "rb");

    if (file == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("fopen");
        return false;
    }

    char *arena = NULL;
    bool is_success = false;

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

    size_t extra_len = ext_info->prefix_len + ext_info->suffix_len;
    size_t total_len = data_len + extra_len;
    arena = malloc((total_len + 1) * sizeof(*arena));

    if (arena == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("malloc");
        goto cleanup;
    }

    char *data_offset = arena + ext_info->prefix_len;
    long written = fread(data_offset, sizeof(*arena), data_len, file);

    if (written < data_len) {
        if (ferror(file) != 0) {
            fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
            perror("fread");
        } else {
            fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
            fprintf(stderr, "fread: Short item count (or zero)");
        }

        goto cleanup;
    }

    memcpy(arena, ext_info->prefix, ext_info->prefix_len);
    memcpy(data_offset + data_len, ext_info->suffix, ext_info->suffix_len);

    arena[total_len] = '\0';

    file_info->data = arena;
    file_info->data_len = total_len;

    arena = NULL;
    is_success = true;

cleanup:
    if (fclose(file) == EOF) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("fclose");
    }

    free(arena);
    return is_success;
}

static bool inject_script(cef_frame_t *frame, const char *local_path, FileInfo *file_info) {
    assert(frame != NULL);
    assert(local_path != NULL);
    assert(file_info != NULL);

    cef_string_utf16_t code = {0};
    cef_string_utf8_to_utf16(file_info->data, file_info->data_len, &code);

    static const char scheme[] = "userscript://"G_DIR_SCRIPTS"/";
    char script_url[sizeof(scheme) + FILENAME_MAX];

    int printed = snprintf(script_url, sizeof(script_url), "%s%s", scheme, local_path);
    assert(printed >= 0);

    if ((size_t)printed >= sizeof(script_url)) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "snprintf: Output was truncated\n");
        return false;
    }

    cef_string_utf16_t script_url16 = {0};
    cef_string_utf8_to_utf16(script_url, strlen(script_url), &script_url16);

    frame->execute_java_script(frame, &code, &script_url16, 0);

    return true;
}

static bool load_scripts(cef_frame_t *frame, const char *curr_path, size_t root_len) {
    assert(frame != NULL);
    assert(curr_path != NULL);

    DIR *dir = opendir(curr_path);

    if (dir == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("opendir");
        return false;
    }

    struct dirent *ent;
    char full_path[FILENAME_MAX];
    struct stat st;
    FileInfo file_info;

    while ((ent = readdir(dir)) != NULL) {
        const char *file_name = file_info.name = ent->d_name;

        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            continue;
        }

        int printed = snprintf(full_path, sizeof(full_path), "%s/%s", curr_path, file_name);
        assert(printed >= 0);

        if ((size_t)printed >= sizeof(full_path)) {
            fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
            fprintf(stderr, "snprintf: Output was truncated\n");
            continue;
        }

        if (stat(full_path, &st) == -1) {
            fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
            perror("stat");
            continue;
        }

        file_info.name_len = strlen(file_name);

        if (S_ISDIR(st.st_mode)) {
            load_scripts(frame, full_path, root_len);
        } else if (read_file(full_path, &file_info)) {
            inject_script(frame, full_path + root_len + 1, &file_info);
            free(file_info.data);
            file_info.data = NULL;
        }
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("closedir");
    }

    return true;
}

static void on_load_start(
    struct _cef_load_handler_t *self,
    struct _cef_browser_t *browser,
    struct _cef_frame_t *frame,
    cef_transition_type_t transition_type
) {
    (void)self;
    (void)browser;
    (void)transition_type;

    cef_string_userfree_utf16_t url16 = frame->get_url(frame);
    cef_string_utf8_t url8 = {0};

    cef_string_utf16_to_utf8(url16->str, url16->length, &url8);
    cef_string_userfree_utf16_free(url16);

    const char url[] = G_URL;

    if (strncmp(url8.str, url, sizeof(url) - 1) != 0) {
        return;
    }

    char root[FILENAME_MAX];
    const char scripts[] = "/"G_DIR_SCRIPTS;
    char cwd[sizeof(root) - (sizeof(scripts) - 1)];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("getcwd");
        return;
    }

    int printed = snprintf(root, sizeof(root), "%s%s", cwd, scripts);
    assert(printed >= 0);

    if ((size_t)printed >= sizeof(root)) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "snprintf: Output was truncated\n");
        return;
    }

    load_scripts(frame, root, strlen(root));
}

extern void my_cef_load_handler_init(my_cef_load_handler_t *self) {
    static_assert(sizeof(self->handler) == sizeof(cef_load_handler_t));
    assert(self != NULL);

    self->handler.base.size = sizeof(self->handler);
    self->handler.on_load_start = on_load_start;
}
