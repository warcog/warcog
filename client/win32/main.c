#include "../game.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <process.h>

#include <ks.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#include <assert.h>

#include "../util.h"

const GUID IID_IMMDeviceEnumerator={0xa95664d2,0x9614,0x4f35,{0xa7,0x46,0xde,0x8d,0xb6,0x36,0x17,0xe6}};
const CLSID CLSID_MMDeviceEnumerator={0xbcde0395,0xe52f,0x467c,{0x8e,0x3d,0xc4,0x57,0x92,0x91,0x69,46}};
const GUID IID_IAudioClient = {0x1cb9ad4c, 0xdbfa, 0x4c32, {0xb1,0x78, 0xc2,0xf5,0x68,0xa7,0x03,0xb2}};
const GUID IID_IAudioRenderClient={0xf294acfc,0x3146,0x4483,{0xa7,0xbf,0xad,0xdc,0xa7,0xc2,0x60,0xe2}};
const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {0x3, 0, 0x10, {0x80,0, 0,0xaa,0,0x38,0x9b,0x71}};

//#include "wgl.h"

static HWND hwnd;
static HINSTANCE hinst;
static RECT win;

static volatile bool _redraw, done;
static bool clipcursor, fullscreen, disabled, init_done;

static game_t game;

static TRACKMOUSEEVENT tme = {
    sizeof(TRACKMOUSEEVENT), TME_LEAVE, 0, 0
};

/*_Static_assert(sizeof(WPARAM) >= 8, "wparam size");
void postmessage(uint8_t id, uint8_t v8, uint16_t v16, uint32_t value, void *data)
{
    msg_t m = {{id, v8, v16, value}};
    PostMessage(hwnd, WM_USER, m.v, (LPARAM)data);
} */

void quit(game_t *g)
{
    (void) g;
    done = 1;
}

void setcursor(game_t *g, uint8_t cursor)
{
    (void) g;
    (void) cursor;
}

static bool apply_clip(HWND hwnd)
{
    RECT r;
    POINT p = {0};

    GetClientRect(hwnd, &r);
    ClientToScreen(hwnd, &p);

    r.left += p.x;
    r.right += p.x;
    r.top += p.y;
    r.bottom += p.y;
    ClipCursor(&r);
    return 1;
}

bool lockcursor(game_t *g, bool lock)
{
    (void) g;

    if (!lock) {
        ClipCursor(0);
        clipcursor = 0;
        return 1;
    }

    return clipcursor = apply_clip(hwnd);
}

