/* board.h - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(NODE_ADDR)
#define NODE_ADDR 0x0001//nodes set at 0x000f will send message to node address 0x0001
//0x001 is the receiver (set in publish address main.c file)
#endif

void board_button_1_pressed(void);
u16_t board_set_target(void);//16 bit target microbit.c file
void board_play(const char *str);//play tune for buzzer

#if defined(CONFIG_BOARD_BBC_MICROBIT)//special config for microbit.c
void board_init(u16_t *addr);
void board_play_tune(const char *str);//can remove
void board_heartbeat(u8_t hops, u16_t feat);//can remove 
void board_other_dev_pressed(u16_t addr);//receives messages
void board_attention(bool attention);//debug test health server and config model
#else
static inline void board_init(u16_t *addr)
{
	*addr = NODE_ADDR;
}

static inline void board_play_tune(const char *str)//optional
{
}

void board_heartbeat(u8_t hops, u16_t feat)//optionall
{
}

void board_other_dev_pressed(u16_t addr)//carrys function from microbit.c
{
}

void board_attention(bool attention)//debug test
{
}
#endif
