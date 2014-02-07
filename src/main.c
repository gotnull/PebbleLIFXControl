#include <pebble.h>

/* #define statements used throughout this file for convenience. 
*/
#define ON_OFF_LENGTH (23) // Length, in bytes, an exchange's name can be.
#define SUBFIELD_LENGTH (23) // Length, in bytes, a price field can be.

#define NUMBER_OF_ITEMS (2) // The number of exchanges. Increment when an exchange is added.
#define FIRST_INDEX (0) // Index in the exchage_data_list array that contains Bitstamp data.
#define SECOND_INDEX (1) // Index in the watch_data_list array that contians Mt. Gox data.

static Window *window;
static MenuLayer *watch_menu;

/* A list of keys used to exchange messages between the watch app and the 
   accompanying JavaScript loaded into the Pebble smartphone app.

   This list should mirror the list of appkeys found in the appinfo.json file.
*/
enum {
    FIRST_KEY = 100, // Informs the JavaScript code to fetch prices from Bitstamp.
    FIRST_KEY_LAST = 103, // Used by the JavaScript code to return Bitstamp's last price.
	
    SECOND_KEY = 200,
    SECOND_KEY_LAST = 203,
};

/* A structure to contain an exchange's information. Each exchange should have
   one struct whose index is based off of the above #define statements.
*/
typedef struct {
    char exchange_name[ON_OFF_LENGTH];
    char last[SUBFIELD_LENGTH];
} ServiceData;

static ServiceData watch_data_list[NUMBER_OF_ITEMS];

/* Returns the ServiceData for the exchange at index. For a good time use the 
   exchange related #defines at the beginning of this file when calling this 
   function.
*/
static ServiceData* get_data(int index) {
    if (index < 0 || index >= NUMBER_OF_ITEMS) {
        return NULL;
    }

    return &watch_data_list[index];
}

/* Asks the JavaScript code loaded on the smartphone's Pebble app to fetch
   prices from Bitcoin exchanges.
*/
static void fetch_message(int index) {
	DictionaryIterator* iter;
	
	app_message_outbox_begin(&iter);

	if (index > 0) {
		dict_write_cstring(iter, 1, "OFF");
	} else {
		dict_write_cstring(iter, 1, "ON");
	}
    dict_write_end(iter);
    
	app_message_outbox_send();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {}
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {}
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {}
static void out_sent_handler(DictionaryIterator *sent, void *context) {}
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {}

static void select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    const int index = cell_index->row;
    fetch_message(index);
}

static void select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    const int index = cell_index->row;
    fetch_message(index);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    return 44;
}

static uint16_t get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return NUMBER_OF_ITEMS;
}

static void draw_row_callback(GContext* ctx, Layer *cell_layer, MenuIndex *cell_index, void *data) {
    ServiceData *watch_data;
    const int index = cell_index->row;

    if ((watch_data = get_data(index)) == NULL) {
        return;
    }

    menu_cell_basic_draw(ctx, cell_layer, watch_data->exchange_name, watch_data->last, NULL);
}

static void in_received_handler(DictionaryIterator *received, void *context) {
    Tuple *bitstamp_exchange = dict_find(received, FIRST_KEY);
    Tuple *mtgox_exchange = dict_find(received, SECOND_KEY);

    // Load the prices for Bitstamp into watch_data_list.
    if (bitstamp_exchange) {
        Tuple *last = dict_find(received, FIRST_KEY_LAST);

        if (last) {
            strncpy(watch_data_list[FIRST_INDEX].last, last->value->cstring, SUBFIELD_LENGTH);
        }
    }

    // Load the prices for Mt. Gox into exchange_datea_list.
    if (mtgox_exchange) {
        Tuple *last = dict_find(received, SECOND_KEY_LAST);

        if (last) {
            strncpy(watch_data_list[SECOND_INDEX].last, last->value->cstring, SUBFIELD_LENGTH);
        }
    }

    menu_layer_reload_data(watch_menu);
}

static void in_dropped_handler(AppMessageResult reason, void *context) {}
static void click_config_provider(void *context) {}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    strncpy(watch_data_list[FIRST_INDEX].exchange_name, "ON\0", ON_OFF_LENGTH);
    strncpy(watch_data_list[FIRST_INDEX].last, "Turn on the light(s).\0", SUBFIELD_LENGTH);

    strncpy(watch_data_list[SECOND_INDEX].exchange_name, "OFF\0", ON_OFF_LENGTH);
    strncpy(watch_data_list[SECOND_INDEX].last, "Turn off the light(s).\0", SUBFIELD_LENGTH);

    watch_menu = menu_layer_create(bounds);
    menu_layer_set_callbacks(watch_menu, NULL, (MenuLayerCallbacks) {
        .get_cell_height = (MenuLayerGetCellHeightCallback) get_cell_height_callback,
        .draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
        .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) get_num_rows_callback,
        .select_click = (MenuLayerSelectCallback) select_callback,
        .select_long_click = (MenuLayerSelectCallback) select_long_callback
    });
    menu_layer_set_click_config_onto_window(watch_menu, window);
    layer_add_child(window_layer, menu_layer_get_layer(watch_menu));

    //fetch_message();
}

static void window_unload(Window *window) {}

// Register any app message handlers.
static void app_message_init(void) {
    app_message_register_outbox_sent(out_sent_handler);
    app_message_register_outbox_failed(out_failed_handler);
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);

    app_message_open(128, 128);
}

static void init(void) {
  window = window_create();
  app_message_init();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
    window_destroy(window);
    menu_layer_destroy(watch_menu);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}