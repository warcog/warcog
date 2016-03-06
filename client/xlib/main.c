#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>

#include "../game.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <pthread.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#define EVENT_MASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask \
                    | LeaveWindowMask | EnterWindowMask \
                    | KeyPressMask | KeyReleaseMask | StructureNotifyMask)

bool done;
Atom wm_delete_window;

Display *display;
Window win;

void quit(game_t *g)
{
    (void) g;
    done = 1;
}

void setcursor(game_t *g, uint8_t cursor)
{
    (void) g;
    static const unsigned int cursors[] = {XC_xterm, XC_target};
    Cursor c;

    if (!cursor) {
        XUndefineCursor(display, win);
    } else {
        c = XCreateFontCursor(display, cursors[cursor - 1]);
        XDefineCursor(display, win, c);
    }
}

bool lockcursor(game_t *g, bool lock)
{
    (void) g;
    if (lock)
        return (XGrabPointer(display, win, True, 0, GrabModeAsync, GrabModeAsync,
                             win, None, CurrentTime) == GrabSuccess);
    XUngrabPointer(display, CurrentTime);
    return 1;
}

bool thread(void (func)(void*), void *arg)
{
    bool res;
    pthread_t thread;
    pthread_attr_t attr;

    if (pthread_attr_init(&attr))
        return 0;

    pthread_attr_setstacksize(&attr, 1 << 16); /* 64k stack */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    res = pthread_create(&thread, &attr, (void*(*)(void*))func, arg) == 0;

    pthread_attr_destroy(&attr);

    return res;
}

#ifndef USE_VULKAN
#include <GL/glx.h>
GLXContext ctx;

bool create_window(int width, int height)
{
    static const int visual_attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24, ///ZERO
        GLX_STENCIL_SIZE, 8, ///ZERO
        GLX_DOUBLEBUFFER, True,
        None
    };

    static const int context_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        None
    };

    GLXFBConfig *fbc, fb;
    XVisualInfo *vi;
    XSetWindowAttributes swa;
    int cw_flags, num;
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;

    if (!(fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &num))) {
        printf("glXChooseFBConfig failed\n");
        return 0;
    }

    printf("%u matching configs\n", num);

    if (!num) {
        XFree(fbc);
        return 0;
    }

    fb = fbc[0];
    XFree(fbc);

    vi = glXGetVisualFromFBConfig(display, fb);
    printf("chosen visualid=0x%x\n", (int) vi->visualid);

    win = RootWindow(display, vi->screen);
    swa.colormap = XCreateColormap(display, win, vi->visual, AllocNone);
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = EVENT_MASK;
    cw_flags = (CWBorderPixel | CWColormap | CWEventMask);

    win = XCreateWindow(display, win, 0, 0, width, height, 0, vi->depth, InputOutput,
                        vi->visual, cw_flags, &swa);
    XFree(vi);
    XFreeColormap(display, swa.colormap);

    if (!win) {
        printf("XCreateWindow() failed\n");
        return 0;
    }

    XSetWMProtocols(display, win, &wm_delete_window, 1);
    XStoreName(display, win, "warcog");
    XMapWindow(display, win);

    glXCreateContextAttribsARB = (void*) glXGetProcAddressARB((uint8_t*)"glXCreateContextAttribsARB");
    if (!glXCreateContextAttribsARB)
        goto fail_free_window;

    ctx = glXCreateContextAttribsARB(display, fb, 0, True, context_attribs);
    if (!ctx) {
        printf("context creation failed\n");
        goto fail_free_window;
    }

    glXMakeCurrent(display, win, ctx);

    //g->win = win;
    //g->ctx = ctx;
    return 1;

fail_free_window:
    XDestroyWindow(display, win);
    return 0;
}


void destroy_window(void)
{
    glXMakeCurrent(display, 0, 0);
    glXDestroyContext(display, ctx);
    XDestroyWindow(display, win);
}

void swap_buffers(void)
{
    glXSwapBuffers(display, win);
}

#else

#include <vk.h>
#include <X11/Xlib-xcb.h>

