#include <pebble.h>
#pragma once

///////////////////////
// weather variables //
///////////////////////
#define KEY_TEMP
#define KEY_WIND
#define KEY_DEG
#define KEY_ICON

////////////////////
// font variables //
////////////////////
#define WORD_FONT RESOURCE_ID_WORD_FONT_12
#define NUMBER_FONT RESOURCE_ID_NUMBER_FONT_16
#define HOUR_FONT RESOURCE_ID_HOUR_FONT_26

// persistent storage keys
#define SETTINGS_KEY 1
#define WEATHER_KEY 2

#define OFF 0
// Set regional settings
enum languages {
    EN=0,
    RU=1
};
static char *a_week[2][7] = {
    { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" },
    { "ВСК", "ПНД", "ВТР", "СРД", "ЧТВ", "ПТН", "СБТ" }
};
enum weather_units {
	METRIC=0,
	IMPERIAL=1
};

///////////////////
// Clay settings //
///////////////////
typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor ForegroundColor;
  bool InvertColors;
  bool DrawAllNumbers;
  uint8_t  Lang;
  uint16_t  WeatherUpdateInterval;
  uint8_t  WeatherUnit;
  uint16_t  VibrateInterval;
  char DateFormat[16];
} ClaySettings; // Define our settings struct

// current weather cache
typedef struct WeatherData {
  unsigned long timeStamp;
  int16_t temp;
  uint8_t wind;
  char deg;
  char icon[64];	
} WeatherData; 

static void config_clear();
static void config_load();
static void config_save();
static void setColors();
static void weather_save();
static void weather_clear();
static void weather_load();
static void weather_update_req();
static void update_proc_dial(Layer *layer, GContext *ctx);
static void update_proc_battery(Layer *layer, GContext *ctx);
static void update_proc_hands(Layer *layer, GContext *ctx);
static void update_date();
static void icons_load_weather();
static void icons_load_state();
static void handler_tick(struct tm *tick_time, TimeUnits units_changed);
static void handler_battery(BatteryChargeState charge_state);
static void handler_health(HealthEventType event, void *context);
static void callback_bluetooth(bool connected);
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void init();
static void deinit();