bool thread(void (func)(void*), void *args)
{
    return (_beginthread(func, 0, args) != ~0ul);
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int x, y, w, h;
    RECT desktop;
    HWND wnd;
    uint8_t buf[256];
    unsigned i, j;

    x = GET_X_LPARAM(lparam);
    y = GET_Y_LPARAM(lparam);
    w = LOWORD(lparam);
    h = HIWORD(lparam);

    switch (msg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        disabled = (!w || !h);
        if (init_done && !disabled)
            game_resize(&game, w, h);
        break;
    case WM_MOVE:
        break;
    case WM_EXITSIZEMOVE:
        break;
    case WM_MOUSELEAVE:
        game.mouse_in = 0;
        break;
    case WM_MOUSEMOVE:
        if (!game.mouse_in) {
            TrackMouseEvent(&tme);
            game.mouse_in = 1;
        }

        game_motion(&game, x, y);

        if (clipcursor)
            apply_clip(hwnd); //TODO: shouldnt be here
        break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        game_button(&game, x, y, msg - WM_LBUTTONDOWN, 0);
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        game_buttonup(&game, msg - WM_LBUTTONUP);
        break;
    case WM_MOUSEWHEEL:
        game_wheel(&game, (double) (int16_t) HIWORD(wparam) / 120.0);
        break;
    case WM_CHAR:
        printf("char %u\n", (unsigned) wparam);
        game_char(&game, (char*) buf, unicode_to_utf8((char*) buf, wparam));
        break;
    case WM_KEYDOWN:
        GetKeyboardState(buf);
        for (i = 0, j = 0; i < 256; i++)
            if (buf[i] & 128)
                game.keys[j++] = i;
        game.num_keys = j;

        game_keydown(&game, wparam);
        game_keysym(&game, wparam);

        if (wparam == VK_F11) {
            if (!fullscreen) {
                GetWindowRect(hwnd, &win);
                SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                wnd = GetDesktopWindow();
                GetWindowRect(wnd, &desktop);
                SetWindowPos(hwnd, 0, 0, 0, desktop.right, desktop.bottom, 0);
            } else {
                SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                SetWindowPos(hwnd, 0, 0, 0, win.right - win.left, win.bottom - win.top, 0);
                SetWindowPos(hwnd, 0, win.left, win.top, win.right-win.left, win.bottom-win.top, 0);
            }
            fullscreen = !fullscreen;
        }
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    case WM_KEYUP:
        game_keyup(&game, wparam);
        break;
    case WM_USER:
        //m.v = wparam;
        //do_msg(m.id, m.v8, m.v16, m.value, (void*)lparam);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    return 0;
}


#define release(x) (x)->lpVtbl->Release(x)
#define check(x) if (FAILED(x)) goto failure

void audio_thread(void *args)
{
    audio_t *a;
    HRESULT hr;
    HANDLE event = 0;
    WAVEFORMATEX *fmt;
    IMMDeviceEnumerator *penum;
    IMMDevice *pdev;
    IAudioClient *acl = 0;
    IAudioRenderClient *rcl = 0;
    uint32_t padding, buf_size, period_size, samples;
    REFERENCE_TIME defp, minp;
    uint8_t *data;

    a = args;

    CoInitialize(0);

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
                          (void**)&penum);
    check(hr);

    hr = penum->lpVtbl->GetDefaultAudioEndpoint(penum, eRender, eConsole, &pdev);
    release(penum);

    check(hr);

    hr = pdev->lpVtbl->Activate(pdev, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&acl);
    release(pdev);
    check(hr);

    hr = acl->lpVtbl->GetMixFormat(acl, &fmt);
    check(hr);

    if (fmt->wFormatTag != WAVE_FORMAT_EXTENSIBLE || fmt->nChannels != 2 ||
        fmt->nSamplesPerSec != 48000 ||
        memcmp(&KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, &((WAVEFORMATEXTENSIBLE*)fmt)->SubFormat,
               sizeof(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))) {
        char msg[256];
        sprintf(msg, "Audio output format not supported: sound will be disabled. "
                "Change your audio output to 2-channel 24/32-bit 48000 Hz and restart the game "
                "(currently %u-channel %u-bit %u Hz).", fmt->nChannels, fmt->wBitsPerSample,
                (int) fmt->nSamplesPerSec);
        MessageBox(0, msg, "Audio Error", MB_OK);
        goto failure;
    }

    hr = acl->lpVtbl->Initialize(acl, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                 500000 /* 50ms */, 0, fmt, 0);
    check(hr);

    CoTaskMemFree(fmt);

    event = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(event);

    hr = acl->lpVtbl->SetEventHandle(acl, event);
    check(hr);

    hr = acl->lpVtbl->GetBufferSize(acl, &buf_size);
    check(hr);

    hr = acl->lpVtbl->GetDevicePeriod(acl, &defp, &minp);
    check(hr);

    hr = acl->lpVtbl->GetCurrentPadding(acl, &padding);
    check(hr);

    hr = acl->lpVtbl->GetService(acl, &IID_IAudioRenderClient, (void**)&rcl);
    check(hr);

    hr = acl->lpVtbl->Start(acl);
    check(hr);

    period_size = defp * 3 / 625;

    printf("audio buf_size=%u period_size=%u\n", buf_size, period_size);

    a->init = 1;
    do {
        hr = acl->lpVtbl->GetCurrentPadding(acl, &padding);
        check(hr);

        samples = buf_size - padding;
        //printf("%u\n", samples);

        hr = rcl->lpVtbl->GetBuffer(rcl, samples, &data);
        check(hr);

        float buf[samples];
        audio_getsamples(a, (sample_t*) data, buf, samples);

        hr = rcl->lpVtbl->ReleaseBuffer(rcl, samples, 0);
        check(hr);
    } while (WaitForSingleObject(event, 2000) == WAIT_OBJECT_0 && a->init);

failure:
    printf("audio thread exit\n");

    if (rcl) release(rcl);
    if (acl) release(acl);
    if (event) CloseHandle(event);

    a->quit = 1;
    return;
}

#ifndef USE_VULKAN

#include <GL/gl.h>
#include <GL/glext.h>
#include "wgl.h"
#include <GL/wglext.h>

static const int ctx_attribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0
};

static HGLRC hrc;
static HDC hdc;
static BOOL (WINAPI*wglSwapIntervalEXT)(int interval);

static bool linkgl(void)
{
#include "wgl_link.h"
	wglSwapIntervalEXT = (void*) wglGetProcAddress("wglSwapIntervalEXT");
    return 1;
}

