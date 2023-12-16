// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_MENU_H
#define MPV_PLUGIN_MENU_H

#include <windows.h>
#include <mpv/client.h>

#include "plugin.h"

HMENU load_menu(struct plugin_ctx *ctx);
void show_menu(struct plugin_ctx *ctx, POINT *pt);
void handle_menu(struct plugin_ctx *ctx, int id);

#endif