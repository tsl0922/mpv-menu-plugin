// Copyright (c) 2023 tsl0922. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <mpv/client.h>

#include "menu.h"

void *talloc_ctx = NULL;
char *input_conf = NULL;
mpv_handle *mpv = NULL;
HWND hwnd = NULL;
WNDPROC wnd_proc = NULL;
HMENU hmenu = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CONTEXTMENU:
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            show_menu(hWnd, hmenu, pt);
            break;
        case WM_COMMAND:
            handle_menu(mpv, hmenu, LOWORD(wParam));
            break;
        default:
            break;
    }

    return CallWindowProc(wnd_proc, hWnd, uMsg, wParam, lParam);
}

void plugin_init(int64_t wid) {
    talloc_ctx = talloc_new(NULL);
    hwnd = (HWND)wid;
    hmenu = load_menu(talloc_ctx, input_conf);
    wnd_proc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}

void plugin_destroy() {
    if (hmenu != NULL) DestroyMenu(hmenu);
    if (hwnd != NULL && wnd_proc != NULL)
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wnd_proc);
    if (input_conf != NULL) free(input_conf);
    if (talloc_ctx != NULL) talloc_free(talloc_ctx);
}

MPV_EXPORT int mpv_open_cplugin(mpv_handle *handle) {
    mpv = handle;
    mpv_observe_property(mpv, 0, "window-id", MPV_FORMAT_INT64);

    while (mpv) {
        mpv_event *event = mpv_wait_event(mpv, -1);
        if (event->event_id == MPV_EVENT_SHUTDOWN) break;

        switch (event->event_id) {
            case MPV_EVENT_PROPERTY_CHANGE:
                mpv_event_property *prop = event->data;
                if (prop->format == MPV_FORMAT_INT64 &&
                    strcmp(prop->name, "window-id") == 0) {
                    int64_t wid = *(int64_t *)prop->data;
                    if (wid > 0) plugin_init(wid);
                }
                break;
            default:
                break;
        }
    }

    return 0;
}

void read_input_conf(HINSTANCE hinstDLL) {
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