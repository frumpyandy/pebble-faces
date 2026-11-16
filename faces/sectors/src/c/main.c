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
 * Settings (Clay page in the phone app): dark/light theme, accent colour,
 * low-battery red, numeral style (bold / geometric / dots), seams, fill order.
 * Hollow ring under the numeral = phone disconnected. */
#include <pebble.h>
#include "common.h"

#define FONT_PX 64
#define SECTOR_ANGLE (TRIG_MAX_ANGLE / 6)
#define SEAM         (TRIG_MAX_ANGLE / 240)   /* 1.5 degrees between solid sectors */

/* Vertical tuning for each numeral style: distance from text-box top to the
 * middle of the digits.  Nudge if a style sits high or low on the real font. */
#define BOLD_MID_OFFSET      ((FONT_PX * 9) / 16)
#define GEOMETRIC_FONT_PX    42
#define GEOMETRIC_MID_OFFSET 24

enum { THEME_DARK = 0, THEME_LIGHT = 1 };
enum { NUMERAL_BOLD = 0, NUMERAL_GEOMETRIC = 1, NUMERAL_DOTS = 2 };

#define SETTINGS_KEY     1
#define SETTINGS_VERSION 1

typedef struct {
  uint8_t version;
  uint8_t theme;
  uint8_t numeral;
  uint8_t seams;
  uint8_t fill_clockwise;
  uint8_t low_battery_red;
  GColor accent;
} Settings;

static Settings s_settings;
static Window *s_window;
static GFont s_font_bold;
static GFont s_font_geometric;
static char s_buf[4];

/* ---------------------------------------------------------------- settings */

static void settings_defaults(void) {
  s_settings = (Settings) {
    .version = SETTINGS_VERSION,
    .theme = THEME_DARK,
    .numeral = NUMERAL_BOLD,
    .seams = 1,
    .fill_clockwise = 0,
    .low_battery_red = 1,
    .accent = GColorChromeYellow,
  };
}

static void settings_load(void) {
  settings_defaults();
  if (persist_exists(SETTINGS_KEY) && persist_get_size(SETTINGS_KEY) == (int)sizeof(Settings)) {
    Settings stored;
    persist_read_data(SETTINGS_KEY, &stored, sizeof(stored));
    if (stored.version == SETTINGS_VERSION) s_settings = stored;
  }
}

