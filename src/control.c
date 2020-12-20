// -----------------------------------------------------------------------------
// controller - the glue between the engine and the hardware
//
// reacts to events (grid press, clock etc) and translates them into appropriate
// engine actions. reacts to engine updates and translates them into user 
// interface and hardware updates (grid LEDs, CV outputs etc)
//
// should talk to hardware via what's defined in interface.h only
// should talk to the engine via what's defined in engine.h only
// ----------------------------------------------------------------------------

#include "compiler.h"
#include "string.h"

#include "control.h"
#include "interface.h"
#include "engine.h"

#include "euclidean.h"

#define GRID_LED_LOW 3
#define GRID_LED_MEDIUM 6
#define GRID_LED_HIGH 9

#define GATE_OFF_TIMER 0
#define FIXED_GATE_LENGTH 5

preset_meta_t meta;
preset_data_t preset;
shared_data_t shared;
int selected_preset;
int selected_pattern;

// ----------------------------------------------------------------------------
// firmware dependent stuff starts here

static void grid_filled_rect( u8 x, u8 y, u8 width, u8 height, u8 brightness );
static void handler_ek_grid_key( u8 x, u8 y, u8 z );
static void event_main_clock( u8 external, u8 phase );
static void external_clock_received( void );
static void process_timed_event( u8 index );

// ----------------------------------------------------------------------------
// functions for main.c

void init_presets(void) {
    // called by main.c if there are no presets saved to flash yet
    // initialize meta - some meta data to be associated with a preset, like a glyph
    // initialize shared (any data that should be shared by all presets) with default values
    // initialize preset with default values 
    // store them to flash
    
    for (u8 i = 0; i < get_preset_count(); i++) {
        for ( u8 j = 0; j < PATTERN_COUNT; j++ ) {
            for ( u8 k = 0; k < ER_COUNT; k++ ) {
                preset.p[j].er[k].steps = 16;
                preset.p[j].er[k].fill = 0;
                preset.p[j].er[k].rotation = 0;
            }
        }
        store_preset_to_flash(i, &meta, &preset);
    }

    for ( u8 i = 0; i < ER_COUNT; i++ ) {
        shared.active_button[i] = ek_steps;
    }

    store_shared_data_to_flash(&shared);
    store_preset_index(0);
}

void init_control(void) {
    // load shared data
    // load current preset and its meta data
    load_shared_data_from_flash(&shared);
    selected_preset = get_preset_index();
    load_preset_from_flash(selected_preset, &preset);
    load_preset_meta_from_flash(selected_preset, &meta);

    // set up any other initial values and timers
    clear_all_grid_leds();
    refresh_grid();
}

void process_event(u8 event, u8 *data, u8 length) {
    switch (event) {
        case MAIN_CLOCK_RECEIVED:
            event_main_clock( data[0], data[1] );
            break;
        
        case MAIN_CLOCK_SWITCHED:
            break;
    
        case GATE_RECEIVED:
            break;
        
        case GRID_CONNECTED:
            break;
        
        case GRID_KEY_PRESSED:
            handler_ek_grid_key( data[0], data[1], data[2] );
            break;
    
        case GRID_KEY_HELD:
            break;
            
        case ARC_ENCODER_COARSE:
            break;
    
        case FRONT_BUTTON_PRESSED:
            break;
    
        case FRONT_BUTTON_HELD:
            break;
    
        case BUTTON_PRESSED:
            break;
    
        case I2C_RECEIVED:
            break;
            
        case TIMED_EVENT:
            process_timed_event( data[0] );
            break;
            
        default:
            break;
    }
}

