#include <windows.h>
#include <shellapi.h>

#define WM_TRAYICON (WM_USER + 1)

enum {
    ID_NEXT = 101,
    ID_PREV,
    ID_PLAY
};

NOTIFYICONDATAW g_trayData = { sizeof(NOTIFYICONDATAW) };

void autostart() {
    HKEY hKey;
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"MusicHotkeys", 0, REG_SZ, (BYTE*)path, (DWORD)(wcslen(path) + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
}

void send_media_key(WORD key) {
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = key;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = key;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, inputs, sizeof(INPUT));
}

void contextmenu(HWND hwnd) {
    POINT mousePos;
    GetCursorPos(&mousePos);
    HMENU hMenu = CreatePopupMenu();

    if (hMenu) {
        AppendMenuW(hMenu, MF_STRING, 2001, L"Выход");
        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, mousePos.x, mousePos.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

LRESULT CALLBACK WindowMessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
            contextmenu(hwnd);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 2001) {
            DestroyWindow(hwnd);
        }
        break;

    case WM_HOTKEY:
        if (wParam == ID_NEXT) send_media_key(VK_MEDIA_NEXT_TRACK);
        else if (wParam == ID_PREV) send_media_key(VK_MEDIA_PREV_TRACK);
        else if (wParam == ID_PLAY) send_media_key(VK_MEDIA_PLAY_PAUSE);
        break;

    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &g_trayData);
        UnregisterHotKey(hwnd, ID_NEXT);
        UnregisterHotKey(hwnd, ID_PREV);
        UnregisterHotKey(hwnd, ID_PLAY);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    autostart();

    const wchar_t* clsName = L"MusicHelperClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowMessageHandler;
    wc.hInstance = hInst;
    wc.lpszClassName = clsName;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, clsName, L"MusicHelper", 0, 0, 0, 0, 0, NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;

    RegisterHotKey(hwnd, ID_NEXT, MOD_CONTROL | MOD_ALT, 'K');
    RegisterHotKey(hwnd, ID_PREV, MOD_CONTROL | MOD_ALT, 'L');
    RegisterHotKey(hwnd, ID_PLAY, MOD_CONTROL | MOD_ALT, 'O');

    g_trayData.hWnd = hwnd;
    g_trayData.uID = 1;
    g_trayData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_trayData.uCallbackMessage = WM_TRAYICON;
    g_trayData.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcscpy_s(g_trayData.szTip, L"Управление музыкой");
    Shell_NotifyIconW(NIM_ADD, &g_trayData);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}