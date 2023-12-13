// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <mpv/client.h>

#include "mpv_talloc.h"
#include "menu.h"
#include "plugin.h"

struct plugin_ctx *ctx = NULL;
char *input_conf = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CONTEXTMENU:
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            show_menu(ctx, pt);
            break;
        case WM_COMMAND:
            handle_menu(ctx, LOWORD(wParam));
            break;
        default:
            break;
    }

    return CallWindowProcW(ctx->wnd_proc, hWnd, uMsg, wParam, lParam);
}

static void plugin_init(mpv_handle *handle, int64_t wid) {
    ctx = talloc_ptrtype(NULL, ctx);
    memset(ctx, 0, sizeof(*ctx));

    ctx->mpv = handle;
    ctx->hwnd = (HWND)wid;
    ctx->hmenu = load_menu(ctx, input_conf);
    ctx->wnd_proc =
        (WNDPROC)SetWindowLongPtrW(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}

static void plugin_destroy() {
    if (ctx != NULL) {
        DestroyMenu(ctx->hmenu);
        SetWindowLongPtrW(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)ctx->wnd_proc);
        talloc_free(ctx);
    }
    if (input_conf != NULL) free(input_conf);
}

MPV_EXPORT int mpv_open_cplugin(mpv_handle *handle) {
    mpv_observe_property(handle, 0, "window-id", MPV_FORMAT_INT64);

    while (handle) {
        mpv_event *event = mpv_wait_event(handle, -1);
        if (event->event_id == MPV_EVENT_SHUTDOWN) break;

        switch (event->event_id) {
            case MPV_EVENT_PROPERTY_CHANGE:
                mpv_event_property *prop = event->data;
                if (prop->format == MPV_FORMAT_INT64 &&
                    strcmp(prop->name, "window-id") == 0) {
                    int64_t wid = *(int64_t *)prop->data;
                    if (wid > 0) plugin_init(handle, wid);
                }
                break;
            default:
                break;
        }
    }

    return 0;
}

static void read_input_conf(HINSTANCE hinstDLL) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hinstDLL, path, MAX_PATH);
    PathCombineW(path, path, L"..\\..\\input.conf");

    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    char *buf = malloc(dwFileSize + 1);
    DWORD dwRead;
    ReadFile(hFile, buf, dwFileSize, &dwRead, NULL);
    buf[dwFileSize] = '\0';
    CloseHandle(hFile);

    input_conf = buf;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            read_input_conf(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            plugin_destroy();
            break;
        default:
            break;
    }
    return TRUE;
}