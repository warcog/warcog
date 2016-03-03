#include <stdio.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "../chat.h"
static chat_t chat;

enum {
    _chat,
    _gamelist,
    _settings,
    _list,
    _listsub,
    _main,
    num_class,
};

static const POINT min_size = {800, 480};

static const char* class_name[] = {
    "my_chat",
    "my_gamelist",
    "my_settings",
    "my_list",
    "my_listsub",
    "my_main",
};

static const char* tab_name[] = {
    "Chat",
    "Gamelist",
    "Settings",
};

static const char* col_name[] = {
    "Game name",
    "Map name",
    "Host",
    "Players",
    "Address",
    "Ping",
};

static HFONT font;
static int font_height, line_height;

//static chat_t chat;

static HWND tabs, tab[3];
static int current_tab;

static HWND edit_chat, edit_input, button_send, list_users;

#define sendmsg(x, y, z, w) SendMessage(x, y, (WPARAM) z, (LPARAM) w)
#define sendmsgw(x, y, z, w) SendMessageW(x, y, (WPARAM) z, (LPARAM) w)
#define setpos(hwnd, x, y, w, h) SetWindowPos(hwnd, 0, x, y, w, h, SWP_NOZORDER)
#define setarea(hwnd, r) SetWindowPos(hwnd, 0, (r)->left, (r)->top, (r)->right - (r)->left, \
                                      (r)->bottom - (r)->top, SWP_NOZORDER)
#define destroy(hwnd) DestroyWindow(hwnd)

static void rect(RECT *r, long left, long top, long right, long bottom)
{
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;
}

static void add_tab(HWND tabs, const char *name, int id)
{
    TCITEM tie;

    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    tie.iImage = -1;
    tie.pszText = (char*) name;
    TabCtrl_InsertItem(tabs, id, &tie);
}

static HWND child(const char *class, const char *name, DWORD style, HWND parent, int id)
{
    HWND hwnd;

    hwnd = CreateWindow(class, name, WS_CHILD | style, 0, 0, 0, 0, parent, (HMENU)(size_t) id, 0, 0);
    sendmsg(hwnd, WM_SETFONT, font, 0);

    return hwnd;
}
#define childv(c, n, s, p, i) child(c, n, (s) | WS_VISIBLE, p, i)

