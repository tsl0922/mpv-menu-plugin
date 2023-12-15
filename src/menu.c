// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "misc/bstr.h"
#include "menu.h"

#define MENU_PREFIX "#menu:"
#define MENU_PREFIX_UOSC "#!"

struct item_data {
    char *key;
    char *cmd;
};

wchar_t *mp_from_utf8(void *talloc_ctx, const char *s) {
    int count = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (count <= 0) abort();
    wchar_t *ret = talloc_array(talloc_ctx, wchar_t, count);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, ret, count);
    return ret;
}

static HMENU find_submenu(HMENU hmenu, wchar_t *name) {
    MENUITEMINFOW mii;
    int count = GetMenuItemCount(hmenu);

    for (int i = 0; i < count; i++) {
        memset(&mii, 0, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING;
        if (!GetMenuItemInfoW(hmenu, i, TRUE, &mii) || mii.cch == 0) continue;

        mii.cch++;
        wchar_t buf[mii.cch];
        mii.dwTypeData = buf;
        mii.fMask |= MIIM_SUBMENU;
        if (!GetMenuItemInfoW(hmenu, i, TRUE, &mii) || !mii.hSubMenu) continue;
        if (wcscmp(mii.dwTypeData, name) == 0) return mii.hSubMenu;
    }
    return NULL;
}

// escape & to && for menu title
static wchar_t *escape_title(void *talloc_ctx, bstr title) {
    bstr left, rest;
    bstr escaped = bstr0(NULL);
    left = bstr_split(title, "&", &rest);
    while (rest.len > 0) {
        bstr_xappend(NULL, &escaped, left);
        bstr_xappend(NULL, &escaped, bstr0("&&"));
        left = bstr_split(rest, "&", &rest);
    }
    bstr_xappend(NULL, &escaped, left);
    wchar_t *ret = mp_from_utf8(talloc_ctx, bstrdup0(escaped.start, escaped));
    talloc_free(escaped.start);
    return ret;
}

// format title as name\tkey
static wchar_t *format_title(void *talloc_ctx, bstr name, bstr key) {
    bstr title = bstrdup(NULL, name);
    if (key.len > 0 && !bstr_equals0(key, "_")) {
        bstr_xappend(NULL, &title, bstr0("\t"));
        bstr_xappend(NULL, &title, key);
    }
    wchar_t *ret = escape_title(talloc_ctx, title);
    talloc_free(title.start);
    return ret;
}

static void insert_menu(void *talloc_ctx, HMENU hmenu, bstr key, bstr cmd,
                        bstr text) {
    static int id = 0;
    bstr name, rest;
    name = bstr_split(text, ">", &rest);
    name = bstr_strip(name);

    MENUITEMINFOW mii;
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID;
    mii.wID = id++;

    if (rest.len == 0) {
        if (bstr_equals0(name, "-") || bstr_startswith0(name, "---")) {
            mii.fType = MFT_SEPARATOR;
            InsertMenuItemW(hmenu, -1, TRUE, &mii);
        } else {
            mii.fMask |= MIIM_STRING;
            if (cmd.len > 0 && !bstr_startswith0(cmd, "#")) {
                struct item_data *data = talloc_ptrtype(talloc_ctx, data);
                data->key = bstrdup0(data, key);
                data->cmd = bstrdup0(data, cmd);
                mii.fMask |= MIIM_DATA;
                mii.dwTypeData = format_title(talloc_ctx, name, key);
                mii.cch = wcslen(mii.dwTypeData);
                mii.dwItemData = (ULONG_PTR)data;
                InsertMenuItemW(hmenu, -1, TRUE, &mii);
            } else {
                mii.fMask |= MIIM_SUBMENU;
                mii.dwTypeData = escape_title(talloc_ctx, name);
                mii.cch = wcslen(mii.dwTypeData);
                mii.hSubMenu = find_submenu(hmenu, mii.dwTypeData);
                if (mii.hSubMenu == NULL) {
                    mii.hSubMenu = CreatePopupMenu();
                    InsertMenuItemW(hmenu, -1, TRUE, &mii);
                }
            }
        }
    } else {
        mii.fMask |= MIIM_STRING | MIIM_SUBMENU;
        mii.dwTypeData = escape_title(talloc_ctx, name);
        mii.cch = wcslen(mii.dwTypeData);
        mii.hSubMenu = find_submenu(hmenu, mii.dwTypeData);
        if (mii.hSubMenu == NULL) {
            mii.hSubMenu = CreatePopupMenu();
            InsertMenuItemW(hmenu, -1, TRUE, &mii);
        }
        insert_menu(talloc_ctx, mii.hSubMenu, key, cmd, rest);
    }
}

static bool split_menu(bstr line, bstr *left, bstr *right) {
    if (line.len == 0) return false;
    if (!bstr_split_tok(line, MENU_PREFIX, left, right))
        bstr_split_tok(line, MENU_PREFIX_UOSC, left, right);
    *left = bstr_strip(*left);
    *right = bstr_strip(*right);
    return right->len > 0;
}

HMENU load_menu(struct plugin_ctx *ctx, char *input) {
    HMENU hmenu = CreatePopupMenu();
    bstr data = bstr0(input);

    while (data.len) {
        bstr line = bstr_strip_linebreaks(bstr_getline(data, &data));
        line = bstr_lstrip(line);
        if (line.len == 0) continue;

        bstr key, cmd, left, right;
        if (bstr_eatstart0(&line, "#")) {
            key = bstr0(NULL);
            cmd = bstr_strip(line);
        } else {
            key = bstr_split(line, WHITESPACE, &cmd);
            cmd = bstr_strip(cmd);
        }
        if (split_menu(cmd, &left, &right))
            insert_menu(ctx, hmenu, key, cmd, right);
    }

    return hmenu;
}

void show_menu(struct plugin_ctx *ctx, POINT pt) {
    RECT rc;

    GetClientRect(ctx->hwnd, &rc);
    ScreenToClient(ctx->hwnd, &pt);

    if (PtInRect(&rc, pt)) {
        ClientToScreen(ctx->hwnd, &pt);
        TrackPopupMenuEx(ctx->hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y,
                         ctx->hwnd, NULL);
    }
}

void handle_menu(struct plugin_ctx *ctx, int id) {
    MENUITEMINFOW mii;
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;
    if (!GetMenuItemInfoW(ctx->hmenu, id, FALSE, &mii)) return;

    struct item_data *data = (struct item_data *)mii.dwItemData;
    if (data != NULL) mp_command_async(data->cmd);
}