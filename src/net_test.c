/* ---------------------------------------------------------------------------
 * AIKON-VXP networking chunk 1: bare plain-HTTP connectivity test.
 *
 * Opens a plain TCP connection to a server, sends a minimal HTTP/1.1 GET,
 * and shows whatever comes back on screen. No TLS yet (deliberately -
 * get the dumbest possible path working first, add encryption once this
 * is proven on real hardware). Built against the real MediaTek MRE SDK
 * headers (vmsock.h / vmgraph.h / vmchset.h), not invented APIs.
 *
 * EDIT SERVER_HOST / SERVER_PORT / SERVER_PATH below before building.
 * ------------------------------------------------------------------------ */

#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include "vmsock.h"
#include "vm4res.h"

/* ---- configure before building --------------------------------------- */
#define SERVER_HOST   "example.com"   /* your server's plain-HTTP host/IP */
#define SERVER_PORT   80
#define SERVER_PATH   "/ping"          /* a small plain-text/HTML endpoint */
/* VM_TCP_APN_CMNET works with most Bangladeshi carrier default data
 * settings; switch to VM_TCP_APN_CMWAP only if your carrier needs the
 * old WAP-proxy APN instead. */
#define SERVER_APN    VM_TCP_APN_CMNET

#define RECV_BUF_SIZE   768
#define WIDE_BUF_SIZE   768
#define CHARS_PER_LINE  28   /* rough wrap width for a small feature-phone screen */

VMINT layer_hdl[1];

static VMINT   g_tcp_handle = -1;
static char    g_recv_buf[RECV_BUF_SIZE];
static VMINT   g_recv_len = 0;
static char    g_status[160] = "Starting...";

void handle_sysevt(VMINT message, VMINT param);
void handle_keyevt(VMINT event, VMINT keycode);
void handle_penevt(VMINT event, VMINT x, VMINT y);
static void draw_status(void);
static void start_connection(void);
static void tcp_callback(VMINT handle, VMINT event);
static void set_status(const char* text);

void vm_main(void) {
    layer_hdl[0] = -1;

    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_keyevt);
    vm_reg_pen_callback(handle_penevt);

    vm_res_init();
}

void handle_sysevt(VMINT message, VMINT param) {
    switch (message) {
    case VM_MSG_CREATE:
    case VM_MSG_ACTIVE:
        layer_hdl[0] = vm_graphic_create_layer(0, 0,
            vm_graphic_get_screen_width(),
            vm_graphic_get_screen_height(),
            -1);
        vm_graphic_set_clip(0, 0,
            vm_graphic_get_screen_width(),
            vm_graphic_get_screen_height());

        if (message == VM_MSG_CREATE) {
            start_connection();
        }
        break;

    case VM_MSG_PAINT:
        draw_status();
        break;

    case VM_MSG_INACTIVE:
        if (layer_hdl[0] != -1) {
            vm_graphic_delete_layer(layer_hdl[0]);
            layer_hdl[0] = -1;
        }
        break;

    case VM_MSG_QUIT:
        if (g_tcp_handle != -1) {
            vm_tcp_close(g_tcp_handle);
            g_tcp_handle = -1;
        }
        if (layer_hdl[0] != -1) {
            vm_graphic_delete_layer(layer_hdl[0]);
            layer_hdl[0] = -1;
        }
        vm_res_deinit();
        break;
    }
}

void handle_keyevt(VMINT event, VMINT keycode) {
    if (g_tcp_handle != -1) {
        vm_tcp_close(g_tcp_handle);
        g_tcp_handle = -1;
    }
    if (layer_hdl[0] != -1) {
        vm_graphic_delete_layer(layer_hdl[0]);
        layer_hdl[0] = -1;
    }
    vm_exit_app();
}

void handle_penevt(VMINT event, VMINT x, VMINT y) {
    if (g_tcp_handle != -1) {
        vm_tcp_close(g_tcp_handle);
        g_tcp_handle = -1;
    }
    if (layer_hdl[0] != -1) {
        vm_graphic_delete_layer(layer_hdl[0]);
        layer_hdl[0] = -1;
    }
    vm_exit_app();
}

