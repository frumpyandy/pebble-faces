/* Ninefold: a nod to TIC-9TTMM, bent into a wheel.
 *   - six 3x3 dot squares sit around the hour numeral, one per ten minutes,
 *     starting top-right and going clockwise
 *   - inside a square the dots light up one per minute (1..9)
 *   - when the tenth minute arrives the whole square turns accent
 * Hollow ring at the bottom = phone disconnected.  Accent turns red on low battery. */
#include <pebble.h>
#include "common.h"

#define FONT_PX 72
#define NUMERAL_MID_OFFSET ((FONT_PX * 9) / 16)

static Window *s_window;
static GFont s_font;
static char s_buf[4];

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint c = grect_center_point(&bounds);
  int16_t R = bounds.size.w / 2;
  const int m = s_state.minute;
  const GColor accent = accent_color(&s_state);

  graphics_context_set_antialiased(ctx, true);

  const int16_t r_blocks = sc(92, R);
  const int16_t pitch = sc(11, R);
  const int16_t r_on = sc(4, R);
  const int16_t r_off = sc(2, R);

  for (int k = 0; k < 6; k++) {
    /* Block k sits at 30, 90, 150, ... degrees (between the even hours). */
    GPoint b = ring_point(c, r_blocks, (TRIG_MAX_ANGLE * (2 * k + 1)) / 12);
    bool complete = (m >= (k + 1) * 10);
    for (int idx = 0; idx < 9; idx++) {
      int minute_val = k * 10 + idx + 1;
      bool on = (minute_val <= m);
      GPoint p = GPoint(b.x + (idx % 3 - 1) * pitch, b.y + (idx / 3 - 1) * pitch);
      graphics_context_set_fill_color(ctx, complete ? accent : (on ? COLOR_FG : COLOR_DIM));
      graphics_fill_circle(ctx, p, on ? r_on : r_off);
    }
  }

  /* Hour numeral. */
  snprintf(s_buf, sizeof(s_buf), "%d", display_hour(s_state.hour));
  GRect box = GRect(bounds.origin.x, c.y - NUMERAL_MID_OFFSET, bounds.size.w, FONT_PX + FONT_PX / 2);
  graphics_context_set_text_color(ctx, COLOR_FG);
  graphics_draw_text(ctx, s_buf, s_font, box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  if (!s_state.connected) draw_disconnected_marker(ctx, GPoint(c.x, c.y + sc(80, R)), sc(5, R));
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITS_72));
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update_proc);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  s_canvas = NULL;
  fonts_unload_custom_font(s_font);
}

static void init(void) {
  s_window = window_create();
  window_set_background_color(s_window, COLOR_BG);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  face_services_init();
}

static void deinit(void) {
  face_services_deinit();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
