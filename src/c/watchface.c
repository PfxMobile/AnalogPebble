// Advanced Analog Watchface for Pebble
// 
//
//
// Based on DIAL watchface (Perfect Analog) by Jacob Rusch <jacob.rusch@gmail.com>
//

#include <pebble.h>
#include "watchface.h"
//#define DEBUG 1
//#define DEMO 1

static Window *s_main_window;
static Layer *s_dial_layer, *s_battery_layer, *s_hands_layer;
static TextLayer *s_temp_layer, *s_health_layer, *s_day_text_layer, *s_date_text_layer;
static GBitmap *s_weather_bitmap=NULL, *s_bluetooth_bitmap=NULL, *s_charging_bitmap=NULL, *s_quiet_bitmap=NULL;
static BitmapLayer *s_weather_bitmap_layer, *s_bluetooth_bitmap_layer, *s_charging_bitmap_layer, *s_quiet_bitmap_layer;
static int buf=PBL_IF_ROUND_ELSE(0, 24);

static GFont s_word_font, s_number_font, s_hour_font;
static Status status;
static WeatherData weather;
static ClaySettings settings; // An instance of the struct

// store text align
static GTextAlignment s_temp_align=GTextAlignmentCenter;
static GTextAlignment s_health_align=GTextAlignmentCenter;

// MAIN

///////////////////////////////
// set default Clay settings //
///////////////////////////////
static void config_clear() {
  settings.BackgroundColor = GColorBlack;
  settings.ForegroundColor = GColorWhite;
  settings.InvertColors = false;
  settings.DrawAllNumbers = true;
  settings.Lang = EN;
  settings.VibrateInterval = OFF;
  settings.VibrateOnBTLost = true;
  settings.WeatherUpdateInterval = 30; 
  settings.WeatherUnit = METRIC;
  strcpy(settings.DateFormat, "%d-%m");
};


/////////////////////////////////////
// load default settings from Clay //
/////////////////////////////////////
static void config_load() {
	persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));	// Read settings from persistent storage, if they exist
#ifdef DEBUG 
  APP_LOG(APP_LOG_LEVEL_DEBUG, "config_load");
#endif
};


/////////////////////////
// saves user settings //
/////////////////////////
static void config_save() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
#ifdef DEBUG 
  APP_LOG(APP_LOG_LEVEL_DEBUG, "config_save");
#endif
};


////////////////
// load default status
//////////
static void status_clear() {
    status.changed=true;
    status.is_charging=false;
    status.battery_percent=0;
    status.is_bt_connected=false;
    status.is_quiet_time=false;
    status.step_count = 0;
};


///////////////////////
// sets watch colors //
///////////////////////
static void setColors() {
  if(settings.InvertColors==1) {
    settings.BackgroundColor = GColorWhite;
    settings.ForegroundColor = GColorBlack;
  } else {
    settings.BackgroundColor = GColorBlack;
    settings.ForegroundColor = GColorWhite;
  }
  // set background color
  window_set_background_color(s_main_window, settings.BackgroundColor);
  // set text color for TextLayers
  text_layer_set_text_color(s_temp_layer, settings.ForegroundColor);
  text_layer_set_text_color(s_health_layer, settings.ForegroundColor);
  text_layer_set_text_color(s_day_text_layer, settings.ForegroundColor);
  text_layer_set_text_color(s_date_text_layer, settings.ForegroundColor);
  // draw hands
  layer_mark_dirty(s_hands_layer); 
#ifdef DEBUG 
  APP_LOG(APP_LOG_LEVEL_DEBUG, "setColors");
#endif
};


//////////////////
// Save the weather to persistent storage
//////////////////
static void weather_save() {
  int ret = persist_write_data(WEATHER_KEY, &weather, sizeof(weather));
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Config: Persistent Weather Saved: (%d)", ret);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather: Saved info with timestamp [%lu]", weather.timeStamp);
#endif
};


////////////////
// Clear weather data
////////////////
static void weather_clear() {
    weather.timeStamp = 0;
    weather.temp = 0;
    weather.wind = 0;
    weather.deg = 'n';
    strcpy(weather.icon, "");
}

////////////////
// Load weather from persistent storage and check it's not too old.
///////////////
static void weather_load() {
#ifdef DEUBG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Run: weather_load");
#endif
  if (settings.WeatherUpdateInterval != OFF ) {
   if ( ( weather.timeStamp >0 ) &&  ( (time(NULL)-weather.timeStamp) <= (settings.WeatherUpdateInterval*60) )
       ) {
#ifdef DEUBG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather: No need to load");
#endif
        return; // no need to load
    };
    weather_clear();
	int ret = persist_read_data(WEATHER_KEY, &weather, sizeof(weather));
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Config: Persistent Weather Loaded: (%d) temp=%d,wind=%d,deg=%c,icon=%s", ret, weather.temp, weather.wind, weather.deg, weather.icon);
    APP_LOG(APP_LOG_LEVEL_INFO, "Weather: loaded (%d), ts: [%lu], timeNow: [%lu], age: [%lu] seconds",ret, weather.timeStamp, time(NULL), time(NULL)-weather.timeStamp);
#endif
    if ( (time(NULL)-weather.timeStamp) > (settings.WeatherUpdateInterval*60*3) ) {  // older than 3 times
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather too old, clear it and request");
#endif
    weather_clear();
	weather_update_req();
    }
  }
  else weather_clear();
};


