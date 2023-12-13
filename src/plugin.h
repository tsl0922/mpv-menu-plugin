// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_H
#define MPV_PLUGIN_H

#include <windows.h>
#include <mpv/client.h>

struct plugin_ctx {
    mpv_handle *mpv;

    HWND hwnd;
    HMENU hmenu;
    WNDPROC wnd_proc;
};

#endif