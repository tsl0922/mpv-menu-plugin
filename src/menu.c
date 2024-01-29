// Copyright (c) 2023-2024 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "misc/bstr.h"
#include "misc/node.h"
#include "menu.h"

#define MENU_PREFIX "#menu:"
#define MENU_PREFIX_UOSC "#!"

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
void build_menu(void *talloc_ctx, HMENU hmenu, mpv_node *node) {
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

static bool is_separator(bstr text, bool uosc) {
    return bstr_equals0(text, "-") || (uosc && bstr_startswith0(text, "---"));
}

// return submenu node if exists, otherwise create a new one
static mpv_node *add_submenu(mpv_node *list, char *title) {
    for (int i = 0; i < list->u.list->num; i++) {
        mpv_node *item = &list->u.list->values[i];

        mpv_node *type_node = node_map_get(item, "type");
        if (!type_node || strcmp(type_node->u.string, "submenu") != 0) continue;

        mpv_node *title_node = node_map_get(item, "title");
        if (title_node && strcmp(title_node->u.string, title) == 0) {
            mpv_node *submenu = node_map_get(item, "submenu");
            if (!submenu)
                submenu = node_map_add(item, "submenu", MPV_FORMAT_NODE_ARRAY);
            return submenu;
        }
    }

    mpv_node *node = node_array_add(list, MPV_FORMAT_NODE_MAP);
    node_map_add_string(node, "title", title);
    node_map_add_string(node, "type", "submenu");
    return node_map_add(node, "submenu", MPV_FORMAT_NODE_ARRAY);
}

// parse menu line and add to menu node
static void parse_menu(mpv_node *list, bstr key, bstr cmd, bstr text,
                       bool uosc) {
    bstr name, rest, comment;

    name = bstr_split(text, ">", &rest);
    name = bstr_split(name, "#", &comment);
    name = bstr_strip(name);
    if (!name.len) return;

    void *tmp = talloc_new(NULL);

    if (!rest.len) {
        if (is_separator(name, uosc)) {
            mpv_node *node = node_array_add(list, MPV_FORMAT_NODE_MAP);
            node_map_add_string(node, "type", "separator");
        } else {
            bstr title = bstrdup(tmp, name);
            if (key.len > 0 && !bstr_equals0(key, "_")) {
                bstr_xappend(tmp, &title, bstr0("\t"));
                bstr_xappend(tmp, &title, key);
            }
            mpv_node *node = node_array_add(list, MPV_FORMAT_NODE_MAP);
            node_map_add_string(node, "title", bstrdup0(tmp, title));
            node_map_add_string(node, "cmd", bstrdup0(tmp, cmd));
        }
    } else {
        mpv_node *sub = add_submenu(list, bstrdup0(tmp, name));
        if (!comment.len) parse_menu(sub, key, cmd, rest, uosc);
    }

    talloc_free(tmp);
}

static bool split_menu(bstr line, bstr *left, bstr *right, bool uosc) {
    if (!line.len) return false;
    if (!bstr_split_tok(line, MENU_PREFIX, left, right)) {
        if (!uosc || !bstr_split_tok(line, MENU_PREFIX_UOSC, left, right))
            return false;
    }
    *left = bstr_strip(*left);
    *right = bstr_strip(*right);
    return right->len > 0;
}

// build menu node from input.conf
void load_menu(mpv_node *node, plugin_config *conf) {
    void *tmp = talloc_new(NULL);
    char *path = mp_get_prop_string(tmp, "input-conf");
    if (path == NULL || strlen(path) == 0) path = "~~/input.conf";

    bstr data = bstr0(mp_read_file(tmp, path));
    node_init(node, MPV_FORMAT_NODE_ARRAY, NULL);

    while (data.len > 0) {
        bstr line = bstr_strip_linebreaks(bstr_getline(data, &data));
        line = bstr_lstrip(line);
        if (!line.len) continue;

        bstr key, cmd, left, right;
        if (bstr_eatstart0(&line, "#")) {
            if (!conf->uosc) continue;
            key = bstr0(NULL);
            cmd = bstr_strip(line);
        } else {
            key = bstr_split(line, WHITESPACE, &cmd);
            cmd = bstr_strip(cmd);
        }
        if (split_menu(cmd, &left, &right, conf->uosc))
            parse_menu(node, key, cmd, right, conf->uosc);
    }

    talloc_free(tmp);
}

// update HMENU if menu node changed
void update_menu(plugin_ctx *ctx, mpv_node *node) {
    if (equal_mpv_node(ctx->node, node)) return;

    copy_mpv_node(ctx->node, node);

    if (ctx->hmenu) {
        while (GetMenuItemCount(ctx->hmenu) > 0)
            RemoveMenu(ctx->hmenu, 0, MF_BYPOSITION);
        talloc_free_children(ctx->hmenu_ctx);
        build_menu(ctx->hmenu_ctx, ctx->hmenu, ctx->node);
    }
}

// show menu at position if it is in window
void show_menu(plugin_ctx *ctx, POINT *pt) {
    if (!ctx->hmenu) return;

    RECT rc;
    GetClientRect(ctx->hwnd, &rc);
    ScreenToClient(ctx->hwnd, pt);
    if (!PtInRect(&rc, *pt)) return;

    ClientToScreen(ctx->hwnd, pt);
    TrackPopupMenuEx(ctx->hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt->x, pt->y,
                     ctx->hwnd, NULL);
}

// run mpv command stored in menu item data
void handle_menu(plugin_ctx *ctx, UINT id) {
    MENUITEMINFOW mii = {0};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;
    if (!GetMenuItemInfoW(ctx->hmenu, id, FALSE, &mii)) return;

    if (mii.dwItemData) mp_command_async((const char *)mii.dwItemData);
}