// clang menu.c -DMPV_CPLUGIN_DYNAMIC_SYM `pkg-config --cflags mpv` -shared -o menu.dll

#include <windows.h>
#include <windowsx.h>
#include <mpv/client.h>

#define MPV_EXPORT __declspec(dllexport)

mpv_handle *mpv = NULL;
HWND hwnd = NULL;
WNDPROC wnd_proc = NULL;
HMENU hmenu = NULL;

struct menu_item {
  char *name;
  char *command;
};

struct menu_item menu_items[] = {
    {"Play/Pause", "cycle pause"},
    {"Stop", "stop"},
    {"-", "ignore"},
    {"Fullscreen", "cycle fullscreen"},
    {"-", "ignore"},
    {"Exit", "quit"},
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CONTEXTMENU:
      RECT rc;
      POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

      GetClientRect(hWnd, &rc);
      ScreenToClient(hWnd, &pt);

      if (PtInRect(&rc, pt)) {
        ClientToScreen(hWnd, &pt);
        TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
      }
      break;
    case WM_COMMAND:
      WORD menu_id = LOWORD(wParam);

      if (menu_id >= 0 && menu_id < ARRAYSIZE(menu_items)) {
        mpv_command_string(mpv, menu_items[menu_id].command);
        return 0;
      }
      break;
    default:
      break;
  }

  return CallWindowProc(wnd_proc, hWnd, uMsg, wParam, lParam);
}

// TODO: Load menu from file
HMENU load_menu() {
  hmenu = CreatePopupMenu();

  for (int i = 0; i < ARRAYSIZE(menu_items); i++) {
    if (strcmp(menu_items[i].name, "-") == 0)
      AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
    else
      AppendMenu(hmenu, MF_STRING, i, menu_items[i].name);
  }

  return hmenu;
}

void plugin_init(int64_t wid) {
  hwnd = (HWND)wid;
  hmenu = load_menu();
  wnd_proc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}

void plugin_destroy() {
  if (hmenu) DestroyMenu(hmenu);
  if (hwnd != NULL && wnd_proc != NULL) SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wnd_proc);
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
        if (prop->format == MPV_FORMAT_INT64 && strcmp(prop->name, "window-id") == 0) {
          int64_t wid = *(int64_t *)prop->data;
          if (wid > 0) plugin_init(wid);
        }
        break;
      case MPV_EVENT_SHUTDOWN:
        plugin_destroy();
        break;
      default:
        break;
    }
  }

  return 0;
}
