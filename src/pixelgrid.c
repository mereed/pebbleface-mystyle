/*
Copyright (C) 2014 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pebble.h"

static AppSync sync;
static uint8_t sync_buffer[128];

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_CLEAR_DAY,
  RESOURCE_ID_CLEAR_NIGHT,
  RESOURCE_ID_WINDY,
  RESOURCE_ID_COLD,
  RESOURCE_ID_PARTLY_CLOUDY_DAY,
  RESOURCE_ID_PARTLY_CLOUDY_NIGHT,
  RESOURCE_ID_HAZE,
  RESOURCE_ID_CLOUD,
  RESOURCE_ID_RAIN,
  RESOURCE_ID_SNOW,
  RESOURCE_ID_HAIL,
  RESOURCE_ID_CLOUDY,
  RESOURCE_ID_STORM,
  RESOURCE_ID_FOG,
  RESOURCE_ID_NA,
};

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  INVERT_COLOR_KEY = 0x2,
  HIDE_SEC_KEY = 0x3,
  BLUETOOTHVIBE_KEY = 0x4,
  HOURLYVIBE_KEY = 0x5,
  BACKGROUND_KEY = 0x6,
  BACKGROUND2_KEY = 0x7,
  BACKGROUND3_KEY = 0x8
};

static bool appStarted = false;

static int invert;
static int hidesec;
static int bluetoothvibe;
static int hourlyvibe;
static int background;
static int background2;
static int background3;

GColor background_color = GColorBlack;

static Window *window;
static Layer *window_layer;

static GFont          *custom_14;

static GBitmap *battery_image;
static BitmapLayer *battery_image_layer;
static BitmapLayer *battery_layer;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

static GBitmap *background_image;
static BitmapLayer *background_layer;

static GBitmap *background_image2;
static BitmapLayer *background_layer2;

static GBitmap *background_image3;
static BitmapLayer *background_layer3;

BitmapLayer *icon_layer;
GBitmap *icon_bitmap = NULL;
TextLayer *temp_layer;

int charge_percent = 0;

static GBitmap *separator_image;
static BitmapLayer *separator_layer;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

#define TOTAL_DATE_DIGITS 2	
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

const int DATENUM_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9,
  RESOURCE_ID_IMAGE_DATENUM_10
};

#define TOTAL_BATTERY_PERCENT_DIGITS 3
static GBitmap *battery_percent_images[TOTAL_BATTERY_PERCENT_DIGITS];
static BitmapLayer *battery_percent_layers[TOTAL_BATTERY_PERCENT_DIGITS];

#define TOTAL_TIME_DIGITS 4
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

#define TOTAL_SECONDS_DIGITS 2
static GBitmap *seconds_digits_images[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers[TOTAL_SECONDS_DIGITS];

const int TINY_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_TINY_0,
  RESOURCE_ID_IMAGE_TINY_1,
  RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3,
  RESOURCE_ID_IMAGE_TINY_4,
  RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6,
  RESOURCE_ID_IMAGE_TINY_7,
  RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9
};

InverterLayer *inverter_layer = NULL;


void set_invert_color(bool invert) {
  if (invert && inverter_layer == NULL) {
    // Add inverter layer
    Layer *window_layer = window_get_root_layer(window);

    inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
    layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));
  } else if (!invert && inverter_layer != NULL) {
    // Remove Inverter layer
    layer_remove_from_parent(inverter_layer_get_layer(inverter_layer));
    inverter_layer_destroy(inverter_layer);
    inverter_layer = NULL;
  }
  // No action required
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple* new_tuple,
                                        const Tuple* old_tuple,
                                        void* context) {

  // App Sync keeps new_tuple in sync_buffer, so we may use it directly
  switch (key) {
    case WEATHER_ICON_KEY:
    if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }

      icon_bitmap = gbitmap_create_with_resource(
          WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;
	  
    case WEATHER_TEMPERATURE_KEY:
      text_layer_set_text(temp_layer, new_tuple->value->cstring);
      break;

    case INVERT_COLOR_KEY:
      invert = new_tuple->value->uint8 != 0;
      persist_write_bool(INVERT_COLOR_KEY, invert);
      set_invert_color(invert);
      break;
	  
	case HIDE_SEC_KEY:
      hidesec = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_SEC_KEY, hidesec);
		if(hidesec) {
		  for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
		  layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[i]), true);
		  }
		tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
		}  else {
			for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
			layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[i]), false);
			}
			tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
		}
		break; 

    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break; 
		
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, background);	  
      break;
	  
	case BACKGROUND_KEY:
	  background = new_tuple->value->uint8 != 0;
	  persist_write_bool(BACKGROUND_KEY, background);
		if(background) {
			layer_set_hidden(bitmap_layer_get_layer(background_layer), true);
		} else {
			layer_set_hidden(bitmap_layer_get_layer(background_layer), false);	
		}
	  
    case BACKGROUND2_KEY:
	  background2 = new_tuple->value->uint8 != 0;
	  persist_write_bool(BACKGROUND2_KEY, background2);
		if(background2) {
			layer_set_hidden(bitmap_layer_get_layer(background_layer2), true);
		} else {
			layer_set_hidden(bitmap_layer_get_layer(background_layer2), false);	
		}
	  
	case BACKGROUND3_KEY:
	  background3 = new_tuple->value->uint8 != 0;
	  persist_write_bool(BACKGROUND3_KEY, background3);
		if(background3) {
			layer_set_hidden(bitmap_layer_get_layer(background_layer3), true);
		} else {
			layer_set_hidden(bitmap_layer_get_layer(background_layer3), false);	
		}
  }
}


static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;
  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
  gbitmap_destroy(old_image);
}

void handle_battery(BatteryChargeState charge_state) {

	charge_percent = charge_state.charge_percent;

	if(charge_percent>=97) {
	layer_set_hidden(bitmap_layer_get_layer(battery_image_layer), false);
    for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
      layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), true);
    }  
    return;
   }
   layer_set_hidden(bitmap_layer_get_layer(battery_image_layer), true);

  for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
  layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), false);
  }
  set_container_image(&battery_percent_images[0], battery_percent_layers[0], DATENUM_IMAGE_RESOURCE_IDS[charge_state.charge_percent/10], GPoint( 37, 39));
  set_container_image(&battery_percent_images[1], battery_percent_layers[1], DATENUM_IMAGE_RESOURCE_IDS[charge_state.charge_percent%10], GPoint( 47, 39));
  set_container_image(&battery_percent_images[2], battery_percent_layers[2], DATENUM_IMAGE_RESOURCE_IDS[10], GPoint( 57, 39));
}

void handle_bluetooth(bool connected) {
    if (connected) {
	  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), false);
    } else {
  	  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), true);
    }
    if (appStarted && bluetoothvibe) {     
        vibes_long_pulse();
	}
}

void bluetooth_connection_callback(bool connected) {
  handle_bluetooth(connected);
}

void force_update(void) {
    handle_battery(battery_state_service_peek());
    handle_bluetooth(bluetooth_connection_service_peek());
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_days(struct tm *tick_time) {
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint( 96, 39));
  set_container_image(&date_digits_images[0], date_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(121, 39));
  set_container_image(&date_digits_images[1], date_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(131, 39));
}

static void update_hours(struct tm *tick_time) {
  
   unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&time_digits_images[0], time_digits_layers[0], TINY_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(6, 51));
  set_container_image(&time_digits_images[1], time_digits_layers[1], TINY_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(36, 51));

  if (!clock_is_24h_style()) {
    if (display_hour/10 == 0) {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
    }
    else {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
    }
  }
}

static void update_minutes(struct tm *tick_time) {
  set_container_image(&time_digits_images[2], time_digits_layers[2], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(81, 51));
  set_container_image(&time_digits_images[3], time_digits_layers[3], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(111, 51));
}

static void update_seconds(struct tm *tick_time) {
  set_container_image(&seconds_digits_images[0], seconds_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[tick_time->tm_sec/10], GPoint(8, 39));
  set_container_image(&seconds_digits_images[1], seconds_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[tick_time->tm_sec%10], GPoint(18, 39));
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {

  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }
  if (units_changed & HOUR_UNIT) {
    update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
    update_minutes(tick_time);
  }	
    if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }		
}

void set_style(void) {
    background_color  = GColorBlack;
    window_set_background_color(window, background_color);
}
	
static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));

  memset(&seconds_digits_layers, 0, sizeof(seconds_digits_layers));
  memset(&seconds_digits_images, 0, sizeof(seconds_digits_images));
	
  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));
	
  memset(&battery_percent_layers, 0, sizeof(battery_percent_layers));
  memset(&battery_percent_images, 0, sizeof(battery_percent_images));

 // Setup messaging
  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);	
	
  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }

  set_style();

  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);

	
	background_image3 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND3);
  GRect frame1 = (GRect) {
    .origin = { .x = 0, .y = 36 },
    .size = background_image3->bounds.size
  };
  background_layer3 = bitmap_layer_create(frame1);
  bitmap_layer_set_bitmap(background_layer3, background_image3);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer3)); 
	
  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  GRect frame2 = (GRect) {
    .origin = { .x = 7, .y = 37 },
    .size = background_image->bounds.size
  };
  background_layer = bitmap_layer_create(frame2);
  bitmap_layer_set_bitmap(background_layer, background_image);
	GCompOp compositing_mode_back = GCompOpOr;
  bitmap_layer_set_compositing_mode(background_layer, compositing_mode_back);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));  
	
  background_image2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND2);
  GRect frame3 = (GRect) {
    .origin = { .x = 7, .y = 52 },
    .size = background_image2->bounds.size
  };
  background_layer2 = bitmap_layer_create(frame3);
  bitmap_layer_set_bitmap(background_layer2, background_image2);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer2)); 
	
  
	
  battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
  GRect frame4 = (GRect) {
    .origin = { .x = 37, .y = 39 },
    .size = battery_image->bounds.size
  };
  battery_layer = bitmap_layer_create(frame4);
  battery_image_layer = bitmap_layer_create(frame4);
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  GCompOp compositing_mode_batt = GCompOpOr;
  bitmap_layer_set_compositing_mode(battery_image_layer, compositing_mode_batt);
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_image_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));
	
  separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEPARATOR);
  GRect frame = (GRect) {
    .origin = { .x = 66, .y = 50 },
    .size = separator_image->bounds.size
  };
  separator_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(separator_layer, separator_image);
  GCompOp compositing_mode2 = GCompOpOr;
  bitmap_layer_set_compositing_mode(separator_layer, compositing_mode2);
  layer_add_child(window_layer, bitmap_layer_get_layer(separator_layer));   

  bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  GRect frame_bt = (GRect) {
    .origin = { .x = 68, .y = 39 },
    .size = bluetooth_image->bounds.size
  };
  bluetooth_layer = bitmap_layer_create(frame_bt);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  GCompOp compositing_mode_bt = GCompOpOr;
  bitmap_layer_set_compositing_mode(bluetooth_layer, compositing_mode_bt);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));
	
  Layer *weather_holder = layer_create(GRect(0, 0, 144, 168 ));
  layer_add_child(window_layer, weather_holder);
	
  icon_layer = bitmap_layer_create(GRect(4, 130, 100, 10));
  GCompOp compositing_mode1 = GCompOpOr;
  bitmap_layer_set_compositing_mode(icon_layer, compositing_mode1);
  layer_add_child(weather_holder, bitmap_layer_get_layer(icon_layer));

	  custom_14  = fonts_load_custom_font( resource_get_handle( RESOURCE_ID_FONT_CUSTOM_14 ) );

  temp_layer = text_layer_create(GRect(97, 126, 45, 30));
  text_layer_set_text_color(temp_layer, GColorWhite);
  text_layer_set_background_color(temp_layer, GColorClear);
  //text_layer_set_font(temp_layer,fonts_get_system_font(FONT_KEY_GOTHIC_28));
  text_layer_set_font(temp_layer, custom_14);
  text_layer_set_text_alignment(temp_layer, GTextAlignmentRight);
  layer_add_child(weather_holder, text_layer_get_layer(temp_layer));
	
	
  // Create time and date layers
   GRect dummy_frame = { {0, 0}, {0, 0} };
   day_name_layer = bitmap_layer_create(dummy_frame);
   GCompOp compositing_mode7 = GCompOpOr;
   bitmap_layer_set_compositing_mode(day_name_layer, compositing_mode7);
   layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));	
	
    for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
	GCompOp compositing_mode3 = GCompOpOr;
    bitmap_layer_set_compositing_mode(time_digits_layers[i], compositing_mode3);		
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
    }
	
    for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
	GCompOp compositing_mode6 = GCompOpOr;
    bitmap_layer_set_compositing_mode(date_digits_layers[i], compositing_mode6);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
    }
	
	for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers[i] = bitmap_layer_create(dummy_frame);
    GCompOp compositing_mode4 = GCompOpOr;
    bitmap_layer_set_compositing_mode(seconds_digits_layers[i], compositing_mode4);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers[i]));
    }
	
    for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    battery_percent_layers[i] = bitmap_layer_create(dummy_frame);
	GCompOp compositing_mode5 = GCompOpOr;
    bitmap_layer_set_compositing_mode(battery_percent_layers[i], compositing_mode5);
    layer_add_child(window_layer, bitmap_layer_get_layer(battery_percent_layers[i]));
    }		
	
	
Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 14),
    TupletCString(WEATHER_TEMPERATURE_KEY, ""),
    TupletInteger(INVERT_COLOR_KEY, persist_read_bool(INVERT_COLOR_KEY)),
	TupletInteger(HIDE_SEC_KEY, persist_read_bool(HIDE_SEC_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(BACKGROUND_KEY, persist_read_bool(BACKGROUND_KEY)),
    TupletInteger(BACKGROUND2_KEY, persist_read_bool(BACKGROUND2_KEY)),
    TupletInteger(BACKGROUND3_KEY, persist_read_bool(BACKGROUND3_KEY)),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values), sync_tuple_changed_callback,
                NULL, NULL);

	  appStarted = true;

  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, MONTH_UNIT + DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);	

	// handlers
    battery_state_service_subscribe(&handle_battery);
    bluetooth_connection_service_subscribe(&handle_bluetooth);
	
	// draw first frame
    force_update();

}

static void deinit(void) {
  
  app_sync_deinit(&sync);
	
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  background_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(background_layer2));
  bitmap_layer_destroy(background_layer2);
  gbitmap_destroy(background_image2);
  background_image2 = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(background_layer3));
  bitmap_layer_destroy(background_layer3);
  gbitmap_destroy(background_image3);
  background_image3 = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(separator_layer));
  bitmap_layer_destroy(separator_layer);
  gbitmap_destroy(separator_image);
    
  layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);
  bluetooth_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(icon_layer));
  bitmap_layer_destroy(icon_layer);
  gbitmap_destroy(icon_bitmap);
	
  text_layer_destroy( temp_layer );
	
  layer_remove_from_parent(bitmap_layer_get_layer(battery_layer));
  bitmap_layer_destroy(battery_layer);
  gbitmap_destroy(battery_image);
  battery_image = NULL;
  layer_remove_from_parent(bitmap_layer_get_layer(battery_image_layer));
  bitmap_layer_destroy(battery_image_layer);

	for (int i = 0; i < TOTAL_SECONDS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(seconds_digits_layers[i]));
    gbitmap_destroy(seconds_digits_images[i]);
    seconds_digits_images[i] = NULL;
    bitmap_layer_destroy(seconds_digits_layers[i]);
	seconds_digits_layers[i] = NULL;
    }
	
	for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    bitmap_layer_destroy(date_digits_layers[i]);
    }

    for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    bitmap_layer_destroy(time_digits_layers[i]);
    } 
	
    for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
	layer_remove_from_parent(bitmap_layer_get_layer(battery_percent_layers[i]));
    gbitmap_destroy(battery_percent_images[i]);
    battery_percent_images[i] = NULL;
    bitmap_layer_destroy(battery_percent_layers[i]);
	battery_percent_layers[i] = NULL;
    }
  
  fonts_unload_custom_font( custom_14 );

  layer_remove_from_parent(window_layer);
  layer_destroy(window_layer);
	
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}