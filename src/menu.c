// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "misc/bstr.h"
#include "menu.h"

#define MENU_PREFIX "#menu:"

struct item_data {
    char *key;
    char *cmd;
};

static HMENU find_submenu(HMENU hmenu, bstr name) {
    MENUITEMINFO mii;
    char buf[256];
    int count = GetMenuItemCount(hmenu);

    for (int i = 0; i < count; i++) {
        memset(&mii, 0, sizeof(mii));
        memset(buf, 0, sizeof(buf));

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING | MIIM_SUBMENU;
        mii.cch = 256;
        mii.dwTypeData = buf;
        GetMenuItemInfo(hmenu, i, TRUE, &mii);
        if (mii.hSubMenu == NULL) continue;

        if (bstr_equals0(name, mii.dwTypeData)) {
            return mii.hSubMenu;
        }
    }
    return NULL;
}

static char *format_title(void *talloc_ctx, bstr name, bstr key) {
    if (bstr_equals0(key, "_")) return bstrdup0(talloc_ctx, name);

    bstr title = bstrdup(NULL, name);
    bstr_xappend(NULL, &title, bstr0("\t"));
    bstr_xappend(NULL, &title, key);
    char *ret = bstrdup0(talloc_ctx, title);
    talloc_free(title.start);
    return ret;
}

static void insert_menu(void *talloc_ctx, HMENU hmenu, bstr key, bstr cmd,
                        bstr comment) {
    static int id = 0;
    bstr rest;
    bstr name = bstr_split(comment, ">", &rest);
    name = bstr_strip(name);

    MENUITEMINFO mii;
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID;
    mii.wID = id++;

    if (rest.len == 0) {
        if (bstr_equals0(name, "-")) {
            mii.fType = MFT_SEPARATOR;
        } else {
            struct item_data *data = talloc_ptrtype(talloc_ctx, data);
            data->key = bstrdup0(talloc_ctx, key);
            data->cmd = bstrdup0(talloc_ctx, cmd);

            mii.fMask |= MIIM_STRING | MIIM_DATA;
            mii.dwTypeData = format_title(talloc_ctx, name, key);
            mii.cch = strlen(mii.dwTypeData);
            mii.dwItemData = (ULONG_PTR)data;
        }

        InsertMenuItem(hmenu, -1, TRUE, &mii);
    } else {
        mii.fMask |= MIIM_STRING | MIIM_SUBMENU;
        mii.dwTypeData = bstrdup0(talloc_ctx, name);
        mii.cch = strlen(mii.dwTypeData);
        mii.hSubMenu = find_submenu(hmenu, name);

        if (mii.hSubMenu == NULL) {
            mii.hSubMenu = CreatePopupMenu();
            InsertMenuItem(hmenu, -1, TRUE, &mii);
        }

        insert_menu(talloc_ctx, mii.hSubMenu, key, cmd, rest);
    }
}

HMENU load_menu(void *talloc_ctx, char *input) {
    HMENU hmenu = CreatePopupMenu();
    bstr data = bstr0(input);

    while (data.len) {
        bstr line = bstr_strip_linebreaks(bstr_getline(data, &data));
        line = bstr_lstrip(line);
        if (line.len == 0 || bstr_startswith0(line, "#")) continue;

        bstr rest;
        bstr key = bstr_split(line, WHITESPACE, &rest);
        rest = bstr_strip(rest);
        if (rest.len == 0) continue;

        bstr comment;
        bstr cmd = bstr_split(rest, "#", &comment);
        cmd = bstr_strip(cmd);
        if (cmd.len == 0) continue;

        comment = bstr_strip(comment);
        if (comment.len == 0 || !bstr_eatstart0(&comment, MENU_PREFIX))
            continue;

        comment = bstr_strip(comment);
        if (comment.len == 0) continue;

        insert_menu(talloc_ctx, hmenu, key, cmd, comment);
    }

    return hmenu;
}

void show_menu(HWND hwnd, HMENU hmenu, POINT pt) {
    RECT rc;

    GetClientRect(hwnd, &rc);
    ScreenToClient(hwnd, &pt);

    if (PtInRect(&rc, pt)) {
        ClientToScreen(hwnd, &pt);
        TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0,
                       hwnd, NULL);
    }
}

void handle_menu(mpv_handle *mpv, HMENU hmenu, int id) {
    MENUITEMINFO mii;
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;
    if (!GetMenuItemInfo(hmenu, id, FALSE, &mii)) return;

    struct item_data *data = (struct item_data *)mii.dwItemData;
    if (data == NULL) return;
    int err = mpv_command_string(mpv, data->cmd);
    if (err < 0) MessageBox(NULL, data->cmd, mpv_error_string(err), MB_OK);
}