bool create_window(int width, int height)
{
    RECT window;
    int w, h;
    int pxformat;
    HGLRC hrc_tmp;
    PIXELFORMATDESCRIPTOR pfd;
    const char *version;
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs;

    memset(&window, 0, sizeof(window));
    if (!AdjustWindowRect(&window, WS_OVERLAPPEDWINDOW, 0))
        return 0;

    w = width + window.right - window.left;
    h = height + window.bottom - window.top;

    //minSize.x = MIN_WIDTH + window.right - window.left;
    //minSize.y = MIN_HEIGHT + window.bottom - window.top;

    hwnd = CreateWindowW(NAME, NAME, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, w, h, 0, 0, hinst, 0);
    if (!hwnd)
        return 0;

    tme.hwndTrack = hwnd;

    hdc = GetDC(hwnd);
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DEPTH_DONTCARE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

    pxformat = ChoosePixelFormat(hdc, &pfd);
    if (!SetPixelFormat(hdc, pxformat, &pfd))
        goto fail_destroy_window;

    hrc_tmp = wglCreateContext(hdc);
    if (!hrc_tmp)
        goto fail_destroy_window;

    if (!wglMakeCurrent(hdc, hrc_tmp)) {
        wglDeleteContext(hrc_tmp);
        goto fail_destroy_window;
    }

    if (!linkgl()) {
        wglMakeCurrent(hdc, 0);
        wglDeleteContext(hrc_tmp);
        goto fail_destroy_window;
    }

    wglCreateContextAttribs = (void*) wglGetProcAddress("wglCreateContextAttribsARB");
    hrc = wglCreateContextAttribs ? wglCreateContextAttribs(hdc, 0, ctx_attribs) : 0;
    wglMakeCurrent(hdc, 0);
    wglDeleteContext(hrc_tmp);
    if (!hrc)
        goto fail_destroy_window;

    if (!wglMakeCurrent(hdc, hrc))
        goto fail_delete_context;

    version = (const char*) glGetString(GL_VERSION);
    printf("GL version: %s\n", version);

    //wglSwapIntervalEXT(1);
    return 1;

fail_delete_context:
    wglDeleteContext(hrc);
fail_destroy_window:
    DestroyWindow(hwnd);
    return 0;
}

void destroy_window(void)
{
    wglMakeCurrent(hdc, 0);
    wglDeleteContext(hrc);
    DestroyWindow(hwnd);
}

void swap_buffers(void)
{
    SwapBuffers(hdc);
}

#else

VkResult create_window(int width, int height, VkInstance inst, VkSurfaceKHR *surface)
{
    VkResult err;
    RECT window;
    int w, h;

    memset(&window, 0, sizeof(window));
    if (!AdjustWindowRect(&window, WS_OVERLAPPEDWINDOW, 0))
        return 0;

    w = width + window.right - window.left;
    h = height + window.bottom - window.top;

    hwnd = CreateWindowW(NAME, NAME, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, w, h, 0, 0, hinst, 0);
    if (!hwnd)
        return 0;

    tme.hwndTrack = hwnd;

    err = vk_create_surface_win32(inst, surface, .hinstance = hinst, .hwnd = hwnd);
    if (err)
        DestroyWindow(hwnd);
    return err;
}

void destroy_window(VkInstance inst, VkSurfaceKHR surface)
{
    vk_destroy(inst, surface);
    DestroyWindow(hwnd);
}

#endif

__attribute__ ((visibility ("default")))
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_inst, LPSTR cmd, int nCmdShow)
{
    (void) prev_inst;
    (void) cmd;
    (void) nCmdShow;

    WSADATA wsadata;
    WNDCLASSW wc;
    MSG msg;

    if (WSAStartup(MAKEWORD(2,2), &wsadata) != NO_ERROR)
        return 0;

    wc.style = CS_OWNDC;
    wc.lpfnWndProc = wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = NAME;

    if (!RegisterClassW(&wc))
        goto fail_wsa_cleanup;

    hinst = instance;

    if (!game_init(&game, 1024, 768, __argc, __argv))
        goto fail_unregister;

    init_done = 1;

    while (!done) {
        game_frame(&game);

        while (PeekMessageW(&msg, 0, 0, 0, 1)) {
            if (msg.message == WM_QUIT) {
                done = 1;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    game_exit(&game);

fail_unregister:
    UnregisterClassW(NAME, instance);
fail_wsa_cleanup:
    WSACleanup();
    return 0;
}