#define proc(x) static LRESULT CALLBACK x##_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
proc(main)
{
    RECT area;
    int i;
    NMHDR *p;

    switch (msg) {
    case WM_USER:
        chat_recv(&chat);
        break;
    case WM_TIMER:
        chat_timer(&chat);
        break;
    case WM_SIZE:
        rect(&area, 0, 0, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        setarea(tabs, &area);

        TabCtrl_AdjustRect(tabs, 0, &area);
        for (i = 0; i < 3; i++)
            setarea(tab[i], &area);
        break;
    case WM_NOTIFY:
        p = (NMHDR*) lparam;
        if (p->code == TCN_SELCHANGE) {
            ShowWindow(tab[current_tab], SW_HIDE);
            current_tab = TabCtrl_GetCurSel(tabs);
            ShowWindow(tab[current_tab], SW_SHOW);
            SetFocus(0);
        }
        break;
    case WM_GETMINMAXINFO:
         ((MINMAXINFO*) lparam)->ptMinTrackSize = min_size;
        break;
    case WM_CREATE:
        tabs = child(WC_TABCONTROL, 0, WS_VISIBLE, hwnd, 0);

        for (i = 0; i < 3; i++) {
            add_tab(tabs, tab_name[i], i);
            tab[i] = child(class_name[i], 0, i == 0 ? WS_VISIBLE : 0, hwnd, 0);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
         return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
}

void chat_add(char *msg)
{
    uint32_t start, end, len;

    sendmsg(edit_chat, EM_GETSEL, &start, &end);
    len = GetWindowTextLength(edit_chat);
    sendmsg(edit_chat, EM_SETSEL, len, len);
    if (len) {
        sendmsg(edit_chat, EM_REPLACESEL, 0, "\r\n");
    }
    sendmsg(edit_chat, EM_REPLACESEL, 0, msg);

    if(start != len || end != len) {
        len = GetWindowTextLength(edit_chat);
        sendmsg(edit_chat, EM_SETSEL, len, len); /* selection end */
    } else {
        sendmsg(edit_chat, EM_SETSEL, start, end); /* restore selection */
    }
}

void chat_addw(wchar_t *msg)
{
    uint32_t start, end, len;

    sendmsgw(edit_chat, EM_GETSEL, &start, &end);
    len = GetWindowTextLengthW(edit_chat);
    sendmsgw(edit_chat, EM_SETSEL, len, len);
    if (len) {
        sendmsg(edit_chat, EM_REPLACESEL, 0, "\r\n");
    }
    sendmsgw(edit_chat, EM_REPLACESEL, 0, msg);

    if(start != len || end != len) {
        len = GetWindowTextLengthW(edit_chat);
        sendmsgw(edit_chat, EM_SETSEL, len, len); /* selection end */
    } else {
        sendmsgw(edit_chat, EM_SETSEL, start, end); /* restore selection */
    }
}

LRESULT CALLBACK subproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    printf("%u %u\n", msg, (int) wparam);

    return DefSubclassProc(hwnd, msg, wparam, lparam);
}

proc(chat)
{
#define EDIT_CHAT_STYLE (WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY)
#define LIST_STYLE (WS_BORDER | WS_VSCROLL | LBS_NOINTEGRALHEIGHT)

    HDC hdc;
    COLORREF color;
    int id, code, len, tmp, w, h;
    wchar_t text[256];
    char str[250];

    if(msg == WM_CTLCOLORSTATIC) {
        hdc = (HDC) wparam;
        color = RGB(255,255,255);
        SetBkColor(hdc, color);
        SetDCBrushColor(hdc, color);
        return (LRESULT)GetStockObject(DC_BRUSH);
    } else if(msg == WM_COMMAND) {
        id = LOWORD(wparam);
        code = HIWORD(wparam);

        if(id == 0) {
            if(code == EN_SETFOCUS) {
                HideCaret(edit_chat);
            }
        } else if(id == 1) {
            if(code == BN_CLICKED) {
                len = GetWindowTextW(edit_input, text, 256);
                len = WideCharToMultiByte(CP_UTF8, 0, text, len, str, 250, 0, 0);
                if (chat_send(&chat, (void*) str, len))
                    SetWindowText(edit_input, "");
            }
        }
    } else if(msg == WM_SIZE) {
        if (!lparam)
            return 0;

        w = GET_X_LPARAM(lparam); h = GET_Y_LPARAM(lparam);
        tmp = font_height + 4;

        setpos(edit_chat, 0, 0, w - 120, h - tmp - 1);
        setpos(edit_input, 0, h - tmp, w - 200, tmp);
        setpos(button_send, w - 200, h - tmp - 1, 80, tmp + 2);
        setpos(list_users, w - 120, 0, 120, h);
    } else if(msg == WM_CREATE) {
        edit_chat = child(WC_EDIT, NULL, WS_VISIBLE | EDIT_CHAT_STYLE, hwnd, 0);
        edit_input = child(WC_EDIT, NULL, WS_VISIBLE | WS_TABSTOP | WS_BORDER, hwnd, 0);
        button_send = child(WC_BUTTON, "Send", WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, hwnd, 1);
        list_users = child(WC_LISTBOX, NULL, WS_VISIBLE | LIST_STYLE, hwnd, 0);

        SetWindowSubclass(edit_input, subproc, 0, 0);
    } else {
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static HWND header, scroll, listdraw;
static int column_x[6], selected, scroll_y, scroll_h;

static HWND gamelist;
//static bool sort_order;
//static int sort_column; //LVM_GETSELECTEDCOLUMN

static void text_range(HDC hdc, int x, int max, int y, const char *str, int len)
{
    int res;
    SIZE size;

    if (!GetTextExtentExPoint(hdc, str, len, max - x, &res, 0, &size))
        return;

    TextOut(hdc, x, y, str, res);
}

static int printaddr(char *str, uint32_t ip, uint16_t port)
{
    union { uint8_t b[4]; uint32_t i;} x = {.i = ip};
    return sprintf(str, "%u.%u.%u.%u:%u", x.b[0], x.b[1], x.b[2], x.b[3], htons(port));
}

static void draw_item(HDC hdc, int i)
{
    COLORREF old_color, old_bk, old_brush;
    int y;
    game_t *g;
    RECT area;
    char str[128];

    y = i * (font_height + 4) - scroll_y;
    g = &chat.list[chat.sort[i]];

    if (i == selected) {
        old_color = SetTextColor(hdc, RGB(255, 255, 255));
        old_bk = SetBkColor(hdc, RGB(51, 153, 255));
        old_brush = SetDCBrushColor(hdc, RGB(51, 153, 255));
    } else {
        old_color = old_bk = old_brush = 0; //warnings fix
    }

    rect(&area, 0, y, column_x[5], y + font_height + 4);
    FillRect(hdc, &area, GetStockObject(DC_BRUSH));

    text_range(hdc, 6,               column_x[0] - 6, y + 2, g->info.desc, strlen(g->info.desc));
    text_range(hdc, column_x[0] + 6, column_x[1] - 6, y + 2, g->info.name, strlen(g->info.name));
    text_range(hdc, column_x[1] + 6, column_x[2] - 6, y + 2, "<anon>", 6);


    text_range(hdc, column_x[2] + 6, column_x[3] - 6, y + 2,
               str, sprintf(str, "%u/%u", g->info.players, g->info.maxplayers));
    text_range(hdc, column_x[3] + 6, column_x[4] - 6, y + 2,
               str, printaddr(str, g->addr.ip, g->addr.port));
    text_range(hdc, column_x[4] + 6, column_x[5] - 6, y + 2,
               str, sprintf(str, "%i", g->info.key ? g->info.ping : -1));

    if (i == selected) {
        SetTextColor(hdc, old_color);
        SetBkColor(hdc, old_bk);
        SetDCBrushColor(hdc, old_brush);
    }
}

static void draw_items(HDC hdc)
{
    int i, y;
    for (i = scroll_y / 17, y = -(scroll_y % 17); i < chat.npinged && y < scroll_h; i++, y += 17)
        draw_item(hdc, i);
}

proc(listsub)
{
    PAINTSTRUCT ps;
    HDC hdc;
    HFONT old_font;
    int i, j, y;

    SCROLLINFO info;

    switch (msg) {
    case WM_SIZE:
        scroll_h = GET_Y_LPARAM(lparam);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        old_font = SelectObject(hdc, font);

        draw_items(hdc);

        SelectObject(hdc, old_font);
        EndPaint(hwnd, &ps);
        break;
    case WM_LBUTTONDOWN:
        y = GET_Y_LPARAM(lparam);
        i = (y + scroll_y) / 17;
        if (i == selected || i >= chat.npinged)
            break;

        hdc = GetDC(hwnd);
        old_font = SelectObject(hdc, font);

        j = selected;
        selected = i;

        draw_item(hdc, j);
        draw_item(hdc, i);

        SelectObject(hdc, old_font);
        ReleaseDC(hwnd, hdc);

        SetFocus(hwnd);
        break;
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;
    case WM_KEYDOWN:
        if (wparam == VK_UP)
            i = selected - 1;
        else if (wparam == VK_DOWN)
            i = selected + 1;
        else
            break;

        if (i < 0 || i >= chat.npinged)
            break;

        hdc = GetDC(hwnd);
        old_font = SelectObject(hdc, font);

        j = selected;
        selected = i;

        draw_item(hdc, j);
        draw_item(hdc, i);

        SelectObject(hdc, old_font);
        ReleaseDC(hwnd, hdc);
        break;
    case WM_MOUSEWHEEL:
        break;
        scroll_y -= (int) ((short) HIWORD(wparam)) * 60 / WHEEL_DELTA;
        if (scroll_y > 17 * chat.npinged - scroll_h)
            scroll_y = 17 * chat.npinged - scroll_h;
        if (scroll_y < 0)
            scroll_y = 0;

        info.cbSize = sizeof(info);
        info.fMask = SIF_POS;
        info.nPos = scroll_y;
        SetScrollInfo(scroll, SB_CTL, &info, 1);

        hdc = GetDC(listdraw);
        old_font = SelectObject(hdc, font);

        draw_items(hdc);

        SelectObject(hdc, old_font);
        ReleaseDC(listdraw, hdc);
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
}

proc(list)
{
    HDC hdc;
    RECT area;
    int i, y;
    HDITEM item;
    HFONT old_font;
    SCROLLINFO info;

    switch (msg) {
    case WM_MOUSEWHEEL:
        printf("wheel2\n");
        break;
    case WM_VSCROLL:
        y = HIWORD(wparam);
        switch (LOWORD(wparam)) {
        case SB_THUMBPOSITION:
            printf("scroll_end %u\n", y);

            info.cbSize = sizeof(info);
            info.fMask = SIF_POS;
            info.nPos = y;
            SetScrollInfo(scroll, SB_CTL, &info, 0);
            break;
        case SB_THUMBTRACK:
            printf("scroll %u\n", y);
            scroll_y = y;

            hdc = GetDC(listdraw);
            old_font = SelectObject(hdc, font);

            draw_items(hdc);

            SelectObject(hdc, old_font);
            ReleaseDC(listdraw, hdc);
            break;
        default:
            printf("unk %u\n", LOWORD(wparam));
            break;
        }
        break;
    case WM_SIZE:
        if (!lparam)
            return 0;

        rect(&area, 0, 0, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

        i = area.right;
        column_x[0] = i / 4;
        column_x[1] = column_x[0] + i / 4;
        column_x[2] = column_x[1] + (i * 4) / 32;
        column_x[3] = column_x[2] + (i * 3) / 32;
        column_x[4] = column_x[3] + (i * 6) / 32;
        column_x[5] = i - 17 + 1; //scrollw

        area.left = area.right - 17; //scrollw
        setarea(scroll, &area);

        info.cbSize = sizeof(info);
        info.fMask = SIF_PAGE | SIF_RANGE;
        info.nPage = area.bottom - 24;
        info.nMin = 0;
        info.nMax = 17 * chat.npinged - 1;//TODO
        if (info.nMax <= (int) info.nPage) {
            //info.fMask |= SIF_DISABLENOSCROLL;
        }
        SetScrollInfo(scroll, SB_CTL, &info, 0);

        area.left = 0;
        area.top = 24;
        area.right -= 17;
        setarea(listdraw, &area);

        area.top = 0;
        area.bottom = 24;
        setarea(header, &area);

        item.mask = HDI_WIDTH;
        for (i = 0; i < 6; i++) {
            item.cxy = column_x[i] - ((i == 0) ? 0 : column_x[i - 1]);
            Header_SetItem(header, i, &item);
        }
        InvalidateRect(header, 0, 0); //
        break;
    case WM_NOTIFY:
        break;
    case WM_CREATE:
        scroll = childv(WC_SCROLLBAR, 0, SBS_VERT, hwnd, 0);
        header = childv(WC_HEADER, 0, HDS_BUTTONS | HDS_NOSIZING, hwnd, 0);
        listdraw = childv(class_name[_listsub], 0, 0, hwnd, 0);

        item.mask = HDI_TEXT;
        item.fmt = 0;//HDF_SORTDOWN;
        for (i = 0; i < 6; i++) {
            item.pszText = (char*) col_name[i];
            Header_InsertItem(header, i, &item);
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
}

static HWND btn[3], edit[3];

static const char* info_str =
"Name: doto 5v5 apem pro only might be a too long name who knows\r\n"
"Map: DotA 1.6\r\n"
"Host: Dendi\r\n"
"Players: 10/10\r\n"
"Address: 127.0.0.1:1337\r\n"
"Ping: 145"
;

proc(gamelist)
{
    int x, y, w, h;

    switch (msg) {
    case WM_SIZE:
        if (!lparam)
            return 0;

        w = GET_X_LPARAM(lparam);
        h = GET_Y_LPARAM(lparam);
        setpos(gamelist, 0, 0, w - 200, h);

        x = w - 200;
        y = 0;

        setpos(edit[0], x + 2, y, 200 - 4, font_height * 8);
        y += font_height * 8;

        setpos(btn[0], x,       y, 100, line_height);
        setpos(btn[1], x + 100, y, 100, line_height);

        y += line_height * 10;

        setpos(btn[2], x + 50, y, 100, line_height);
        break;
    case WM_CREATE:
        gamelist = childv(class_name[_list], 0, 0, hwnd, 0);

        edit[0] = child(WC_EDIT, info_str, WS_VISIBLE | ES_MULTILINE | ES_READONLY, hwnd, 0);

        /*label[0] = childv(WC_STATIC, "Name: doto 5v5 apem pro only", SS_ENDELLIPSIS, hwnd, 0);
        label[1] = childv(WC_STATIC, "Map: DotA 1.6", SS_ENDELLIPSIS, hwnd, 0);
        label[2] = childv(WC_STATIC, "Host: Dendi", SS_ENDELLIPSIS, hwnd, 0);
        label[3] = childv(WC_STATIC, "Players: 10/10", SS_ENDELLIPSIS, hwnd, 0);
        label[4] = childv(WC_STATIC, "Address: 127.0.0.1:1337", SS_ENDELLIPSIS, hwnd, 0);
        label[5] = childv(WC_STATIC, "Ping: 145", SS_ENDELLIPSIS, hwnd, 0);*/

        btn[0] = childv(WC_BUTTON, "Refresh", WS_TABSTOP | BS_PUSHBUTTON, hwnd, 0);
        btn[1] = childv(WC_BUTTON, "Connect", WS_TABSTOP | BS_PUSHBUTTON, hwnd, 1);
        btn[2] = childv(WC_BUTTON, "Connect", WS_TABSTOP | BS_PUSHBUTTON, hwnd, 2);
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
}

proc(settings)
{
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void* class_proc[] = {
    chat_proc,
    gamelist_proc,
    settings_proc,
    list_proc,
    listsub_proc,
    main_proc,
};

static bool comctl_init(void)
{
    INITCOMMONCONTROLSEX icex = {
        .dwSize = sizeof(INITCOMMONCONTROLSEX),
        .dwICC = ICC_TAB_CLASSES,
    };

    return InitCommonControlsEx(&icex);
}

static bool class_init(const char *name, WNDPROC proc, HINSTANCE hinst)
{
    WNDCLASS wc = {
        .lpfnWndProc = proc,
        .hInstance = hinst,
        .hIcon = LoadIcon(0, IDI_APPLICATION),
        .hCursor = LoadCursor(0, IDC_ARROW),
        .hbrBackground = GetStockObject(WHITE_BRUSH),
        .lpszClassName = name,
    };

    return RegisterClass(&wc);

}

HWND init(HINSTANCE hinst)
{
    int i;
    HWND hwnd;

    if (!comctl_init())
        return 0;

    for (i = 0; i < num_class; i++)
        if (!class_init(class_name[i], class_proc[i], hinst))
            goto fail;

    font = GetStockObject(DEFAULT_GUI_FONT);
    if (!font)
        goto fail;

    {
        HDC hdc;
        TEXTMETRIC tm;
        HGDIOBJ old;

        hdc = GetDC(0);
        old = SelectObject(hdc, font);
        GetTextMetrics(hdc, &tm);
        SelectObject(hdc, old);
        ReleaseDC(0, hdc);

        font_height = tm.tmHeight;
        line_height = font_height + 4;
    }

    hwnd = CreateWindow(class_name[_main], "warcog-chat", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0, hinst, 0);
    if (!hwnd)
        goto fail;
    //ShowWindow(hwnd, SW_SHOW);

    return hwnd;
fail:
    while(--i >= 0)
        UnregisterClass(class_name[i], hinst);
    return 0;
}

void chat_userlist_cb(chat_t *chat)
{
    (void) chat;
}

void chat_gamelist_cb(chat_t *chat)
{
    SCROLLINFO info;

    if (selected >= chat->npinged)
        selected = -1;

    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE;
    info.nPage = scroll_h;//area.bottom - 24;
    info.nMin = 0;
    info.nMax = chat->npinged ? 17 * chat->npinged - 1 : 0;//TODO

    //if (info.nMax <= (int) info.nPage) {
        //info.fMask |= SIF_DISABLENOSCROLL;
    //}
    SetScrollInfo(scroll, SB_CTL, &info, 0);

    InvalidateRect(listdraw, 0, 0);
}

void chat_status_cb(chat_t *chat)
{
    printf("status: %u\n", chat->connected);
    if (chat->connected) {
        chat_add("Connected!");
        chat_gamelist_refresh(chat);
    } else {
        chat_add("Disconnected.");
    }
}

void chat_name_cb(chat_t *chat, uint32_t id, const char *name, uint8_t len)
{
    (void) chat;
    (void) name;
    (void) len;
    char str[32];

    sprintf(str, "<%u> joined.", id);
    chat_add(str);
}

void chat_leave_cb(chat_t *chat, uint32_t id)
{
    (void) chat;
    char str[32];

    sprintf(str, "<%u> disconnected.", id);
    chat_add(str);
}

void chat_msg_cb(chat_t *chat, uint32_t id, const char *msg, uint8_t len)
{
    (void) chat;
    wchar_t str[300];
    int i;

    i  = swprintf(str, 300, L"<%u>: ", id);
    i += MultiByteToWideChar(CP_UTF8, 0, msg, len, str + i, len);
    str[i] = 0;

    chat_addw(str);
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprev, PSTR cmd, int cmdshow)
{
    (void) hprev;
    (void) cmd;
    (void) cmdshow;

    MSG msg;
    WSADATA wsadata;
    HWND hwnd;

    /* initialize winsock */
    if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
        printf("WSAStartup failed\n");
        return 0;
    }

    hwnd = init(hinst);
    if (!hwnd)
        return 0;

    if (!chat_init(&chat, hwnd))
        goto fail; //destroy(hwnd)

    SetTimer(hwnd, 0, 1000 / 2, 0); //

    chat_add("Connecting...");

    while(GetMessage(&msg, 0, 0, 0) > 0) {
        if (IsDialogMessage(GetParent(GetFocus()), &msg)) {
            continue;
            /* Already handled by dialog manager */
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    chat_free(&chat);
fail:
    //cleanup
    return 0;
}
