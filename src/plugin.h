// Copyright (c) 2023-2024 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_H
#define MPV_PLUGIN_H

#include <stdbool.h>
#include <windows.h>
#include <mpv/client.h>
#include "misc/dispatch.h"

typedef struct {
    mpv_handle *mpv;              // mpv client handle
    mp_dispatch_queue *dispatch;  // dispatch queue

    HWND hwnd;         // window handle
    HMENU hmenu;       // native menu handle
    void *hmenu_ctx;   // menu talloc context
    WNDPROC wnd_proc;  // previous window procedure
} plugin_ctx;

wchar_t *mp_from_utf8(void *talloc_ctx, const char *s);
char *mp_to_utf8(void *talloc_ctx, const wchar_t *s);
void mp_command_async(const char *args);

#endif