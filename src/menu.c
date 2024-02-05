// Copyright (c) 2023-2024 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <windows.h>
#include "misc/bstr.h"
#include "menu.h"

// escape & to && for menu title
static wchar_t *escape_title(void *talloc_ctx, char *title) {
    void *tmp = talloc_new(NULL);
    bstr left, rest;
    bstr escaped = bstr0(NULL);

    left = bstr_split(bstr0(title), "&", &rest);
    while (rest.len > 0) {
        bstr_xappend(tmp, &escaped, left);
        bstr_xappend(tmp, &escaped, bstr0("&&"));
        left = bstr_split(rest, "&", &rest);
    }
    bstr_xappend(tmp, &escaped, left);

    wchar_t *ret = mp_from_utf8(talloc_ctx, bstrdup0(tmp, escaped));
    talloc_free(tmp);
    return ret;
}

// append menu item to HMENU
static int append_menu(HMENU hmenu, UINT fMask, UINT fType, UINT fState,
                       wchar_t *title, HMENU submenu, void *data) {
    static UINT id = WM_USER + 100;
    MENUITEMINFOW mii = {0};

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | fMask;
    mii.wID = id++;

    if (fMask & MIIM_FTYPE) mii.fType = fType;
    if (fMask & MIIM_STATE) mii.fState = fState;
    if (fMask & MIIM_STRING) {
        mii.dwTypeData = title;
        mii.cch = wcslen(title);
    }
    if (fMask & MIIM_SUBMENU) mii.hSubMenu = submenu;
    if (fMask & MIIM_DATA) mii.dwItemData = (ULONG_PTR)data;

    return InsertMenuItemW(hmenu, -1, TRUE, &mii) ? mii.wID : -1;
}

// build fState for menu item creation
static int build_state(mpv_node *node) {
    int fState = 0;
    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node *item = &node->u.list->values[i];
        if (item->format != MPV_FORMAT_STRING) continue;

        if (strcmp(item->u.string, "hidden") == 0) {
            return -1;
        } else if (strcmp(item->u.string, "checked") == 0) {
            fState |= MFS_CHECKED;
        } else if (strcmp(item->u.string, "disabled") == 0) {
            fState |= MFS_DISABLED;
        }
    }
    return fState;
}

// build HMENU from mpv node
//
// node structure:
//
// MPV_FORMAT_NODE_ARRAY
//   MPV_FORMAT_NODE_MAP (menu item)
//      "type"           MPV_FORMAT_STRING
//      "title"          MPV_FORMAT_STRING
//      "cmd"            MPV_FORMAT_STRING
//      "state"          MPV_FORMAT_NODE_ARRAY[MPV_FORMAT_STRING]
//      "submenu"        MPV_FORMAT_NODE_ARRAY[menu item]
static void build_menu(void *talloc_ctx, HMENU hmenu, mpv_node *node) {
    if (node->format != MPV_FORMAT_NODE_ARRAY || node->u.list->num == 0) return;

    for (int i = 0; i < node->u.list->num; i++) {
        mpv_node *item = &node->u.list->values[i];
        if (item->format != MPV_FORMAT_NODE_MAP) continue;

        mpv_node_list *list = item->u.list;

        char *type = "";
        char *title = NULL;
        char *cmd = NULL;
        int fState = 0;
        HMENU submenu = NULL;

        for (int j = 0; j < list->num; j++) {
            char *key = list->keys[j];
            mpv_node *value = &list->values[j];

            switch (value->format) {
                case MPV_FORMAT_STRING:
                    if (strcmp(key, "title") == 0) {
                        title = value->u.string;
                    } else if (strcmp(key, "cmd") == 0) {
                        cmd = value->u.string;
                    } else if (strcmp(key, "type") == 0) {
                        type = value->u.string;
                    }
                    break;
                case MPV_FORMAT_NODE_ARRAY:
                    if (strcmp(key, "state") == 0) {
                        fState = build_state(value);
                    } else if (strcmp(key, "submenu") == 0) {
                        submenu = CreatePopupMenu();
                        build_menu(talloc_ctx, submenu, value);
                    }
                    break;
                default:
                    break;
            }
        }
        if (fState == -1) continue;

        if (strcmp(type, "separator") == 0) {
            append_menu(hmenu, MIIM_FTYPE, MFT_SEPARATOR, 0, NULL, NULL, NULL);
        } else {
            if (title == NULL || strlen(title) == 0) continue;

            UINT fMask = MIIM_STRING | MIIM_STATE;
            bool grayed = false;
            if (strcmp(type, "submenu") == 0) {
                if (submenu == NULL) submenu = CreatePopupMenu();
                fMask |= MIIM_SUBMENU;
                grayed = GetMenuItemCount(submenu) == 0;
            } else {
                fMask |= MIIM_DATA;
                grayed = cmd == NULL || cmd[0] == '\0' || cmd[0] == '#' ||
                         strcmp(cmd, "ignore") == 0;
            }
            int id = append_menu(hmenu, fMask, 0, (UINT)fState,
                                 escape_title(talloc_ctx, title), submenu,
                                 talloc_strdup(talloc_ctx, cmd));
            if (grayed) EnableMenuItem(hmenu, id, MF_BYCOMMAND | MF_GRAYED);
        }
    }
}

// update HMENU if menu node changed
void update_menu(plugin_ctx *ctx, mpv_node *node) {
    while (GetMenuItemCount(ctx->hmenu) > 0)
        RemoveMenu(ctx->hmenu, 0, MF_BYPOSITION);
    talloc_free_children(ctx->hmenu_ctx);
    build_menu(ctx->hmenu_ctx, ctx->hmenu, node);
}

// show menu at position if it is in window
void show_menu(plugin_ctx *ctx, POINT *pt) {
    RECT rc;
    GetClientRect(ctx->hwnd, &rc);
    ScreenToClient(ctx->hwnd, pt);
    if (!PtInRect(&rc, *pt)) return;

    ClientToScreen(ctx->hwnd, pt);
    mpv_command(ctx->mpv,
                (const char *[]){"script-message", "menu-open", NULL});
    TrackPopupMenuEx(ctx->hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt->x, pt->y,
                     ctx->hwnd, NULL);
    mpv_command(ctx->mpv,
                (const char *[]){"script-message", "menu-close", NULL});
}

// run mpv command stored in menu item data
void handle_menu(plugin_ctx *ctx, UINT id) {
    MENUITEMINFOW mii = {0};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;
    if (!GetMenuItemInfoW(ctx->hmenu, id, FALSE, &mii)) return;

    if (mii.dwItemData) mp_command_async((const char *)mii.dwItemData);
}