static void settings_save(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static GColor col_bg(void)  { return s_settings.theme == THEME_LIGHT ? GColorWhite : GColorBlack; }
static GColor col_fg(void)  { return s_settings.theme == THEME_LIGHT ? GColorBlack : GColorWhite; }
static GColor col_dim(void) {
  return PBL_IF_COLOR_ELSE(s_settings.theme == THEME_LIGHT ? GColorLightGray : GColorDarkGray, col_fg());
}
static GColor col_accent(void) {
  if (s_settings.low_battery_red && s_state.low_battery) return PBL_IF_COLOR_ELSE(GColorRed, col_fg());
  return PBL_IF_COLOR_ELSE(s_settings.accent, col_fg());
}

/* Clay sends numbers as int32 and option values occasionally as strings; accept both. */
static int tuple_int(const Tuple *t) {
  if (!t) return 0;
  if (t->type == TUPLE_CSTRING) {
    int v = 0;
    for (const char *p = t->value->cstring; *p >= '0' && *p <= '9'; p++) v = v * 10 + (*p - '0');
    return v;
  }
  return (int)t->value->int32;
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *t;
  if ((t = dict_find(iter, MESSAGE_KEY_Theme)))         s_settings.theme = tuple_int(t) ? THEME_LIGHT : THEME_DARK;
  if ((t = dict_find(iter, MESSAGE_KEY_AccentColor)))   s_settings.accent = GColorFromHEX(t->value->int32);
  if ((t = dict_find(iter, MESSAGE_KEY_LowBatteryRed))) s_settings.low_battery_red = tuple_int(t) ? 1 : 0;
  if ((t = dict_find(iter, MESSAGE_KEY_Numeral)))       s_settings.numeral = (uint8_t)(tuple_int(t) % 3);
  if ((t = dict_find(iter, MESSAGE_KEY_Seams)))         s_settings.seams = tuple_int(t) ? 1 : 0;
  if ((t = dict_find(iter, MESSAGE_KEY_FillClockwise))) s_settings.fill_clockwise = tuple_int(t) ? 1 : 0;
  settings_save();
  window_set_background_color(s_window, col_bg());
  if (s_canvas) layer_mark_dirty(s_canvas);
}

/* ---------------------------------------------------------------- drawing */

static void fill_sector_range(GContext *ctx, GRect ring, int16_t thickness, int first, int last, bool seams) {
  int32_t a0 = SECTOR_ANGLE * first;
  int32_t a1 = SECTOR_ANGLE * (last + 1);
  if (seams) {
    a0 += SEAM;
    a1 -= SEAM;
  }
  graphics_fill_radial(ctx, ring, GOvalScaleModeFitCircle, thickness, a0, a1);
}

static void draw_numeral(GContext *ctx, GRect bounds, GPoint c, int16_t R) {
  int hour = display_hour(s_state.hour);
  graphics_context_set_text_color(ctx, col_fg());
  graphics_context_set_fill_color(ctx, col_fg());
  switch (s_settings.numeral) {
    case NUMERAL_DOTS:
      draw_dot_number(ctx, hour, c, sc(14, R), sc(5, R), clock_is_24h_style());
      break;
    case NUMERAL_GEOMETRIC: {
      snprintf(s_buf, sizeof(s_buf), "%d", hour);
      GRect box = GRect(bounds.origin.x, c.y - GEOMETRIC_MID_OFFSET, bounds.size.w, GEOMETRIC_FONT_PX + GEOMETRIC_FONT_PX / 2);
      graphics_draw_text(ctx, s_buf, s_font_geometric, box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      break;
    }
    default: {
      snprintf(s_buf, sizeof(s_buf), "%d", hour);
      GRect box = GRect(bounds.origin.x, c.y - BOLD_MID_OFFSET, bounds.size.w, FONT_PX + FONT_PX / 2);
      graphics_draw_text(ctx, s_buf, s_font_bold, box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      break;
    }
  }
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
  graphics_context_set_fill_color(ctx, col_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* Solid sectors. */
  const int complete = m / 10;                 /* finished sectors, 0..5 */
  graphics_context_set_fill_color(ctx, col_accent());
  int s = 0;
  if (complete >= 3) {
    fill_sector_range(ctx, ring, r_out - r_in, 0, 2, false);   /* fused half-disc */
    s = 3;
  }
  for (; s < complete; s++) {
    fill_sector_range(ctx, ring, r_out - r_in, s, s, s_settings.seams);
  }

  /* The sector currently filling. */
  if (complete < 6) {
    int32_t base = SECTOR_ANGLE * complete;
    for (int k = 0; k < 9; k++) {
      int row, col;
      if (s_settings.fill_clockwise) { col = k / 3; row = k % 3; }   /* column by column, outer dot first */
      else                           { row = k / 3; col = k % 3; }   /* ring by ring, clockwise */
      /* columns sit at 10, 30 and 50 degrees into the sector */
      int32_t a = base + (TRIG_MAX_ANGLE * (10 + 20 * col)) / 360;
      bool on = (complete * 10 + k + 1 <= m);
      graphics_context_set_fill_color(ctx, on ? col_fg() : col_dim());
      graphics_fill_circle(ctx, ring_point(c, rows[row], a), on ? r_on : r_off);
    }
  }

  draw_numeral(ctx, bounds, c, R);

  if (!s_state.connected) {
    graphics_context_set_stroke_color(ctx, col_fg());
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, GPoint(c.x, c.y + sc(44, R)), sc(4, R));
  }
}

/* ---------------------------------------------------------------- lifecycle */

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_font_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITS_64));
  s_font_geometric = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update_proc);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  s_canvas = NULL;
  fonts_unload_custom_font(s_font_bold);
}

static void init(void) {
  settings_load();
  s_window = window_create();
  window_set_background_color(s_window, col_bg());
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  face_services_init();

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(256, 64);
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
