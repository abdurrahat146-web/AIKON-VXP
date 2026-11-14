/* ShowIMSI: shows the SIM's IMSI (and whether a SIM is present) on screen,
 * so the exact number can be copied into manifest.json for signing.
 * Built WITHOUT an IMSI tag, to find out whether Series 30+ will launch
 * an untagged app at all. Needs a SIM inside the phone to show a number.
 * Press any key to exit. */

#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include "vmsim.h"
#include "vmtimer.h"
#include "vm4res.h"

VMINT layer_hdl[1];
static VMINT g_timer = -1;
static char g_line1[64] = "Reading SIM...";
static char g_line2[64] = "";
static char g_line3[64] = "";

void handle_sysevt(VMINT message, VMINT param);
void handle_keyevt(VMINT event, VMINT keycode);
void handle_penevt(VMINT event, VMINT x, VMINT y);
static void draw_screen(void);
static void poll_imsi(VMINT tid);

static void put_line(VMINT idx, VMINT y) {
    short wide[64];
    const char* src = idx == 1 ? g_line1 : (idx == 2 ? g_line2 : g_line3);
    VMINT n = vm_ascii_to_ucs2(wide, 64, (VMSTR)src);
    if (n > 0) {
        vm_graphic_textout_to_layer(layer_hdl[0], 4, y, wide, n);
    }
}

void vm_main(void) {
    layer_hdl[0] = -1;
    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_keyevt);
    vm_reg_pen_callback(handle_penevt);
    vm_res_init();
}

static void draw_screen(void) {
    vm_graphic_color color;
    VMINT w, h, lh;
    if (layer_hdl[0] == -1) return;
    w = vm_graphic_get_screen_width();
    h = vm_graphic_get_screen_height();
    lh = vm_graphic_get_character_height() + 2;
    color.vm_color_565 = VM_COLOR_WHITE;
    vm_graphic_setcolor(&color);
    vm_graphic_fill_rect_ex(layer_hdl[0], 0, 0, w, h);
    color.vm_color_565 = VM_COLOR_BLACK;
    vm_graphic_setcolor(&color);
    put_line(1, 4);
    put_line(2, 4 + lh);
    put_line(3, 4 + lh * 2);
    vm_graphic_flush_layer(layer_hdl, 1);
}

static void poll_imsi(VMINT tid) {
    VMSTR imsi;
    VMINT i;
    if (!vm_has_sim_card()) {
        sprintf(g_line1, "No SIM detected");
        g_line2[0] = 0;
        draw_screen();
        return;
    }
    imsi = vm_get_imsi();
    if (imsi == 0) {
        sprintf(g_line1, "Reading SIM...");
        draw_screen();
        return;
    }
    sprintf(g_line1, "IMSI:");
    for (i = 0; i < 63 && imsi[i] != 0; i++) {
        g_line2[i] = (char)imsi[i];
    }
    g_line2[i] = 0;
    sprintf(g_line3, "Any key = exit");
    draw_screen();
    if (g_timer != -1) {
        vm_delete_timer(g_timer);
        g_timer = -1;
    }
}

void handle_sysevt(VMINT message, VMINT param) {
    switch (message) {
    case VM_MSG_CREATE:
    case VM_MSG_ACTIVE:
        layer_hdl[0] = vm_graphic_create_layer(0, 0,
            vm_graphic_get_screen_width(),
            vm_graphic_get_screen_height(), -1);
        vm_graphic_set_clip(0, 0,
            vm_graphic_get_screen_width(),
            vm_graphic_get_screen_height());
        if (message == VM_MSG_CREATE) {
            g_timer = vm_create_timer(700, poll_imsi);
        }
        break;
    case VM_MSG_PAINT:
        draw_screen();
        break;
    case VM_MSG_INACTIVE:
        if (layer_hdl[0] != -1) {
            vm_graphic_delete_layer(layer_hdl[0]);
            layer_hdl[0] = -1;
        }
        break;
    case VM_MSG_QUIT:
        if (g_timer != -1) {
            vm_delete_timer(g_timer);
            g_timer = -1;
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
    if (g_timer != -1) {
        vm_delete_timer(g_timer);
        g_timer = -1;
    }
    if (layer_hdl[0] != -1) {
        vm_graphic_delete_layer(layer_hdl[0]);
        layer_hdl[0] = -1;
    }
    vm_exit_app();
}

void handle_penevt(VMINT event, VMINT x, VMINT y) {
    handle_keyevt(event, 0);
}
