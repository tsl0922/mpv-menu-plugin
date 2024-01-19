// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_H
#define MPV_PLUGIN_H

#include <stdbool.h>
#include <windows.h>
#include <mpv/client.h>
#include "misc/dispatch.h"

typedef struct {
    bool load;  // load menu on startup, default: yes
    bool uosc;  // use uosc menu syntax, default: no
} plugin_config;

typedef struct {
    plugin_config *conf;                 // plugin config
    struct mp_dispatch_queue *dispatch;  // dispatch queue

    mpv_handle *mpv;  // mpv client handle
    mpv_node *node;   // menu property node

    HWND hwnd;         // window handle
    HMENU hmenu;       // native menu handle
    void *hmenu_ctx;   // menu talloc context
    WNDPROC wnd_proc;  // previous window procedure
} plugin_ctx;

wchar_t *mp_from_utf8(void *talloc_ctx, const char *s);
char *mp_to_utf8(void *talloc_ctx, const wchar_t *s);
char *mp_get_prop_string(void *talloc_ctx, const char *prop);
char *mp_expand_path(void *talloc_ctx, char *path);
char *mp_read_file(void *talloc_ctx, char *path);
void mp_command_async(const char *args);

#endif