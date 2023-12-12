// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_MENU_H
#define MPV_PLUGIN_MENU_H

#include <windows.h>
#include <mpv/client.h>
#include "mpv_talloc.h"

HMENU load_menu(void *talloc_ctx, char *input);
void show_menu(HWND hwnd, HMENU hmenu, POINT pt);
void handle_menu(mpv_handle *mpv, HMENU hmenu, int id);

#endif