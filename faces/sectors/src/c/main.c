/* Sectors: TIC-9TTMM's counting rule, rebuilt for a round screen.
 *
 *   - the ring around the hour is divided into six 60-degree sectors, one per
 *     ten minutes, clockwise from 12
 *   - inside the sector that is currently filling, nine dots light up one per
 *     minute (a 3x3 grid bent into polar coordinates: three rings, three angles)
 *   - the tenth minute has no dot: the whole sector fills solid instead
 *   - at :30 the first three solid sectors fuse into one half-disc, the way
 *     three squares fused into a bar on the original
 *   - sectors still to come stay empty
 *
 * Hollow ring under the numeral = phone disconnected.  Accent turns red on low battery. */
#include <pebble.h>
#include "common.h"

#define FONT_PX 64
#define NUMERAL_MID_OFFSET ((FONT_PX * 9) / 16)

#define SECTOR_ANGLE (TRIG_MAX_ANGLE / 6)
#define SEAM         (TRIG_MAX_ANGLE / 240)   /* 1.5 degrees between solid sectors */

static Window *s_window;
static GFont s_font;
static char s_buf[4];

static void fill_sector_range(GContext *ctx, GRect ring, int16_t thickness, int first, int last, bool seams) {
  int32_t a0 = SECTOR_ANGLE * first;
  int32_t a1 = SECTOR_ANGLE * (last + 1);
  if (seams) {
    a0 += SEAM;
    a1 -= SEAM;
  }
  graphics_fill_radial(ctx, ring, GOvalScaleModeFitCircle, thickness, a0, a1);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint c = grect_center_point(&bounds);
  int16_t R = bounds.size.w / 2;
  const int m = s_state.minute;

  const int16_t r_out = sc(122, R);
  const int16_t r_in = sc(58, R);
  const int16_t rows[3] = { sc(110, R), sc(90, R), sc(70, R) };
  const int16_t r_on = sc(5, R);
  const int16_t r_off = sc(2, R);
  GRect ring = grect_inset(bounds, GEdgeInsets(R - r_out));

  graphics_context_set_antialiased(ctx, true);

  /* Solid sectors. */
  const int complete = m / 10;                 /* number of finished sectors, 0..5 */
  graphics_context_set_fill_color(ctx, accent_color(&s_state));
  int s = 0;
  if (complete >= 3) {
    fill_sector_range(ctx, ring, r_out - r_in, 0, 2, false);   /* fused half-disc */
    s = 3;
  }
  for (; s < complete; s++) {
    fill_sector_range(ctx, ring, r_out - r_in, s, s, true);
  }

  /* The sector currently filling. */
  if (complete < 6) {
    int32_t base = SECTOR_ANGLE * complete;
    for (int k = 0; k < 9; k++) {
      int row = k / 3, col = k % 3;
      /* columns sit at 10, 30 and 50 degrees into the sector */
      int32_t a = base + (TRIG_MAX_ANGLE * (10 + 20 * col)) / 360;
      bool on = (complete * 10 + k + 1 <= m);
      graphics_context_set_fill_color(ctx, on ? COLOR_FG : COLOR_DIM);
      graphics_fill_circle(ctx, ring_point(c, rows[row], a), on ? r_on : r_off);
    }
  }

  /* Hour numeral in the clear centre. */
  snprintf(s_buf, sizeof(s_buf), "%d", display_hour(s_state.hour));
  GRect box = GRect(bounds.origin.x, c.y - NUMERAL_MID_OFFSET, bounds.size.w, FONT_PX + FONT_PX / 2);
  graphics_context_set_text_color(ctx, COLOR_FG);
  graphics_draw_text(ctx, s_buf, s_font, box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  if (!s_state.connected) draw_disconnected_marker(ctx, GPoint(c.x, c.y + sc(44, R)), sc(4, R));
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITS_64));
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
