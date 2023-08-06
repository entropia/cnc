/********************************************************************
* Description:  spigen.c
*               A HAL component that generates SPI output
*
* Author: Florian Larysch <fl@n621.de>
* License: GPL Version 2
*
* Copyright (c) 2014
*
********************************************************************/

#include "rtapi.h"
#include "rtapi_app.h"
#include "hal.h"

MODULE_AUTHOR("Florian Larysch");
MODULE_DESCRIPTION("SPI generator for LinuxCNC HAL");
MODULE_LICENSE("GPL");

int wordsize = 16;
RTAPI_MP_INT(wordsize, "Number of bits to send per transfer");

int retransmit_count = 3;
RTAPI_MP_INT(retransmit_count, "Number of times to transmit a value");

long clock_hz = 16000;
RTAPI_MP_LONG(clock_hz, "SPI clock frequency");

long min_pause = 800000;
RTAPI_MP_LONG(min_pause, "Minimal pause between transmissions");

/***********************************************************************
*                STRUCTURES AND GLOBAL VARIABLES                       *
************************************************************************/

typedef enum {
	IDLE, PRETICK, TICK, HOLDOFF_DISABLE, PAUSE
} spigen_transfer_state;

typedef struct {
	// internal state
	int last_enabled;

	spigen_transfer_state state;

	int remaining_transmits;
	uint32_t last_value;

	int bit;

	long long nsecs;
	long long halftick;

	// inputs
	hal_u32_t *value;
	hal_bit_t *enable;

	// outputs
	hal_bit_t *cs, *clock, *data;
} spigen_t;

static spigen_t *spigen_array;

/* other globals */
static int comp_id;             /* component ID */

/***********************************************************************
*                  LOCAL FUNCTION DECLARATIONS                         *
************************************************************************/

static int export_spigen(int num, spigen_t *spigen);
static void do_transfer(void *arg, long period);

/***********************************************************************
*                       INIT AND EXIT CODE                             *
************************************************************************/

int rtapi_app_main(void) {
	int retval;

	if(wordsize > 32) {
		rtapi_print_msg(RTAPI_MSG_ERR,
		    "SPIGEN: ERROR: Wordsize '%d' is too big\n",
		    wordsize);

		return -1;
	}

	comp_id = hal_init("spigen");
	if(comp_id < 0) {
		rtapi_print_msg(RTAPI_MSG_ERR, "SPIGEN: ERROR: hal_init() failed\n");
		return -1;
	}

	spigen_array = hal_malloc(1 * sizeof(spigen_t));
	if(spigen_array == 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
				"SPIGEN: ERROR: hal_malloc() failed\n");
		hal_exit(comp_id);
		return -1;
	}

	retval = export_spigen(0, spigen_array);
	if(retval != 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
			"SPIGEN: ERROR: spigen%d var export failed\n",
			0);

		hal_exit(comp_id);
		return -1;
	}

	retval = hal_export_funct("spigen.do-transfer", do_transfer,
			spigen_array, 0, 0, comp_id);
	if(retval != 0) {
		rtapi_print_msg(RTAPI_MSG_ERR,
			"SPIGEN: ERROR: do-transfer funct export failed\n");

		hal_exit(comp_id);
		return -1;
	}

	rtapi_print_msg(RTAPI_MSG_INFO,
			"PWMGEN: installed successfully\n");

	hal_ready(comp_id);

	return 0;
}

void rtapi_app_exit(void) {
	hal_exit(comp_id);
}

/***********************************************************************
*                         REALTIME CODE                                *
************************************************************************/

static void set_cs(spigen_t *spigen, int cs) {
	*(spigen->cs) = !cs;
}

static void next_bit(spigen_t *spigen) {
	spigen->bit--;

	*(spigen->data) = (spigen->last_value >> spigen->bit) & 1;
}

static void do_transfer(void *arg, long period) {
	spigen_t *spigen = arg;

	spigen->nsecs += period;

	if(spigen->state == IDLE) {
		int enabled = *(spigen->enable);
		uint32_t val = *(spigen->value);

		if(!enabled) {
			spigen->last_enabled = enabled;
			return;
		}

		if(spigen->last_enabled != enabled) { // force retransmit on enable
			spigen->last_enabled = enabled;

			spigen->remaining_transmits = retransmit_count;
		}

		if(val != spigen->last_value && !spigen->remaining_transmits)
			spigen->remaining_transmits = retransmit_count;

		if(spigen->remaining_transmits) {
			spigen->last_value = val;
			spigen->remaining_transmits--;

			set_cs(spigen, 1);

			spigen->bit = wordsize;
			next_bit(spigen);

			*(spigen->clock) = 0;

			spigen->nsecs = 0;
			spigen->state = PRETICK;
		}
	} else if(spigen->state == PRETICK) {
		if(spigen->nsecs >= spigen->halftick) {
			*(spigen->clock) = 1;

			spigen->nsecs = 0;
			spigen->state = TICK;
		}
	} else if(spigen->state == TICK) {
		if(spigen->nsecs >= spigen->halftick) {
			*(spigen->clock) = 0;

			spigen->nsecs = 0;
			if(spigen->bit) {
				next_bit(spigen);
				spigen->state = PRETICK;
			} else {
				*(spigen->data) = 0;
				spigen->state = HOLDOFF_DISABLE;
			}
		}
	} else if(spigen->state == HOLDOFF_DISABLE) {
		if(spigen->nsecs >= spigen->halftick) {
			set_cs(spigen, 0);

			spigen->nsecs = 0;
			spigen->state = PAUSE;
		}
	} else if(spigen->state == PAUSE) {
		if(spigen->nsecs >= min_pause) {
			spigen->nsecs = 0;
			spigen->state = IDLE;
		}
	}
}

/***********************************************************************
*                   LOCAL FUNCTION DEFINITIONS                         *
************************************************************************/

static int export_spigen(int num, spigen_t *spigen) {
	int retval;

	// inputs
	retval = hal_pin_u32_newf(HAL_IN, &(spigen->value), comp_id,
			"spigen.%d.value", num);

	if(retval != 0)
		return retval;

	retval = hal_pin_bit_newf(HAL_IN, &(spigen->enable), comp_id,
			"spigen.%d.enable", num);

	if(retval != 0)
		return retval;

	// outputs
	retval = hal_pin_bit_newf(HAL_OUT, &(spigen->cs), comp_id,
			"spigen.%d.cs", num);

	if(retval != 0)
		return retval;

	retval = hal_pin_bit_newf(HAL_OUT, &(spigen->clock), comp_id,
			"spigen.%d.clock", num);

	if(retval != 0)
		return retval;

	retval = hal_pin_bit_newf(HAL_OUT, &(spigen->data), comp_id,
			"spigen.%d.data", num);

	if(retval != 0)
		return retval;

	// setup default pin values
	set_cs(spigen, 0);
	*(spigen->clock) = 0;
	*(spigen->data) = 0;
	*(spigen->enable) = 1;

	// initialize the structure
	spigen->remaining_transmits = 0;
	spigen->last_enabled = 0;
	spigen->nsecs = 0;
	spigen->halftick = (1000L * 1000L * 1000L) / (clock_hz * 2);

	return 0;
}