///////////////////
// requiest weather update
///////////////////
static void weather_update_req() {
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Run: weater update req");
#endif
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
    // Send the message!
    app_message_outbox_send();
};


/////////////////////////
// draws dial on watch //
/////////////////////////
static void update_proc_dial(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds); 
  // draw hours 
  graphics_context_set_text_color(ctx, settings.ForegroundColor);
  graphics_context_set_antialiased(ctx, true);
  // base numbers
  graphics_draw_text(ctx, "12", s_hour_font, GRect((bounds.size.w / 2) - 15, -5, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "3", s_hour_font, GRect(bounds.size.w - 31, (bounds.size.h / 2) - 15, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
graphics_draw_text(ctx, "6", s_hour_font, GRect((bounds.size.w / 2) - 15, bounds.size.h - 26, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
graphics_draw_text(ctx, "9", s_hour_font, GRect(1, (bounds.size.h / 2) - 15, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
if ( PBL_IF_ROUND_ELSE(true, false) || !settings.DrawAllNumbers ) {
      //round - 
  // draw marks
    graphics_context_set_stroke_width(ctx, 4);
  	graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
	uint8_t t_pos[10] = {5,10,20,25,35,40,50,55};
    uint16_t tick_length_end = (bounds.size.w+buf)/2; 
    uint16_t tick_length_start;
    for(int i=0; i<8; i++) {
    // if number is divisible by 5, make large mark
    tick_length_start = tick_length_end-9;
    int angle = TRIG_MAX_ANGLE * t_pos[i]/60;
    GPoint tick_mark_start = {
      .x = (int)(sin_lookup(angle) * (int)tick_length_start / TRIG_MAX_RATIO) + center.x,
      .y = (int)(-cos_lookup(angle) * (int)tick_length_start / TRIG_MAX_RATIO) + center.y,
    };
    GPoint tick_mark_end = {
      .x = (int)(sin_lookup(angle) * (int)tick_length_end / TRIG_MAX_RATIO) + center.x,
      .y = (int)(-cos_lookup(angle) * (int)tick_length_end / TRIG_MAX_RATIO) + center.y,
    };      
    graphics_draw_line(ctx, tick_mark_end, tick_mark_start);  
  	} // end of loop 
  } else { 
      //rectangle
  graphics_draw_text(ctx, "1", s_hour_font, GRect((bounds.size.w / 4*3) - PBL_IF_ROUND_ELSE(30,15), PBL_IF_ROUND_ELSE(5,-5), 30, 24), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "2", s_hour_font, GRect(bounds.size.w - PBL_IF_ROUND_ELSE(49,31), (bounds.size.h / 4) - 15, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
  graphics_draw_text(ctx, "4", s_hour_font, GRect(bounds.size.w - PBL_IF_ROUND_ELSE(49,31), (bounds.size.h / 4*3) - 15, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
  graphics_draw_text(ctx, "5", s_hour_font, GRect((bounds.size.w / 4*3) - PBL_IF_ROUND_ELSE(30,15), bounds.size.h - PBL_IF_ROUND_ELSE(36,26), 30, 24), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "7", s_hour_font, GRect((bounds.size.w / 4) - PBL_IF_ROUND_ELSE(0,15), bounds.size.h - PBL_IF_ROUND_ELSE(36,26), 30, 24), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "8", s_hour_font, GRect(PBL_IF_ROUND_ELSE(18,1), (bounds.size.h / 4*3) - 15, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, "10", s_hour_font, GRect(PBL_IF_ROUND_ELSE(18,1), (bounds.size.h / 4) - 15, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, "11", s_hour_font, GRect((bounds.size.w / 4) - PBL_IF_ROUND_ELSE(0,15), PBL_IF_ROUND_ELSE(5,-5), 30, 24), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }
#ifdef DEBUG
 APP_LOG(APP_LOG_LEVEL_DEBUG,"Redraw: dial");
#endif
}


///////////////////////////
// update battery status //
///////////////////////////
static void update_proc_battery(Layer *layer, GContext *ctx) {
 //draw horizontal battery
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_context_set_stroke_width(ctx, 1);
  // end point
  GPoint start_temp_line = GPoint(PBL_IF_ROUND_ELSE(150, 119), PBL_IF_ROUND_ELSE(109, 99));
  GPoint end_temp_line = GPoint(PBL_IF_ROUND_ELSE(151, 120), PBL_IF_ROUND_ELSE(109, 99));
  graphics_draw_line(ctx, start_temp_line, end_temp_line);    
  // battery line 
  int batt = status.battery_percent/10;
  start_temp_line = GPoint(PBL_IF_ROUND_ELSE(119, 88), PBL_IF_ROUND_ELSE(109, 99));
  end_temp_line = GPoint(PBL_IF_ROUND_ELSE(120, 89)+(int)batt*3, PBL_IF_ROUND_ELSE(109, 99));
  graphics_draw_line(ctx, start_temp_line, end_temp_line);    
  // top line
  start_temp_line = GPoint(PBL_IF_ROUND_ELSE(119, 88), PBL_IF_ROUND_ELSE(108, 98));
  end_temp_line = GPoint(PBL_IF_ROUND_ELSE(151, 120), PBL_IF_ROUND_ELSE(108, 98));
  graphics_draw_line(ctx, start_temp_line, end_temp_line);    
  // bottom line
  //start_temp_line = GPoint(PBL_IF_ROUND_ELSE(119, 88), PBL_IF_ROUND_ELSE(110, 100));
  //end_temp_line = GPoint(PBL_IF_ROUND_ELSE(151, 120), PBL_IF_ROUND_ELSE(110, 100));
  //graphics_draw_line(ctx, start_temp_line, end_temp_line);    
  // set visibility of charging icon
  layer_set_hidden(bitmap_layer_get_layer(s_charging_bitmap_layer), !status.is_charging);
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Redraw: battery");
#endif
}


/////////////////////////////////
// draw hands and update ticks //
/////////////////////////////////
static void update_proc_hands(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds); 
    
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  int hand_point_end = ((bounds.size.h)/2)-30;
  int hand_point_start = hand_point_end -60;
  
  int filler_point_end = 30;
  int filler_point_start = filler_point_end-15;


  // move health position depending on time (hand down position)
  GTextAlignment tmp_ta;
  // detect align
  if (( t->tm_min >= 25 ) && ( t->tm_min < 30 )) {
      tmp_ta=GTextAlignmentLeft;
  } else if (( t->tm_min >= 30 ) && ( t->tm_min <= 35 )) {
      tmp_ta=GTextAlignmentRight;
  } else {
      // default
      tmp_ta=GTextAlignmentCenter;
  };
  // set align if not right
  if ( s_health_align != tmp_ta ) {
    s_health_align = tmp_ta;
    text_layer_set_text_alignment(s_health_layer, tmp_ta);
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Move health text");
#endif
  }

  if ( settings.WeatherUpdateInterval != OFF ) {
  // move temperature position
  if ( t->tm_min >= 55 ) {
     tmp_ta=GTextAlignmentRight;
  } else if ( t->tm_min <= 5 ) {
     tmp_ta=GTextAlignmentLeft;
  } else {
    // default
     tmp_ta=GTextAlignmentCenter;
  };
  // set align if not right
  if ( s_temp_align != tmp_ta ) {
    s_temp_align = tmp_ta;
    text_layer_set_text_alignment(s_temp_layer, tmp_ta);
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Move temp text");
#endif
  };
  }

  float minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  GPoint minute_hand_start = {
    .x = (int)(sin_lookup(minute_angle) * (int)hand_point_start / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(minute_angle) * (int)hand_point_start / TRIG_MAX_RATIO) + center.y,
  };
  
  GPoint minute_hand_end = {
    .x = (int)(sin_lookup(minute_angle) * (int)hand_point_end / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(minute_angle) * (int)hand_point_end / TRIG_MAX_RATIO) + center.y,
  };    
  
  float hour_angle = TRIG_MAX_ANGLE * ((((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  GPoint hour_hand_start = {
    .x = (int)(sin_lookup(hour_angle) * (int)hand_point_start / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(hour_angle) * (int)hand_point_start / TRIG_MAX_RATIO) + center.y,
  };
  
  GPoint hour_hand_end = {
    .x = (int)(sin_lookup(hour_angle) * (int)(hand_point_end-16) / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(hour_angle) * (int)(hand_point_end-16) / TRIG_MAX_RATIO) + center.y,
  };   
  
  GPoint filler_start = {
    .x = (int)(sin_lookup(hour_angle) * (int)filler_point_start / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(hour_angle) * (int)filler_point_start / TRIG_MAX_RATIO) + center.y,
  };
  
  GPoint filler_end = {
    .x = (int)(sin_lookup(hour_angle) * (int)filler_point_end / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(hour_angle) * (int)filler_point_end / TRIG_MAX_RATIO) + center.y,
  }; 
  
  graphics_context_set_antialiased(ctx, true);

  // draw wide part of minute hand in background color for shadow
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);  
  graphics_context_set_stroke_width(ctx, 8);  
  graphics_draw_line(ctx, minute_hand_start, minute_hand_end);  
  
  // draw minute line
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);  
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, center, minute_hand_start);  
  
  // draw wide part of minute hand
   graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_context_set_stroke_width(ctx, 6);  
  graphics_draw_line(ctx, minute_hand_start, minute_hand_end);   
  
  // draw wide part of hour hand in background color for shadow
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);  
  graphics_context_set_stroke_width(ctx, 8);
  graphics_draw_line(ctx, hour_hand_start, hour_hand_end);  
  
  // draw small hour line
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor); 
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, center, hour_hand_start);   
  
  // draw wide part of hour hand
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);  
  graphics_context_set_stroke_width(ctx, 6);
  graphics_draw_line(ctx, hour_hand_start, hour_hand_end);
  
  // draw inner hour line
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);  
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, filler_start, filler_end);    
  
  // circle overlay
  // draw circle in middle 
  graphics_context_set_fill_color(ctx, settings.ForegroundColor);  
  graphics_fill_circle(ctx, center, 3);
  
  // dot in the middle
  graphics_context_set_fill_color(ctx, settings.BackgroundColor);
  graphics_fill_circle(ctx, center, 1);    

#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Redraw: hands");
#endif
};


///////////////////////
// update date
///////////////////////
static void update_date() {
  // get a tm strucutre
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // write date to buffer
  static char date_buffer[32];
  strcpy(date_buffer, "");
  strftime(date_buffer, sizeof(date_buffer), settings.DateFormat, tick_time);
  
  static char *weekday;
  weekday = a_week[settings.Lang][tick_time->tm_wday];

  // display this time on the text layer
  text_layer_set_text(s_date_text_layer, date_buffer+(('0' == date_buffer[0]?1:0))); // remove padding
  text_layer_set_text(s_day_text_layer, weekday); 
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Update: date, format=%s, res=%s", settings.DateFormat, date_buffer);
#endif
};


//////////////////////////////////////////////////////
// display appropriate weather icon                 //
// works with DarkSky.net and OpenWeatherMap.org    //
// https://darksky.net/dev/docs/response#data-point //
// https://openweathermap.org/weather-conditions    //
//////////////////////////////////////////////////////
static void icons_load_weather() {
  if (( weather.timeStamp == 0 ) || ( settings.WeatherUpdateInterval == OFF )) {
      text_layer_set_text(s_temp_layer, "");
      layer_set_hidden( bitmap_layer_get_layer(s_weather_bitmap_layer), true );
      return; // no weather
  };

   // icon
  char icon_buf[64];
  snprintf(icon_buf, sizeof(icon_buf), "%s", weather.icon);

  if ( s_weather_bitmap ) {
      gbitmap_destroy (s_weather_bitmap );
      s_weather_bitmap=NULL;
  }
  // if inverted
  if(settings.InvertColors) {
    // populate icon variable
    
    // DS clear-day
    // OW 01d (clear sky, day)
    
    if(strcmp(icon_buf, "clear-day")==0 || 
       strcmp(icon_buf, "01d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_DAY_BLACK_ICON);  
      
    // DS clear-night
    // OW 01n (clear sky, night)
      
    } else if(strcmp(icon_buf, "clear-night")==0 || 
              strcmp(icon_buf, "01n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_NIGHT_BLACK_ICON);
      
    // DS rain
    // OW 09d (shower rain, day)
    // OW 09n (shower rain, night)
    // OW 10d (rain, day)
    // OW 10n (rain, night)
    // OW 11d (thunderstorm, day)
    // OW 11n (thunderstorm, night)
      
    } else if(strcmp(icon_buf, "rain")==0 ||
             strcmp(icon_buf, "09d")==0 || 
             strcmp(icon_buf, "09n")==0 || 
             strcmp(icon_buf, "10d")==0 || 
             strcmp(icon_buf, "10n")==0 || 
             strcmp(icon_buf, "11d")==0 || 
             strcmp(icon_buf, "11n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RAIN_BLACK_ICON);
      
    // OW 50d (mist, day)
      
    } else if(strcmp(icon_buf, "50d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MIST_DAY_BLACK_ICON);
      
    // OW 50n (mist, night)
      
    } else if(strcmp(icon_buf, "50n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MIST_NIGHT_BLACK_ICON);
      
    // DS snow
    // OW 13d (snow, day)
    // OW 13n (snow, night)
      
    } else if(strcmp(icon_buf, "snow")==0 || 
              strcmp(icon_buf, "13d")==0 || 
              strcmp(icon_buf, "13n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SNOW_BLACK_ICON);
      
    // DS sleet
      
    } else if(strcmp(icon_buf, "sleet")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SLEET_BLACK_ICON);
      
    // DS wind
      
    } else if(strcmp(icon_buf, "wind")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WIND_BLACK_ICON);
      
    // DS fog
      
    } else if(strcmp(icon_buf, "fog")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FOG_BLACK_ICON);
      
    // DS cloudy
      
    } else if(strcmp(icon_buf, "cloudy")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOUDY_BLACK_ICON);
      
    // DS partly-cloudy-day
    // OW 02d (few clouds, day)
    // OW 03d (scattered clouds, day)
    // OW 04d (broken clouds, day)
      
    } else if(strcmp(icon_buf, "partly-cloudy-day")==0 || 
              strcmp(icon_buf, "02d")==0 || 
              strcmp(icon_buf, "03d")==0 || 
              strcmp(icon_buf, "04d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_DAY_BLACK_ICON);
      
    // DS partly-cloudy-night
    // OW 02d (few clouds, night)
    // OW 03d (scattered clouds, night)
    // OW 04d (broken clouds, night)      
      
    } else if(strcmp(icon_buf, "partly-cloudy-night")==0 || 
              strcmp(icon_buf, "02n")==0 || 
              strcmp(icon_buf, "03n")==0 || 
              strcmp(icon_buf, "04n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_NIGHT_BLACK_ICON);
    } 
    
  } else {
  // not inverted
  // populate icon variable
    
    // DS clear-day
    // OW 01d (clear sky, day)    
    
    if(strcmp(icon_buf, "clear-day")==0 || 
       strcmp(icon_buf, "01d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_DAY_WHITE_ICON);  
      
    // DS clear-night
    // OW 01n (clear sky, night)
      
    } else if(strcmp(icon_buf, "clear-night")==0 || 
              strcmp(icon_buf, "01n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_NIGHT_WHITE_ICON);
      
    // DS rain
    // OW 09d (shower rain, day)
    // OW 09n (shower rain, night)
    // OW 10d (rain, day)
    // OW 10n (rain, night)
    // OW 11d (thunderstorm, day)
    // OW 11n (thunderstorm, night)
      
    } else if(strcmp(icon_buf, "rain")==0 ||
             strcmp(icon_buf, "09d")==0 || 
             strcmp(icon_buf, "09n")==0 || 
             strcmp(icon_buf, "10d")==0 || 
             strcmp(icon_buf, "10n")==0 || 
             strcmp(icon_buf, "11d")==0 || 
             strcmp(icon_buf, "11n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RAIN_WHITE_ICON);
      
    // OW 50d (mist, day)
      
    } else if(strcmp(icon_buf, "50d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MIST_DAY_WHITE_ICON);
      
    // OW 50n (mist, night)
      
    } else if(strcmp(icon_buf, "50n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MIST_NIGHT_WHITE_ICON);      
      
    // DS snow
    // OW 13d (snow, day)
    // OW 13n (snow, night)
      
    } else if(strcmp(icon_buf, "snow")==0 || 
              strcmp(icon_buf, "13d")==0 || 
              strcmp(icon_buf, "13n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SNOW_WHITE_ICON);
      
    // DS sleet
      
    } else if(strcmp(icon_buf, "sleet")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SLEET_WHITE_ICON);
      
    // DS wind
      
    } else if(strcmp(icon_buf, "wind")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WIND_WHITE_ICON);
      
    // DS fog
      
    } else if(strcmp(icon_buf, "fog")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FOG_WHITE_ICON);
      
    // DS cloudy
      
    } else if(strcmp(icon_buf, "cloudy")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOUDY_WHITE_ICON);
      
    // DS partly-cloudy-day
    // OW 02d (few clouds, day)
    // OW 03d (scattered clouds, day)
    // OW 04d (broken clouds, day)
      
    } else if(strcmp(icon_buf, "partly-cloudy-day")==0 || 
              strcmp(icon_buf, "02d")==0 || 
              strcmp(icon_buf, "03d")==0 || 
              strcmp(icon_buf, "04d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_DAY_WHITE_ICON);
      
    // DS partly-cloudy-night
    // OW 02d (few clouds, night)
    // OW 03d (scattered clouds, night)
    // OW 04d (broken clouds, night)
      
    } else if(strcmp(icon_buf, "partly-cloudy-night")==0 || 
              strcmp(icon_buf, "02n")==0 || 
              strcmp(icon_buf, "03n")==0 || 
              strcmp(icon_buf, "04n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_NIGHT_WHITE_ICON);
    }   
  }
 

  bitmap_layer_set_bitmap(s_weather_bitmap_layer, s_weather_bitmap); 
  layer_set_hidden( bitmap_layer_get_layer(s_weather_bitmap_layer), false );

  // populate temp
  static char temp_buf[32];
  strcpy(temp_buf, "");
  snprintf(temp_buf, sizeof(temp_buf), "%d°%d%c", weather.temp, weather.wind, weather.deg); 
  text_layer_set_text(s_temp_layer, temp_buf);
 
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Redraw: weather bitmap");
#endif
};


////////////////////////////
// load black/white icon and recreate layers with it
// can call on init or reinit (change invert)
/////////////////////////////
static void icons_load_state() {
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Run: icons_load_state");
#endif
  if ( s_bluetooth_bitmap ) { gbitmap_destroy( s_bluetooth_bitmap ); s_bluetooth_bitmap=NULL; };
  if ( s_quiet_bitmap ) { gbitmap_destroy( s_quiet_bitmap ); s_quiet_bitmap=NULL; };
  if ( s_charging_bitmap ) { gbitmap_destroy( s_charging_bitmap ); s_charging_bitmap=NULL; };
  // select icon
  if(settings.InvertColors) {
    s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_DISCONNECTED_BLACK_ICON);
    s_quiet_bitmap = gbitmap_create_with_resource(RESOURCE_ID_QUIET_BLACK_ICON); 
    s_charging_bitmap = gbitmap_create_with_resource(RESOURCE_ID_POWER_BLACK_ICON);
  } else {
    s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_DISCONNECTED_WHITE_ICON);
    s_quiet_bitmap = gbitmap_create_with_resource(RESOURCE_ID_QUIET_WHITE_ICON);
    s_charging_bitmap = gbitmap_create_with_resource(RESOURCE_ID_POWER_WHITE_ICON);
  }
  bitmap_layer_set_bitmap(s_bluetooth_bitmap_layer, s_bluetooth_bitmap); 
  bitmap_layer_set_bitmap(s_quiet_bitmap_layer, s_quiet_bitmap); 
  bitmap_layer_set_bitmap(s_charging_bitmap_layer, s_charging_bitmap); 
  // hide icons by it state
  layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_bitmap_layer), status.is_bt_connected);
  layer_set_hidden(bitmap_layer_get_layer(s_quiet_bitmap_layer), !status.is_quiet_time);
  layer_set_hidden(bitmap_layer_get_layer(s_charging_bitmap_layer), !status.is_charging);
};


//////////////////
// handle ticks //
//////////////////
static void handler_tick(struct tm *tick_time, TimeUnits units_changed) {
  // update quiet time status
  status.is_quiet_time = quiet_time_is_active();
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Run: handler tick");
#endif
#ifdef DEMO
  status.is_quiet_time = true; 
#endif
  layer_set_hidden(bitmap_layer_get_layer(s_quiet_bitmap_layer), !status.is_quiet_time);
  layer_mark_dirty(s_hands_layer);
  update_date();
 
  // vibrate 
  if (settings.VibrateInterval > 0) {
     if ( (tick_time->tm_min % settings.VibrateInterval == 0) || (
          (tick_time->tm_min == 0 ) && ( settings.VibrateInterval == 60) ) )  {
        if (!status.is_quiet_time) { vibes_short_pulse(); };
     };
  };

  // Get weather update requiest
  if (settings.WeatherUpdateInterval != OFF ) {
      weather_load();
      if (settings.WeatherUpdateInterval < 60 ) {
            if(tick_time->tm_min % settings.WeatherUpdateInterval == 0) { weather_update_req(); }; // minutes
      } else {
            if ( (tick_time->tm_hour % (settings.WeatherUpdateInterval/60) == 0) &&
            (tick_time->tm_min == 0 )) {  weather_update_req(); }; // hours
      };
  };
};


/////////////////////////////////////
// registers battery update events //
/////////////////////////////////////
static void handler_battery(BatteryChargeState charge_state) {
  status.battery_percent = charge_state.charge_percent;
  if(charge_state.is_charging || charge_state.is_plugged) {
    status.is_charging = true;
  } else {
    status.is_charging = false;
  }
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Run: handler battery");
#endif
#ifdef DEMO
  status.is_charging = true; 
#endif
};


//////////////
// registers health update events
///////////
static void handler_health(HealthEventType event, void *context) {
 if(event==HealthEventMovementUpdate) {
    status.step_count = (int)health_service_sum_today(HealthMetricStepCount);
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Run: handler_health");
#endif
#ifdef DEMO
    status.step_count = 12345;
#endif
    static char health_buf[32];
    strcpy(health_buf,"");
 	snprintf(health_buf, sizeof(health_buf), "%d", status.step_count);
    text_layer_set_text(s_health_layer, health_buf);
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "health handler completed");
#endif
  }
};


/////////////////////////////
// manage bluetooth status //
/////////////////////////////
static void callback_bluetooth(bool connected) {
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Run: bluetooth_callback");
#endif
#ifdef DEMO
  connected=false;
#endif
  // vibrate on disconnect not on quiet time 
  if(status.is_bt_connected && !connected && !status.is_quiet_time && settings.VibrateOnBTLost) {
    vibes_double_pulse();
  }
  // status changed to on
  if (!status.is_bt_connected && connected ) {
    if ( settings.WeatherUpdateInterval != OFF ) { weather_load(); }; // check maybe weather too old
  };
  status.is_bt_connected=connected;
  layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_bitmap_layer), status.is_bt_connected);
#ifdef DEBUG
   APP_LOG(APP_LOG_LEVEL_DEBUG,"Redraw: bluetooth, %d", status.is_bt_connected);
#endif
};


////////////////////////////
// weather and Clay calls //
////////////////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
#ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback");
#endif
  // Read tuples for data

  // weather unit
  Tuple *weather_unit_t = dict_find(iterator, MESSAGE_KEY_KEY_WEATHER_UNIT);
  if (weather_unit_t) {
      // weather unit changed
      if ( settings.WeatherUnit != atoi(weather_unit_t->value->cstring) ) {
          settings.WeatherUnit = atoi(weather_unit_t->value->cstring) ;
          weather_update_req();
      };
#ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_INFO, "Weather unit=%s", weather_unit_t->value->cstring );
#endif
  };
 
  // inverted colors
  Tuple *invert_colors_t = dict_find(iterator, MESSAGE_KEY_KEY_INVERT_COLORS);
  if(invert_colors_t) { settings.InvertColors = invert_colors_t->value->int32 == 1; }

  // vibrate on bt lost
  Tuple *vibrate_on_bt_lost_t = dict_find(iterator, MESSAGE_KEY_KEY_VIBRATE_ON_BT_LOST);
  if(vibrate_on_bt_lost_t) { settings.VibrateOnBTLost = vibrate_on_bt_lost_t->value->int32 == 1; }

  // draw all numbers
  Tuple *draw_all_numbers_t = dict_find(iterator, MESSAGE_KEY_KEY_DRAW_ALL_NUMBERS);
  if(draw_all_numbers_t) { settings.DrawAllNumbers = draw_all_numbers_t->value->int32 == 1; }

  // languate settings
  Tuple *lang_t = dict_find(iterator, MESSAGE_KEY_KEY_LANG);
  if(lang_t) {
      settings.Lang = atoi(lang_t->value->cstring);
#ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_INFO, "set Lang=%d", settings.Lang );
#endif
  };

  // date format
  Tuple *date_format_t = dict_find(iterator, MESSAGE_KEY_KEY_DATE_FORMAT);
  if (date_format_t) {
      snprintf(settings.DateFormat, sizeof(settings.DateFormat), "%s", date_format_t->value->cstring);
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "set Date format=%s", date_format_t->value->cstring );
#endif
  }

  // weather update interval
  Tuple *weather_update_interval_t = dict_find(iterator, MESSAGE_KEY_KEY_WEATHER_UPDATE_INTERVAL);
  if(weather_update_interval_t) { 
  // weather update interval changed
  if ( settings.WeatherUpdateInterval != atoi(weather_update_interval_t->value->cstring) ) {
       settings.WeatherUpdateInterval = atoi(weather_update_interval_t->value->cstring);
      // load weather 
      icons_load_weather();
      if (settings.WeatherUpdateInterval != OFF) { 
      weather_update_req();
      };
  };
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "set Weather update interval=%d", settings.WeatherUpdateInterval );
#endif
  }

  // vibrate interval
  Tuple *vibrate_interval_t = dict_find(iterator, MESSAGE_KEY_KEY_VIBRATE_INTERVAL);
  if(vibrate_interval_t) { 
      settings.VibrateInterval = atoi(vibrate_interval_t->value->cstring);
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "set Vibrate interval=%d", settings.VibrateInterval );
#endif
  }

  // weather tuples
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_KEY_TEMP);
  Tuple *wind_tuple = dict_find(iterator, MESSAGE_KEY_KEY_WIND);
  Tuple *deg_tuple = dict_find(iterator, MESSAGE_KEY_KEY_DEG);
  Tuple *icon_tuple = dict_find(iterator, MESSAGE_KEY_KEY_ICON);  
  
  // If all data is available, use it
  if(temp_tuple && icon_tuple && wind_tuple && deg_tuple && settings.WeatherUpdateInterval ) { 
    weather.timeStamp = time(NULL); // now
	weather.temp = (int16_t)temp_tuple->value->int32;
	weather.wind = (uint8_t)wind_tuple->value->int32;
	weather.deg  = (char)deg_tuple->value->cstring[0]; 
	snprintf( weather.icon, sizeof(weather.icon), "%s", icon_tuple->value->cstring);
    weather_save();
    icons_load_weather();
  };
  
	setColors();	
    icons_load_state();
    update_date();
	config_save();
};


static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
};


static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
};


static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
};