VkResult create_window(int width, int height, VkInstance inst, VkSurfaceKHR *surface)
{
    VkResult err;
    XSetWindowAttributes swa;

    swa.event_mask = EVENT_MASK;

    win = RootWindow(display, DefaultScreen(display));
    win = XCreateWindow(display, win, 0, 0, width, height, 0, CopyFromParent, InputOutput,
                         CopyFromParent, CWEventMask, &swa);
    if (!win)
        return -1;

    XSetWMProtocols(display, win, &wm_delete_window, 1);
    XStoreName(display, win, "warcog");
    XMapWindow(display, win);

    err = vk_create_surface_xcb(inst, surface, .connection = XGetXCBConnection(display),
                                .window = win);
    if (err)
        XDestroyWindow(display, win);
    return err;
}

void destroy_window(VkInstance inst, VkSurfaceKHR surface)
{
    vk_destroy(inst, surface);
    XDestroyWindow(display, win);
}

#endif

static XIC ic_init(XIM *p_im, Display *display, Window win)
{
    XIM im;
    XIC ic;

    im = XOpenIM(display, 0, 0, 0);
    if (!im)
        return 0;

    ic = XCreateIC(im, XNInputStyle, XIMStatusNothing | XIMPreeditNothing, XNClientWindow, win, NULL);
    if (!ic)
        XCloseIM(im);
    *p_im = im;
    return ic;
}

static bool xlib_events(game_t *g, XIC ic)
{
    XEvent event;
    unsigned i, j;

    while (XPending(display)) {
        XNextEvent(display, &event);
        switch(event.type) {
        case KeyPress: {
            XKeyEvent *ev = &event.xkey;
            char buf[32];
            Status status;
            int res;
            KeySym sym;

            XQueryKeymap(display, buf);
            for (i = 0, j = 0; i < 256; i++)
                if (buf[i / 8] & (1 << (i & 7)))
                    g->keys[j++] = i;
            g->num_keys = j;

            game_keydown(g, ev->keycode);

            if (XFilterEvent(&event, None)) //are only KeyPress events filtered?
                break;

            res = Xutf8LookupString(ic, ev, buf, sizeof(buf), &sym, &status);
            if (res < 0)
                break; //can this happen?

            if (status == XLookupKeySym || status == XLookupBoth)
                game_keysym(g, sym);

            if (res)
                game_char(g, buf, res);
        } break;
        case KeyRelease:
            game_keyup(g, event.xkey.keycode);
            break;
        case EnterNotify:
            g->mouse_in = 1;
            break;
        case LeaveNotify:
            g->mouse_in = 0;
            break;
        case MotionNotify: {
            XMotionEvent *ev = &event.xmotion;
            game_motion(g, ev->x, ev->y);
        } break;
        case ButtonPress: {
            XButtonEvent *ev = &event.xbutton;

            if (ev->button < Button1)
                break;

            if (ev->button <= Button3)
                game_button(g, ev->x, ev->y, ev->button - Button1, ev->state);
            else if (ev->button <= Button5)
                game_wheel(g, (ev->button == Button4) ? 1.0 : -1.0);

        } break;
        case ButtonRelease: {
            XButtonEvent *ev = &event.xbutton;

            if (ev->button >= Button1 && ev->button <= Button3)
                game_buttonup(g, ev->button - Button1);

        } break;
        case ConfigureNotify: {
            XConfigureEvent *ev = &event.xconfigure;
            if (ev->width != g->width || ev->height != g->height)
                game_resize(g, ev->width, ev->height);
        } break;
        case ClientMessage: {
            XClientMessageEvent *ev = &event.xclient;
            if ((Atom) ev->data.l[0] == wm_delete_window) {
                return 0;
            } else {
                printf("unknown clientmessage\n");
            }
        } break;
        }
    }
    return 1;
}

int main(int argc, char *argv[])
{
    static game_t game;
    game_t *g = &game;

    XIM im;
    XIC ic;
    int width, height;

    setlocale(LC_ALL, "");

    width = 1024;
    height = 768;

    display = XOpenDisplay(0);
    if (!display)
        return 0;

    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);

    if (!game_init(&game, width, height, argc, argv))
        goto exit_closedisplay;

    ic = ic_init(&im, display, win);
    if (!ic)
        goto exit_game;

    do {
        game_frame(&game);
    } while (xlib_events(g, ic) && !done);

    XDestroyIC(ic);
    XCloseIM(im);

exit_game:
    game_exit(&game);
exit_closedisplay:
    XCloseDisplay(display);
    return 0;
}
