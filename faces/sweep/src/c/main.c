/* Sweep: the minute is a band of colour.
 *   - outer band fills clockwise from 12 as the minutes pass
 *   - 12 short ticks inside the band; the current hour's tick is long and white
 *   - big hour numeral in the middle
 * Hollow ring below the numeral = phone disconnected.  Accent turns red on low battery. */
#include <pebble.h>
#include "common.h"

#define FONT_PX 96
/* Distance from the top of the text box to the vertical middle of the digits,
 * for DejaVu Sans Bold at FONT_PX.  Nudge this if the numeral sits high or low. */
#define NUMERAL_MID_OFFSET ((FONT_PX * 9) / 16)

static Window *s_window;
static GFont s_font;
static char s_buf[4];

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint c = grect_center_point(&bounds);
  int16_t R = bounds.size.w / 2;
  const int m = s_state.minute;
  const int16_t band = sc(16, R);
  GRect ring = grect_inset(bounds, GEdgeInsets(sc(2, R)));

  graphics_context_set_antialiased(ctx, true);

  /* Track, then the minutes elapsed so far. */
  graphics_context_set_fill_color(ctx, COLOR_DIM);
  graphics_fill_radial(ctx, ring, GOvalScaleModeFitCircle, band, 0, TRIG_MAX_ANGLE);
  if (m > 0) {
    graphics_context_set_fill_color(ctx, accent_color(&s_state));
    graphics_fill_radial(ctx, ring, GOvalScaleModeFitCircle, band, 0, minute_angle(m));
  }

  /* Hour ticks. */
  const int16_t r_outer = R - sc(2, R) - band - sc(6, R);
  for (int i = 0; i < 12; i++) {
    int32_t a = (TRIG_MAX_ANGLE * i) / 12;
    bool now = (i == s_state.hour % 12);
    graphics_context_set_stroke_color(ctx, now ? COLOR_FG : COLOR_DIM);
    graphics_context_set_stroke_width(ctx, now ? sc(4, R) : sc(2, R));
    graphics_draw_line(ctx, ring_point(c, r_outer, a),
                       ring_point(c, r_outer - (now ? sc(14, R) : sc(6, R)), a));
  }

  /* Hour numeral. */
  snprintf(s_buf, sizeof(s_buf), "%d", display_hour(s_state.hour));
  GRect box = GRect(bounds.origin.x, c.y - NUMERAL_MID_OFFSET, bounds.size.w, FONT_PX + FONT_PX / 2);
  graphics_context_set_text_color(ctx, COLOR_FG);
  graphics_draw_text(ctx, s_buf, s_font, box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  if (!s_state.connected) draw_disconnected_marker(ctx, GPoint(c.x, c.y + sc(82, R)), sc(5, R));
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITS_96));
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
