/* microbit.c - BBC micro:bit specific hooks */
/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <gpio.h>//sets gpio pins 
#include <board.h>//board layout
#include <soc.h>//registers
#include <misc/printk.h>//print conditions
#include <ctype.h>//
#include <gpio.h>//config pins
#include <pwm.h>//ignore maybe
#include <display/mb_display.h>//led display on microbit
#include <bluetooth/mesh.h>//ble mesh config
#include "board.h"//board file
#define SCROLL_SPEED   K_MSEC(300)//scroll speed set for LEDS
#define delay  	       k_sleep(20000)
#define BUZZER_PIN     EXT_P7_GPIO_PIN //Left in code as it can be used to add an alarm with the motion sensor
#define BEEP_DURATION  K_MSEC(60)//duration of buzzer 
// https://lists.zephyrproject.org/g/devel/topic/16760514?p=,,,20,0,0,0::recentpostdate%2Fsticky,,,20,2,400,16760514
#define SEQ_PER_BIT  976
#define SEQ_PAGE     (NRF_FICR->CODEPAGESIZE * (NRF_FICR->CODESIZE - 1))//two lines of code used to allow flashing
#define SEQ_MAX      (NRF_FICR->CODEPAGESIZE * 8 * SEQ_PER_BIT)// code clears one bit at a time to stop flash erasing 
//this is why sequence numbers are defined  , NOTE: Due to an update this can be removed as flash access 
//if coordinated with radio acces now, left in to be safe
static struct device *gpio;//sets gpio ports
static struct device *nvm;//nvm structure for device
static struct device *pwm;//creats pulse width structure for buzzer 

static struct k_work button_work;// K is used for zephyr kernal, k_work button allows button on the mcirobit to be pressed

static void button_send_pressed(struct k_work *work)//function which reports when button is pressed
{
	printk("button_send_pressed()\n");//alerts user prints to serial terminal
	board_button_1_pressed();//returns to function
}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   u32_t pins)//initilizes the button on the board 
{
	struct mb_display *disp = mb_display_get();//LED display 

	if (pins & BIT(SW0_GPIO_PIN)) {//sets the GPIO pin(s) when left button on microbit is pressed send message
		k_work_submit(&button_work);//submit button press
	} else {
		u16_t target = board_set_target();// sets the function to create a 16bit variable 

		if (target > 0x0009) {//range of numbers to display on LED
		mb_display_print(disp, MB_DISPLAY_MODE_SINGLE, //turn off LEDS
					 K_SECONDS(2), "A");
		} else {
		mb_display_print(disp, MB_DISPLAY_MODE_SINGLE,
					 K_SECONDS(2), "%X", (target & 0xf));
		}
	}
}
//BELOW CAN BE REMOVED !!
static const struct { //related to the buzzer period time
	char  note;
	u32_t period;
	u32_t sharp;
} period_map[] = {
	{ 'C',  3822,  3608 },
	{ 'D',  3405,  3214 },
	{ 'E',  3034,  3034 },
	{ 'F',  2863,  2703 },
	{ 'G',  2551,  2407 },
	{ 'A',  2273,  2145 },
	{ 'B',  2025,  2025 },
};

static u32_t get_period(char note, bool sharp)//setting the period relating to the period map
{
	int i;

	if (note == ' ') {
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(period_map); i++) {
		if (period_map[i].note != note) {
			continue;
		}

		if (sharp) {
			return period_map[i].sharp;
		} else {
			return period_map[i].period;
		}
	}

	return 1500;
}

