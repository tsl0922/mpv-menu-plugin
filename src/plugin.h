// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_H
#define MPV_PLUGIN_H

#include <windows.h>
#include <mpv/client.h>

#include "osdep/threads.h"
#include "misc/dispatch.h"

struct plugin_config {
    bool uosc;  // use uosc menu syntax
};

struct plugin_ctx {
    struct plugin_config *conf;  // plugin config

    struct mp_dispatch_queue *dispatch;  // dispatch queue
    mp_thread thread;                    // dispatch thread
    bool terminate;                      // terminate thread

    mpv_handle *mpv;   // mpv client handle
    HWND hwnd;         // window handle
    HMENU hmenu;       // menu handle
    WNDPROC wnd_proc;  // previous window procedure
};

wchar_t *mp_from_utf8(void *talloc_ctx, const char *s);
char *mp_get_prop(void *talloc_ctx, const char *prop);
char *mp_expand_path(void *talloc_ctx, char *path);
char *mp_read_file(void *talloc_ctx, char *path);
void mp_command_async(const char *args);

#endif