/* Beads: an analogue watch with no hands.
 *   - big white bead on the outer track  = hour (creeps like an hour hand)
 *   - small accent bead on the inner track = minute
 * Hollow ring in the centre = phone disconnected.  Accent turns red on low battery. */
#include <pebble.h>
#include "common.h"

static Window *s_window;

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint c = grect_center_point(&bounds);
  int16_t R = bounds.size.w / 2;

  graphics_context_set_antialiased(ctx, true);

  /* 60-step scale on the rim: ticks on the hours, pinpricks between. */
  for (int i = 0; i < 60; i++) {
    int32_t a = minute_angle(i);
    if (i % 5 == 0) {
      graphics_context_set_stroke_color(ctx, COLOR_DIM);
      graphics_context_set_stroke_width(ctx, sc(3, R));
      graphics_draw_line(ctx, ring_point(c, sc(120, R), a), ring_point(c, sc(126, R), a));
    } else {
      graphics_context_set_fill_color(ctx, COLOR_DIM);
      graphics_fill_circle(ctx, ring_point(c, sc(123, R), a), 1);
    }
  }

  /* Inner track for the minute bead. */
  const int16_t r_hour = sc(112, R);
  const int16_t r_min = sc(84, R);
  graphics_context_set_stroke_color(ctx, COLOR_DIM);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, c, r_min);

  /* Hour bead. */
  graphics_context_set_fill_color(ctx, COLOR_FG);
  graphics_fill_circle(ctx, ring_point(c, r_hour, hour_angle(s_state.hour, s_state.minute)), sc(14, R));

  /* Minute bead. */
  graphics_context_set_fill_color(ctx, accent_color(&s_state));
  graphics_fill_circle(ctx, ring_point(c, r_min, minute_angle(s_state.minute)), sc(8, R));

  if (!s_state.connected) draw_disconnected_marker(ctx, c, sc(6, R));
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
