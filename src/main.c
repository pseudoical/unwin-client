#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>

#include "../cef/include/capi/cef_app_capi.h"

#include "../include/global.h"
#include "../include/my_cef.h"
#include "../include/my_pthread.h"
#include "../include/MyX.h"

static bool cef_settings_init(cef_settings_t *settings) {
    assert(settings != NULL);

    settings->size = sizeof(*settings);

    char dir[FILENAME_MAX];
    char cwd[sizeof(dir) - 32];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        perror("getcwd");
        return false;
    }

    int printed = snprintf(dir, sizeof(dir), "%s/" G_DIR_CEF_RESOURCES, cwd);
    assert(printed >= 0);

    if ((size_t)printed >= sizeof(dir)) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "snprintf: Output was truncated\n");
        return false;
    }

    cef_string_utf8_to_utf16(dir, strlen(dir), &settings->resources_dir_path);

    printed = snprintf(dir, sizeof(dir), "%s/" G_DIR_CEF_LOCALES, cwd);
    assert(printed >= 0);

    if ((size_t)printed >= sizeof(dir)) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "snprintf: Output was truncated\n");
        return false;
    }

    cef_string_utf8_to_utf16(dir, strlen(dir), &settings->locales_dir_path);

    printed = snprintf(dir, sizeof(dir), "%s/" G_DIR_CEF_CACHE, cwd);
    assert(printed >= 0);

    if ((size_t)printed >= sizeof(dir)) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "snprintf: Output was truncated\n");
        return false;
    }

    cef_string_utf8_to_utf16(dir, strlen(dir), &settings->root_cache_path);

    return true;
}

int main(int argc, char **argv) {
    cef_api_hash(CEF_API_VERSION, 0);

    cef_main_args_t main_args = { .argc = argc, .argv = argv };
    cef_app_t app = { .base.size = sizeof(app) };

    {
        int exit_code = cef_execute_process(&main_args, &app, NULL);

        if (exit_code >= 0) {
            return exit_code;
        }
    }

    cef_settings_t settings = {0};

    if (!cef_settings_init(&settings)) {
        return EXIT_FAILURE;
    }

    if (cef_initialize(&main_args, &settings, &app, NULL) == 0) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "cef_initialize: Initialization failed\n");
        return cef_get_exit_code();
    }

    XSetErrorHandler(MyXErrorHandler);
    XSetIOErrorHandler(MyXIOErrorHandler);

    XInitThreads();

    Display *display = XOpenDisplay(NULL);

    if (display == NULL) {
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
        fprintf(stderr, "XOpenDisplay: Did not succeed\n");
        return EXIT_FAILURE;
    }

    int screen = DefaultScreen(display);
    unsigned int width = DisplayWidth(display, screen) / 1.5;
    unsigned int height = DisplayHeight(display, screen) / 1.5;

    Window window = MyXCreateWindow(display, width, height, G_APP_NAME);

    cef_window_info_t window_info = {
        .size = sizeof(window_info),
        .bounds = { .width = width, .height = height },
        .parent_window = window,
    };

    my_cef_life_span_handler_t my_life_span = {0};
    my_cef_life_span_handler_init(&my_life_span);

    my_cef_keyboard_handler_t my_keyboard = {0};
    my_cef_keyboard_handler_init(&my_keyboard, display, window);

    my_cef_load_handler_t my_load = {0};
    my_cef_load_handler_init(&my_load);

    my_cef_resource_request_handler_t my_resource_request = {0};
    my_cef_resource_request_handler_init(&my_resource_request);

    my_cef_request_handler_t my_request = {0};
    my_cef_request_handler_init(&my_request, &my_resource_request);

    my_cef_client_t my_client = {0};
    my_cef_client_init(&my_client, &my_life_span, &my_keyboard, &my_load, &my_request);

    cef_string_utf16_t url = {0};
    cef_string_utf8_to_utf16(G_URL, sizeof(G_URL) - 1, &url);

    cef_browser_settings_t browser_settings = { .size = sizeof(browser_settings) };

    cef_browser_t *browser = cef_browser_host_create_browser_sync(
        &window_info, &my_client.client, &url, &browser_settings, NULL, NULL);

    assert(browser != NULL);

    bool is_pthread_xevent_init = false;
    bool is_pthread_xevent_start = false;
    int exit_code = EXIT_SUCCESS;
    my_pthread_xevent_arg xevent_arg;
    bool should_shutdown = false;

    is_pthread_xevent_init = my_pthread_xevent_init(&xevent_arg, display, window, browser);

    if (!is_pthread_xevent_init) {
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    is_pthread_xevent_start = my_pthread_xevent_start(&xevent_arg);

    if (!is_pthread_xevent_start) {
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    MyXMaximizeWindow(display, window);

    should_shutdown = true;

    cef_run_message_loop();

cleanup:
    if (is_pthread_xevent_start) {
        my_pthread_xevent_stop(&xevent_arg);
    }

    if (is_pthread_xevent_init) {
        my_pthread_xevent_deinit(&xevent_arg);
    }

    if (should_shutdown) {
        cef_shutdown();
    }

    my_cef_resource_request_handler_deinit(&my_resource_request);

    XDestroyWindow(display, window);

    XCloseDisplay(display);

    return exit_code;
}

// unwin = untitled window