//////////////////////
// load main window //
//////////////////////
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // fonts
  s_word_font = fonts_load_custom_font(resource_get_handle(WORD_FONT));
  s_number_font = fonts_load_custom_font(resource_get_handle(NUMBER_FONT));
  s_hour_font = fonts_load_custom_font(resource_get_handle(HOUR_FONT));

  // setup layers

  // create canvas layer for dial
  s_dial_layer = layer_create(bounds);
  layer_set_update_proc(s_dial_layer, update_proc_dial);
  layer_add_child(window_layer, s_dial_layer);  
  
  // create temp text
  s_temp_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(46, 30), PBL_IF_ROUND_ELSE(26, 22), 88, 16));
  text_layer_set_background_color(s_temp_layer, GColorClear); // xxx 
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentCenter);
  text_layer_set_font(s_temp_layer, s_number_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_temp_layer));

  // create weather bitmap
  s_weather_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(78, 60), 40, 24, 16));
  bitmap_layer_set_compositing_mode(s_weather_bitmap_layer, GCompOpSet);  
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_weather_bitmap_layer)); 
  // bluetooth disconnected icon
  s_bluetooth_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(20, 16), PBL_IF_ROUND_ELSE(84, 79), 14, 14));
  bitmap_layer_set_compositing_mode(s_bluetooth_bitmap_layer, GCompOpSet);
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_bluetooth_bitmap_layer));       
  // quiettime icon
  s_quiet_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(30, 28), PBL_IF_ROUND_ELSE(84, 79), 14, 14));
  bitmap_layer_set_compositing_mode(s_quiet_bitmap_layer, GCompOpSet);
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_quiet_bitmap_layer));    
  // charging icon 
  s_charging_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(38, 36), PBL_IF_ROUND_ELSE(84, 79), 14, 14));
  bitmap_layer_set_compositing_mode(s_charging_bitmap_layer, GCompOpSet);
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_charging_bitmap_layer));    

  // battery
  s_battery_layer = layer_create(bounds);
  layer_set_update_proc(s_battery_layer, update_proc_battery);
  layer_add_child(s_dial_layer, s_battery_layer);

 // create health layer text
  s_health_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(46, 30), PBL_IF_ROUND_ELSE(140, 130), 88, 16));
  text_layer_set_background_color(s_health_layer, GColorClear); // xxx
  text_layer_set_text_alignment(s_health_layer, GTextAlignmentCenter);
  text_layer_set_font(s_health_layer, s_number_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_health_layer));  
  
  // Day Text
  s_day_text_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(114, 86), PBL_IF_ROUND_ELSE(76, 66), 34, 14));
  text_layer_set_background_color(s_day_text_layer, GColorClear); //xxx
  text_layer_set_text_alignment(s_day_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_day_text_layer, s_word_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_day_text_layer));
  
  // Date text
  s_date_text_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(110, 80), PBL_IF_ROUND_ELSE(88, 78), 48, 16)); //50=48
  text_layer_set_background_color(s_date_text_layer, GColorClear); //xxx
  text_layer_set_text_alignment(s_date_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_text_layer, s_number_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_date_text_layer));  
  
  // create canvas layer for hands
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, update_proc_hands);
  layer_add_child(window_layer, s_hands_layer);

  // detect quiet
  status.is_quiet_time = quiet_time_is_active(); 

  // Make sure the date is displayed from the start
  update_date();

  setColors();	
  icons_load_state();
  icons_load_weather();  
	
  status_clear(); 
  config_save(); 
