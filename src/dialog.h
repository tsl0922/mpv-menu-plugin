// Copyright (c) 2023-2024 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#ifndef MPV_PLUGIN_DIALOG_H
#define MPV_PLUGIN_DIALOG_H

#include "plugin.h"

char *open_dialog(void *talloc_ctx, plugin_ctx *ctx);
char **open_dialog_multi(void *talloc_ctx, plugin_ctx *ctx);
char *open_folder(void *talloc_ctx, plugin_ctx *ctx);
char *save_dialog(void *talloc_ctx, plugin_ctx *ctx);
#endif