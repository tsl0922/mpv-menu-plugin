// Copyright (c) 2023-2024 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_CLIPBOARD_H
#define MPV_PLUGIN_CLIPBOARD_H

#include "plugin.h"

char *get_clipboard(plugin_ctx *ctx, void *talloc_ctx);
void set_clipboard(plugin_ctx *ctx, const char *text);

#endif