#ifdef DEBUG 
  APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load");
#endif
};


///////////////////
// unload window //
///////////////////
static void main_window_unload(Window *window) {
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "begin main window unload");
#endif

  // unsubscribe
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  health_service_events_unsubscribe(); 

  // unload text layer
  text_layer_destroy(s_temp_layer); 
  text_layer_destroy(s_health_layer);
  text_layer_destroy(s_day_text_layer);
  text_layer_destroy(s_date_text_layer);  

#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "destroy gbitmap");
#endif
  
  // unload gbitmap
  if (s_weather_bitmap) gbitmap_destroy(s_weather_bitmap); 
  if (s_bluetooth_bitmap) gbitmap_destroy(s_bluetooth_bitmap);
  if (s_charging_bitmap) gbitmap_destroy(s_charging_bitmap);
  if (s_quiet_bitmap) gbitmap_destroy(s_quiet_bitmap);
  
#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "destroy bitmap layer");
#endif

  // unload bitmap layer
  if(s_weather_bitmap_layer) { bitmap_layer_destroy(s_weather_bitmap_layer); s_weather_bitmap_layer=NULL; };
  if(s_bluetooth_bitmap_layer) { bitmap_layer_destroy(s_bluetooth_bitmap_layer); s_bluetooth_bitmap_layer=NULL; };
  if(s_charging_bitmap_layer) { bitmap_layer_destroy(s_charging_bitmap_layer); s_charging_bitmap_layer=NULL; };
  if(s_quiet_bitmap_layer) { bitmap_layer_destroy(s_quiet_bitmap_layer); s_quiet_bitmap_layer=NULL; };

