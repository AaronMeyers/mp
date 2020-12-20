// ----------------------------------------------------------------------------
// defines functions for multipass to send events to the controller (grid 
// presses etc)
//
// defines functions for engine to send updates (note on etc)
//
// defines data structures for multipass preset management
// ----------------------------------------------------------------------------

#pragma once
#include "types.h"


// ----------------------------------------------------------------------------
// firmware dependent stuff starts here
#define ER_COUNT 4
#define PATTERN_COUNT 16

// ----------------------------------------------------------------------------
// shared types

typedef enum {
    ek_steps,
    ek_fill,
    ek_rotation
} ek_button_t;

typedef struct {
    u8 steps;
    u8 fill;
    u8 rotation;
    u8 index;
} ek_er_t;

typedef struct {
    ek_er_t er[ER_COUNT];
} ek_pattern_t;

typedef struct {
    ek_pattern_t p[PATTERN_COUNT];
} preset_data_t;

typedef struct {
} preset_meta_t;

typedef struct {
    ek_button_t active_button[ER_COUNT];
} shared_data_t;


// ----------------------------------------------------------------------------
// firmware settings/variables main.c needs to know


// ----------------------------------------------------------------------------
// functions control.c needs to implement (will be called from main.c)

void init_presets(void);
void init_control(void);
void process_event(u8 event, u8 *data, u8 length);
void render_grid(void);
void render_arc(void);


// ----------------------------------------------------------------------------
// functions engine needs to call
