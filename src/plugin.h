// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_H
#define MPV_PLUGIN_H

#include <windows.h>
#include <mpv/client.h>
#include "misc/dispatch.h"
#include "types.h"

typedef struct {
    bool uosc;  // use uosc menu syntax
} plugin_config;

typedef struct {
    plugin_config *conf;                 // plugin config
    struct mp_dispatch_queue *dispatch;  // dispatch queue

    mpv_handle *mpv;  // mpv client handle
    mp_state *state;  // cached mpv properties

    HWND hwnd;         // window handle
    HMENU hmenu;       // menu handle
    WNDPROC wnd_proc;  // previous window procedure
} plugin_ctx;

wchar_t *mp_from_utf8(void *talloc_ctx, const char *s);
char *mp_get_prop_string(void *talloc_ctx, const char *prop);
char *mp_expand_path(void *talloc_ctx, char *path);
char *mp_read_file(void *talloc_ctx, char *path);
void mp_command_async(const char *args);

#endif