void board_play_tune(const char *str)//plays the alaram
{
	while (*str) {
		u32_t period, duration = 0U;

		while (*str && !isdigit((unsigned char)*str)) {
			str++;
		}

		while (isdigit((unsigned char)*str)) {
			duration *= 10U;
			duration += *str - '0';
			str++;
		}

		if (!*str) {
			break;
		}

		if (str[1] == '#') {
			period = get_period(*str, true);
			str += 2;
		} else {
			period = get_period(*str, false);
			str++;
		}

		if (period) {
		pwm_pin_set_usec(pwm, BUZZER_PIN, period, period / 2U);
		}

		k_sleep(duration);

		/* Disable the PWM */
	pwm_pin_set_usec(pwm, BUZZER_PIN, 0, 0);
	
	
		
	}
}
//ABOVE CAN BE REMOVED !!
void board_heartbeat(u8_t hops, u16_t feat)//TESTING PURPOSE ALLOWS HEARTBEAT PUBLISH TO DEMONSTRATE MESH NETWORK
{
	struct mb_display *disp = mb_display_get();//LIGHTS LEDS WHEN NODES ARE ALL SET AT SAME ID 
	const struct mb_image hops_img[] = {
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 0, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 0, 1, 0, 1 },
			 { 0, 0, 0, 0, 0 },
			 { 1, 0, 0, 0, 1 },
			 { 0, 0, 0, 0, 0 },
			 { 1, 0, 1, 0, 1 })
	};

	printk("%u hops\n", hops);//DEBUGGING PURPOSE

	if (hops) {
		hops = MIN(hops, ARRAY_SIZE(hops_img));
		mb_display_image(disp, MB_DISPLAY_MODE_SINGLE, K_SECONDS(2),//DISPLAY ON MICROBIT FOR 2 SECOND INTERVALS 
				 &hops_img[hops - 1], 1);
	}
}
//IMPORTAN BELOW MESSAGE RECEIVER
void board_other_dev_pressed(u16_t addr)//Function which allows nodes to read/relay messages
{
	struct mb_display *disp = mb_display_get();//Lights LEDS when microbit receives message (TURN OFF TO save power)

	printk("board_other_dev_pressed(0x%04x)\n", addr);//work packets Printing across mesh network
	printk("Warning Motion Detected!\n");
	mb_display_print(disp, MB_DISPLAY_MODE_SINGLE, K_SECONDS(2),//display led message received
			 "%X", (addr & 0xf));//reserved special adddress for bbc microbit board TURN OFF TO SAVE POWER
    delay;
	printk("Clear\n");
}

void board_attention(bool attention)//debug testing for health server
{
	struct mb_display *disp = mb_display_get();//relates to above setting LEDS on 
	static const struct mb_image attn_img[] = {//TRUN OFF TO REMOVE LEDS  IMPROVE POWER
		MB_IMAGE({ 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 1, 0, 0 },      //testing leds for health server check
			 { 0, 0, 0, 0, 0 },
			 { 0, 0, 0, 0, 0 }),
		MB_IMAGE({ 0, 0, 0, 0, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 1, 1, 1, 0 },
			 { 0, 0, 0, 0, 0 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 0, 1, 1 },
			 { 1, 1, 1, 1, 1 },
			 { 1, 1, 1, 1, 1 }),
		MB_IMAGE({ 1, 1, 1, 1, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 0, 0, 0, 1 },
			 { 1, 1, 1, 1, 1 }),
	};

	if (attention) {//used along with the health server to test and debug nodes error finding
		mb_display_image(disp,
				 MB_DISPLAY_MODE_DEFAULT | MB_DISPLAY_FLAG_LOOP,
				 K_MSEC(150), attn_img, ARRAY_SIZE(attn_img));
	} else {
		mb_display_stop(disp);//stop displaying testing when turned off 
	}
}

static void configure_button(void)//setting the button config
//reads the bbc microbit board.h file to get the GPIO pin for the button(SW0 AND SW1)
{
		
	static struct gpio_callback button_cb;

	k_work_init(&button_work, button_send_pressed);//ZEPHYR kernal button working send button function

	gpio = device_get_binding(SW0_GPIO_CONTROLLER);//setting variable named gpio to board config button SW0(left button on microbit)

	gpio_pin_configure(gpio, SW0_GPIO_PIN, //setting what the left button does
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |//gpio interupt pin , edge trigger
			    GPIO_INT_ACTIVE_LOW)); //triggers the pin on press (low) 
	gpio_pin_configure(gpio, SW1_GPIO_PIN,//set up right button on microbit
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |//gpio_dir_in sets the pin to be an input
			    GPIO_INT_ACTIVE_LOW));

	gpio_init_callback(&button_cb, button_pressed, //when button is pressed do the tasks initilized 
			   BIT(SW0_GPIO_PIN) | BIT(SW1_GPIO_PIN));//Call back pins sw0 and sw1
	gpio_add_callback(gpio, &button_cb);//calls back to gpio and button call back

	gpio_pin_enable_callback(gpio, SW0_GPIO_PIN);//calls back 
	gpio_pin_enable_callback(gpio, SW1_GPIO_PIN);
}

void board_init(u16_t *addr)
{
	struct mb_display *disp = mb_display_get();//led settins

	nvm = device_get_binding(DT_FLASH_DEV_NAME);//board.h for bbc microbit sets the device to nvm allows flashing through cmd 
	pwm = device_get_binding(CONFIG_PWM_NRF5_SW_0_DEV_NAME);//sets pulse width for a buzzer/alarm  
	*addr = NRF_UICR->CUSTOMER[0];//points to node address setting
	if (!*addr || *addr == 0xffff) {//if the address == 0xffff define addres to be equal node address
#if defined(NODE_ADDR)
		*addr = NODE_ADDR;
#else
		*addr = 0x0b0c;// special heart beat address for testing
#endif
	}

	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, SCROLL_SPEED,
			 "0x%04x", *addr);// display message when microbit turns on

	configure_button();//configure buttons on microbit
}