void render_grid(void) {
    if ( !is_grid_connected() ) return;

    clear_all_grid_leds();

    u8 steps, fill, rotation, index;
    ek_button_t active_button;

    for ( u8 i = 0; i < ER_COUNT; i++) {
        steps = preset.p[selected_pattern].er[i].steps;
        fill = preset.p[selected_pattern].er[i].fill;
        rotation = preset.p[selected_pattern].er[i].rotation;
        index = preset.p[selected_pattern].er[i].index;

        active_button = shared.active_button[i];

        // light the steps in the current settings
        for ( u8 j = 0; j < steps; j++ ) {
            u8 er_step = euclidean( fill, steps, ( index + rotation ) % steps );
            set_grid_led( j, i * 2, er_step ? GRID_LED_MEDIUM : GRID_LED_LOW );
        }
        // make the current step bright
        set_grid_led( index, i * 2, GRID_LED_HIGH );
        // draw buttons
        grid_filled_rect( 0, i * 2 + 1, 3, 1, active_button == ek_steps ? GRID_LED_MEDIUM : GRID_LED_LOW );
        grid_filled_rect( 4, i * 2 + 1, 3, 1, active_button == ek_fill ? GRID_LED_MEDIUM : GRID_LED_LOW );
        grid_filled_rect( 4, i * 2 + 1, 3, 1, active_button == ek_rotation ? GRID_LED_MEDIUM : GRID_LED_LOW );
    }
}

void render_arc(void) {
    // render arc LEDs here or leave blank if not used
}


static void process_timed_event( u8 index ) {
    if ( index >= GATE_OFF_TIMER && index < GATE_OFF_TIMER + 4 ) {
        set_gate( index - GATE_OFF_TIMER, 0 );
    }
}

static void event_main_clock( u8 external, u8 phase )
{
    if ( external & phase ) external_clock_received();
}

static void external_clock_received() {
    u8 steps, fill, rotation, index;

    for ( u8 i = 0; i < ER_COUNT; i++ ) {
        steps = preset.p[selected_pattern].er[i].steps;
        fill = preset.p[selected_pattern].er[i].fill;
        rotation = preset.p[selected_pattern].er[i].rotation;
        index = preset.p[selected_pattern].er[i].index;

        if ( ++index >= steps ) {
            index = 0;
        }
        preset.p[selected_pattern].er[i].index = index;

        if ( euclidean( fill, steps, ( index + rotation ) % steps ) ) {
            // output a gate
            set_gate( i, 1 );
            add_timed_event( GATE_OFF_TIMER + i, FIXED_GATE_LENGTH, 0 );
        }
    }
    refresh_grid();
}

static void handler_ek_grid_key( u8 x, u8 y, u8 z ) {
    // handle if it is one of the buttons
    if ( z && y % 2 == 1 ) {
        u8 er_index = ( y - 1 ) / 2;
        if ( x >= 0 && x <= 2 ) {
            shared.active_button[er_index] = ek_steps;
            refresh_grid();
        }
        else if ( x >= 4 && x <= 6 ) {
            shared.active_button[er_index] = ek_fill;
            refresh_grid();
        }
        else if ( x >= 8 && x <= 10 ) {
            shared.active_button[er_index] = ek_rotation;
            refresh_grid();
        }
    }
    // handle if it is one of the steps
    if ( z && y % 2 == 0 )
    {
        u8 er_index = y / 2;
        u8 active_button = shared.active_button[er_index];
        u8 steps = preset.p[selected_pattern].er[er_index].steps;
        u8 index = preset.p[selected_pattern].er[er_index].index;

        if ( active_button == ek_steps && x > 0 ) {
            steps = x + 1;
            preset.p[selected_pattern].er[er_index].steps = steps;
            if ( index >= steps ) {
                preset.p[selected_pattern].er[er_index].index = steps - 1;
            }
            refresh_grid();
        }
        else if ( active_button == ek_fill && x < steps ) {
            preset.p[selected_pattern].er[er_index].fill = x + 1;
            refresh_grid();
        }
        else if ( active_button == ek_rotation && x < steps ) {
            preset.p[selected_pattern].er[er_index].rotation = x + 1;
            refresh_grid();
        }
    }
}

static void grid_filled_rect( u8 x, u8 y, u8 width, u8 height, u8 brightness ) {
    for ( u8 i = 0; i < width; i++ ) {
        for ( u8 j = 0; j < height; j++ ) {
            set_grid_led( x + i, y + j, brightness );
        }
    }
}