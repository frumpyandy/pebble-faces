/* Tally: count the dots.
 *   - 60 dots around the rim light up one per minute (accent on the fives,
 *     the current minute is a slightly larger accent dot)
 *   - the hour is a dot-matrix numeral in the middle
 * Hollow ring below the numeral = phone disconnected.  Accent turns red on low battery. */
#include <pebble.h>
#include "common.h"

static Window *s_window;

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint c = grect_center_point(&bounds);
  int16_t R = bounds.size.w / 2;
  const int m = s_state.minute;
  const GColor accent = accent_color(&s_state);

  graphics_context_set_antialiased(ctx, true);

  /* Minute ring. */
  const int16_t r_ring = sc(117, R);
  for (int i = 0; i < 60; i++) {
    bool five = (i % 5 == 0);
    int16_t r = five ? sc(4, R) : sc(2, R);
    GColor col;
    if (i < m) {
      col = five ? accent : COLOR_FG;
    } else if (i == m) {
      col = accent;
      r = sc(5, R);
    } else {
      col = COLOR_DIM;
    }
    graphics_context_set_fill_color(ctx, col);
    graphics_fill_circle(ctx, ring_point(c, r_ring, minute_angle(i)), r);
  }

  /* Hour numeral. */
  graphics_context_set_fill_color(ctx, COLOR_FG);
  draw_dot_number(ctx, display_hour(s_state.hour), c, sc(24, R), sc(9, R), clock_is_24h_style());

  if (!s_state.connected) draw_disconnected_marker(ctx, GPoint(c.x, c.y + sc(92, R)), sc(5, R));
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update_proc);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  s_canvas = NULL;
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
