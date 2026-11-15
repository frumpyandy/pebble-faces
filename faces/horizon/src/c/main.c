/* Horizon: the minute is where the line is.
 *   - a horizon line sinks from near the top (:00) to near the bottom (:59)
 *   - above the line the face is light, below it dark; the big hour numeral
 *     is cut by the line and inverts across it
 * Hollow ring near the bottom = phone disconnected.  The line turns red on low battery.
 *
 * Implementation: two layers.  The top layer covers the whole screen and draws
 * the light half; the bottom layer's frame starts at the horizon and draws the
 * dark half.  Layers clip to their frame, which gives us the inverted numeral
 * without any pixel tricks. */
#include <pebble.h>
#include "common.h"

#define FONT_PX 96
#define NUMERAL_MID_OFFSET ((FONT_PX * 9) / 16)
#define MARGIN 20          /* design-space distance of :00 / :59 from the screen edge */

static Window *s_window;
static Layer *s_bottom;
static GFont s_font;
static char s_buf[4];

static int16_t horizon_y(GRect bounds) {
  int16_t R = bounds.size.w / 2;
  int16_t margin = sc(MARGIN, R);
  return margin + (int16_t)(((bounds.size.h - 2 * margin) * s_state.minute) / 59);
}

/* Draw the numeral and the disconnected marker in screen coordinates shifted
 * by `dy` (0 for the top layer, -frame.origin.y for the bottom layer). */
static void draw_shared(GContext *ctx, GRect screen, int16_t dy, GColor ink) {
  GPoint c = grect_center_point(&screen);
  int16_t R = screen.size.w / 2;
  snprintf(s_buf, sizeof(s_buf), "%d", display_hour(s_state.hour));
  GRect box = GRect(screen.origin.x, c.y - NUMERAL_MID_OFFSET + dy, screen.size.w, FONT_PX + FONT_PX / 2);
  graphics_context_set_text_color(ctx, ink);
  graphics_draw_text(ctx, s_buf, s_font, box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  if (!s_state.connected) {
    graphics_context_set_stroke_color(ctx, ink);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, GPoint(c.x, screen.size.h - sc(32, R) + dy), sc(5, R));
  }
}

static void top_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, COLOR_FG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  draw_shared(ctx, bounds, 0, COLOR_BG);
}

static void bottom_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);     /* local coords, origin 0,0 */
  GRect frame = layer_get_frame(layer);       /* where we sit on screen */
  GRect screen = GRect(0, 0, frame.size.w, frame.origin.y + frame.size.h);
  int16_t R = screen.size.w / 2;

  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, COLOR_BG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  draw_shared(ctx, screen, -frame.origin.y, COLOR_FG);

  /* The horizon line itself, along our top edge. */
  graphics_context_set_fill_color(ctx, accent_color(&s_state));
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, sc(3, R)), 0, GCornerNone);
}

static void reposition_horizon(void) {
  if (!s_canvas || !s_bottom) return;
  GRect bounds = layer_get_bounds(s_canvas);
  int16_t y = horizon_y(bounds);
  layer_set_frame(s_bottom, GRect(0, y, bounds.size.w, bounds.size.h - y));
  layer_mark_dirty(s_canvas);
  layer_mark_dirty(s_bottom);
}

static void horizon_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  face_tick_handler(tick_time, units_changed);
  reposition_horizon();
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITS_96));

  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, top_update_proc);
  layer_add_child(root, s_canvas);

  s_bottom = layer_create(GRect(0, bounds.size.h / 2, bounds.size.w, bounds.size.h / 2));
  layer_set_update_proc(s_bottom, bottom_update_proc);
  layer_add_child(root, s_bottom);

  reposition_horizon();
}

static void window_unload(Window *window) {
  layer_destroy(s_bottom);
  s_bottom = NULL;
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
  /* Replace the shared minute handler with one that also moves the horizon. */
  tick_timer_service_subscribe(MINUTE_UNIT, horizon_tick_handler);
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