#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "destroy fonts");
#endif
  // unload font
  fonts_unload_custom_font(s_word_font);
  fonts_unload_custom_font(s_number_font);
  fonts_unload_custom_font(s_hour_font);

#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "destroy layers");
#endif

  // unload layer
  layer_destroy(s_dial_layer);
  layer_destroy(s_hands_layer);
  layer_destroy(s_battery_layer); 

#ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_INFO, "end main window unload");
#endif
};


////////////////////
// initialize app //
////////////////////
static void init() {
  config_clear();
  config_load();
  weather_clear();
  weather_load();
  
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // show window on the watch with animated=true
  window_stack_push(s_main_window, true);
  
  // subscribe to time events
  tick_timer_service_subscribe(MINUTE_UNIT, handler_tick);

  // subscribe to health events 
  health_service_events_subscribe(handler_health, NULL); 
  // force initial update
  handler_health(HealthEventMovementUpdate, NULL);   
    
  // register with Battery State Service
  battery_state_service_subscribe(handler_battery);
  // force initial update
  handler_battery(battery_state_service_peek());      
  
  // register with bluetooth state service
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = callback_bluetooth
  });
  // force bluetooth
  callback_bluetooth(connection_service_peek_pebble_app_connection());  
    
  // Register weather callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  app_message_open(128, 128);  

#ifdef DEBUG 
  APP_LOG(APP_LOG_LEVEL_DEBUG, "init");  
#endif
};


///////////////////////
// de-initialize app //
///////////////////////
static void deinit() {
  window_destroy(s_main_window);
};


/////////////
// run app //
/////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
}
