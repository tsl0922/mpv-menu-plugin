// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_MENU_H
#define MPV_PLUGIN_MENU_H

#include <windows.h>
#include <mpv/client.h>
#include "plugin.h"

#define MENU_DATA_PROP "user-data/menu/items"

void load_menu(mpv_node *node, plugin_config *conf);
void build_menu(void *talloc_ctx, HMENU hmenu, mpv_node *node);
void update_menu(plugin_ctx *ctx, mpv_node *node);
void show_menu(plugin_ctx *ctx, POINT *pt);
void handle_menu(plugin_ctx *ctx, int id);

#endif