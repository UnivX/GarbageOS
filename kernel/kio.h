#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "display_interface.h"
#include "kdefs.h"
#include "hal.h"

typedef struct KioState{
	bool is_initialized;
	DisplayInterface display;
} KioState;

//the display interface needs to be initialized
//the display will be finalized during the
//finalize_kio() function
void init_kio(DisplayInterface display);
void finalize_kio();
