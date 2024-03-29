// Copyright (c) 2023-2024 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_MENU_H
#define MPV_PLUGIN_MENU_H

#include "plugin.h"

#define MENU_DATA_PROP "user-data/menu/items"

void update_menu(plugin_ctx *ctx, mpv_node *node);
void show_menu(plugin_ctx *ctx, POINT *pt);
void handle_menu(plugin_ctx *ctx, UINT id);

#endif
