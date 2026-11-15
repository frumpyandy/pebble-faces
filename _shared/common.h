#pragma once
#include <pebble.h>

/* ------------------------------------------------------------------
 * Shared helpers for the Round-faces family.
 * Everything is laid out relative to the screen radius so the same
 * code renders on Pebble Round 2 (gabbro, 260x260) and Pebble Time
 * Round (chalk, 180x180).  No floating point anywhere.
 * ---------------------------------------------------------------- */

#define DESIGN_RADIUS 130   /* the designs were drawn for a 260 px screen */

/* Scale a design-space length to the current screen radius. */
static inline int16_t sc(int16_t v, int16_t screen_radius) {
  return (int16_t)((v * screen_radius + DESIGN_RADIUS / 2) / DESIGN_RADIUS);
}

/* Point at distance r from c, at a trig angle measured clockwise from 12. */
static inline GPoint ring_point(GPoint c, int16_t r, int32_t angle) {
  return GPoint(c.x + (int16_t)((sin_lookup(angle) * r) / TRIG_MAX_RATIO),
                c.y - (int16_t)((cos_lookup(angle) * r) / TRIG_MAX_RATIO));
}

static inline int32_t minute_angle(int m) {
  return (TRIG_MAX_ANGLE * m) / 60;
}

/* Hour angle that creeps forward with the minutes, like a real hour hand. */
static inline int32_t hour_angle(int h, int m) {
  return (TRIG_MAX_ANGLE * ((h % 12) * 60 + m)) / 720;
}

/* Hour to display: 0-23 in 24h mode, 1-12 otherwise. */
static inline int display_hour(int h) {
  if (clock_is_24h_style()) return h;
  h %= 12;
  return h == 0 ? 12 : h;
}

/* Palette. Round 2 is a 64-colour e-paper panel, so stick to a few
 * strong colours; dim grey is used for "not yet" marks. */
#define COLOR_BG      GColorBlack
#define COLOR_FG      GColorWhite
#define COLOR_DIM     PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite)
#define COLOR_ACCENT  PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite)
#define COLOR_WARN    PBL_IF_COLOR_ELSE(GColorRed, GColorWhite)

#define LOW_BATTERY_PERCENT 20

/* Shared app state. */
typedef struct {
  int hour;
  int minute;
  bool connected;
  bool low_battery;
} FaceState;

static inline GColor accent_color(const FaceState *s) {
  return s->low_battery ? COLOR_WARN : COLOR_ACCENT;
}

/* Small, quiet "phone disconnected" marker: a hollow ring near the
 * bottom edge.  Pass the point where you want it. */
static inline void draw_disconnected_marker(GContext *ctx, GPoint p, int16_t r) {
  graphics_context_set_stroke_color(ctx, COLOR_FG);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_circle(ctx, p, r);
}

/* ------------------------------------------------------------------
 * 3x5 dot-matrix digits ("count the dots").
 * Each row is 3 bits, MSB = left column.
 * ---------------------------------------------------------------- */
static const uint8_t DOT_DIGITS[10][5] = {
  {0x7, 0x5, 0x5, 0x5, 0x7},  /* 0 */
  {0x2, 0x6, 0x2, 0x2, 0x7},  /* 1 */
  {0x7, 0x1, 0x7, 0x4, 0x7},  /* 2 */
  {0x7, 0x1, 0x7, 0x1, 0x7},  /* 3 */
  {0x5, 0x5, 0x7, 0x1, 0x1},  /* 4 */
  {0x7, 0x4, 0x7, 0x1, 0x7},  /* 5 */
  {0x7, 0x4, 0x7, 0x5, 0x7},  /* 6 */
  {0x7, 0x1, 0x1, 0x1, 0x1},  /* 7 */
  {0x7, 0x5, 0x7, 0x5, 0x7},  /* 8 */
  {0x7, 0x5, 0x7, 0x1, 0x7},  /* 9 */
};

/* Draw one digit with its top-left dot centred at (x0, y0). */
static inline void draw_dot_digit(GContext *ctx, int d, int16_t x0, int16_t y0,
                                  int16_t pitch, int16_t r) {
  for (int row = 0; row < 5; row++) {
    uint8_t bits = DOT_DIGITS[d % 10][row];
    for (int col = 0; col < 3; col++) {
      if (bits & (0x4 >> col)) {
        graphics_fill_circle(ctx, GPoint(x0 + col * pitch, y0 + row * pitch), r);
      }
    }
  }
}

/* Draw an integer (1 or 2 digits) centred on `c`. */
static inline void draw_dot_number(GContext *ctx, int n, GPoint c,
                                   int16_t pitch, int16_t r, bool two_digits) {
  int16_t gap = (pitch * 13) / 10;          /* space between digits */
  int16_t digit_w = 2 * pitch;              /* centre-to-centre width of a digit */
  int ndigits = (two_digits || n >= 10) ? 2 : 1;
  int16_t total = ndigits * digit_w + (ndigits - 1) * gap;
  int16_t x = c.x - total / 2;
  int16_t y = c.y - 2 * pitch;
  if (ndigits == 2) {
    draw_dot_digit(ctx, (n / 10) % 10, x, y, pitch, r);
    x += digit_w + gap;
  }
  draw_dot_digit(ctx, n % 10, x, y, pitch, r);
}

/* ------------------------------------------------------------------
 * Event plumbing shared by every face.  Each face defines
 *   static Layer *s_canvas;  and  static FaceState s_state;
 * and calls face_services_init()/deinit() from init()/deinit().
 * ---------------------------------------------------------------- */
static Layer *s_canvas;
static FaceState s_state;

static void face_update_time(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_state.hour = t->tm_hour;
  s_state.minute = t->tm_min;
}

static void face_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_state.hour = tick_time->tm_hour;
  s_state.minute = tick_time->tm_min;
  if (s_canvas) layer_mark_dirty(s_canvas);
}

static void face_battery_handler(BatteryChargeState charge) {
  bool low = charge.charge_percent <= LOW_BATTERY_PERCENT && !charge.is_plugged;
  if (low != s_state.low_battery) {
    s_state.low_battery = low;
    if (s_canvas) layer_mark_dirty(s_canvas);
  }
}

static void face_connection_handler(bool connected) {
  if (connected != s_state.connected) {
    s_state.connected = connected;
    if (!connected) vibes_short_pulse();
    if (s_canvas) layer_mark_dirty(s_canvas);
  }
}

static void face_services_init(void) {
  face_update_time();
  s_state.connected = connection_service_peek_pebble_app_connection();
  face_battery_handler(battery_state_service_peek());
  tick_timer_service_subscribe(MINUTE_UNIT, face_tick_handler);
  battery_state_service_subscribe(face_battery_handler);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = face_connection_handler,
  });
}

static void face_services_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
}