static void set_status(const char* text) {
    VMINT i = 0;
    while (text[i] != 0 && i < (VMINT)sizeof(g_status) - 1) {
        g_status[i] = text[i];
        i++;
    }
    g_status[i] = 0;
    draw_status();
}

static void start_connection(void) {
    set_status("Connecting to server...");
    g_recv_len = 0;
    g_tcp_handle = vm_tcp_connect(SERVER_HOST, SERVER_PORT, SERVER_APN, tcp_callback);
    if (g_tcp_handle < 0) {
        set_status("Connect failed to start.");
    }
}

static void tcp_callback(VMINT handle, VMINT event) {
    char request[256];
    VMINT req_len;
    VMINT n;

    switch (event) {
    case VM_TCP_EVT_CONNECTED:
        set_status("Connected. Sending request...");
        req_len = sprintf(request,
            "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
            SERVER_PATH, SERVER_HOST);
        vm_tcp_write(handle, request, req_len);
        break;

    case VM_TCP_EVT_CAN_READ:
        n = vm_tcp_read(handle, g_recv_buf + g_recv_len,
                         RECV_BUF_SIZE - g_recv_len - 1);
        if (n == VM_TCP_READ_EOF) {
            g_recv_buf[g_recv_len] = 0;
            set_status(g_recv_len > 0 ? g_recv_buf : "(empty reply)");
            vm_tcp_close(handle);
            g_tcp_handle = -1;
        } else if (n > 0) {
            g_recv_len += n;
            if (g_recv_len >= RECV_BUF_SIZE - 1) {
                g_recv_len = RECV_BUF_SIZE - 1;
            }
            g_recv_buf[g_recv_len] = 0;
            set_status(g_recv_buf);
        }
        break;

    case VM_TCP_EVT_PIPE_BROKEN:
        set_status("Connection broken.");
        g_tcp_handle = -1;
        break;

    case VM_TCP_EVT_HOST_NOT_FOUND:
        set_status("Server not found (DNS/host).");
        g_tcp_handle = -1;
        break;

    case VM_TCP_EVT_PIPE_CLOSED:
        if (g_recv_len > 0) {
            g_recv_buf[g_recv_len] = 0;
            set_status(g_recv_buf);
        } else {
            set_status("Connection closed, no reply.");
        }
        g_tcp_handle = -1;
        break;

    default:
        break;
    }
}

static void draw_status(void) {
    short wide[WIDE_BUF_SIZE];
    vm_graphic_color color;
    VMINT screen_w, screen_h, line_h;
    VMINT y;
    VMINT src_pos, line_start;
    VMINT total_len;

    if (layer_hdl[0] == -1) {
        return;
    }

    screen_w = vm_graphic_get_screen_width();
    screen_h = vm_graphic_get_screen_height();
    line_h = vm_graphic_get_character_height();

    color.vm_color_565 = VM_COLOR_WHITE;
    vm_graphic_setcolor(&color);
    vm_graphic_fill_rect_ex(layer_hdl[0], 0, 0, screen_w, screen_h);

    color.vm_color_565 = VM_COLOR_BLACK;
    vm_graphic_setcolor(&color);

    total_len = vm_ascii_to_ucs2(wide, WIDE_BUF_SIZE, g_status);

    y = 4;
    src_pos = 0;
    while (src_pos < total_len && y + line_h <= screen_h) {
        VMINT chunk = CHARS_PER_LINE;
        if (src_pos + chunk > total_len) {
            chunk = total_len - src_pos;
        }
        line_start = src_pos;
        vm_graphic_textout_to_layer(layer_hdl[0], 4, y, wide + line_start, chunk);
        src_pos += chunk;
        y += line_h + 1;
    }

    vm_graphic_flush_layer(layer_hdl, 1);
}
