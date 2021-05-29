/* main.c - Application main entry point */
/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <misc/printk.h>
#include <settings/settings.h>
#include <bluetooth/bluetooth.h>//bluetooth header
#include <bluetooth/mesh.h>//mesh documentation header
#include "board.h"//reads bbc microbit board.h
#define MOD_LF 0x0000 //vendor model test models are used for node functions 
//mod lf currently is set as off    0x00, 0x00, 0xa8, 0x01, /* Vendor Model Server */
////Models which arnt specified by bluetooth SIG are vendor models
#define GROUP_ADDR 0xc000 //unique group address change to create groups
#define PUBLISHER_ADDR  0x0001 //this acts as the central device 
//devices will sent to the publish address 0x0001 

#define OP_VENDOR_BUTTON BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID_LF)//vendor addresss off

static const u8_t net_key[16] = { //network key setup change to increase security
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t dev_key[16] = {//device key setup change to increase security
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t app_key[16] = {//application key setup change to increase security
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u16_t net_idx; //key index
static const u16_t app_idx;
static const u32_t iv_index;
static u8_t flags; //function for flags setup in zephyr sdk kernal
static u16_t addr = NODE_ADDR;//16bit node address size

//..........................................................
static void heartbeat(u8_t hops, u16_t feat) //optional to remove
{
	board_heartbeat(hops, feat); 
	board_play("100H");
}
//..........................................................
static struct bt_mesh_cfg_srv cfg_srv = {//configuration server model 
//according to ble mesh spec this server controls most parameters of the mesh node
#if defined(CONFIG_BOARD_BBC_MICROBIT)
	.relay = BT_MESH_RELAY_ENABLED, 
	.beacon = BT_MESH_BEACON_DISABLED,
	//config server sets up relay  turn off beacon for microbit to lower memory
#else
	.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_ENABLED,//if not microbit turn on beacon
#endif
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED, //friend not supported for bbcmicrobit 
	.default_ttl = 7,//default ttl is set a 7 

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),   
	.relay_retransmit = BT_MESH_TRANSMIT(3, 20),
	//change these to lower power when sending message

	.hb_sub.func = heartbeat,//remove if desired 
};

static struct bt_mesh_cfg_cli cfg_cli = {//configuration client
};

static void attention_on(struct bt_mesh_model *model)//debug process for health server check
{
	printk("attention_on()\n");
	board_attention(true);
	board_play("100H100C100H100C100H100C");
}

static void attention_off(struct bt_mesh_model *model)//debug 
{
	printk("attention_off()\n");
	board_attention(false);
}

static const struct bt_mesh_health_srv_cb health_srv_cb = { //callback function to check for faults
	.attn_on = attention_on,//makes the device turn into blinking led for testing
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {//callback function to check for faults
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);
static struct bt_mesh_model root_models[] = {//different mesh configurations
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),//config server
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),//config client
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),//heath server debug
};
//more info from https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/reference/bluetooth/mesh/health_srv.html
static void vnd_button_pressed(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	printk("src 0x%04x\n", ctx->addr); //debug button press identifies devices

	if (ctx->addr == bt_mesh_model_elem(model)->addr) {
		return;
	}

	board_other_dev_pressed(ctx->addr);
	board_play("100G200 100G");//debugging
}
static const struct bt_mesh_model_op vnd_ops[] = {//vendor model option
	{ OP_VENDOR_BUTTON, 0, vnd_button_pressed },
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_model vnd_models[] = {//vendor model
	BT_MESH_MODEL_VND(BT_COMP_ID_LF, MOD_LF, vnd_ops, NULL, NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),//defines mesh elemenet within array 
};

static const struct bt_mesh_comp comp = {//vendor models bluetooth SIG ID
	.cid = BT_COMP_ID_LF,//Models which arnt specified by bluetooth SIG are vendor models
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};
static void configure(void)
{
	printk("Configuring...\n");

	/* Add Application Key */
	bt_mesh_cfg_app_key_add(net_idx, addr, net_idx, app_idx, app_key, NULL);

	/* Bind to vendor model */
	bt_mesh_cfg_mod_app_bind_vnd(net_idx, addr, addr, app_idx,
				     MOD_LF, BT_COMP_ID_LF, NULL);

	/* Bind to Health model */
	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
				 BT_MESH_MODEL_ID_HEALTH_SRV, NULL);

	/* Add model subscription */
	bt_mesh_cfg_mod_sub_add_vnd(net_idx, addr, addr, GROUP_ADDR,
				    MOD_LF, BT_COMP_ID_LF, NULL);

#if NODE_ADDR == PUBLISHER_ADDR 
	{
		struct bt_mesh_cfg_hb_pub pub = {//publish configurtion
			.dst = GROUP_ADDR,//point to group address
			.count = 0xff,// count 
			.period = 0x05,//sets publish period
			.ttl = 0x07,//transmisson time to live 
			.feat = 0,
			.net_idx = net_idx,//network key
		};

		bt_mesh_cfg_hb_pub_set(net_idx, addr, &pub, NULL);//configuration publish setting
		printk("Publishing heartbeat messages\n");//heart beat
	}
#endif
	printk("Configuration complete\n");

	board_play("100C100D100E100F100G100A100H");//Once configured bbc microbit lights leds
}
static const u8_t dev_uuid[16] = { 0xdd, 0xdd };//unique identification 16 bits

static const struct bt_mesh_prov prov = {//SETUP AUTO PROVISION
	.uuid = dev_uuid,// USING UUID KEY
};
static void bt_ready(int err)//BLUETOOTH ERROR TEST
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_mesh_init(&prov, &comp);//mesh error testing
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	printk("Mesh initialized\n");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		printk("Loading stored settings\n");
		settings_load();
	}

	err = bt_mesh_provision(net_key, net_idx, flags, iv_index, addr,
				dev_key);
	if (err == -EALREADY) {
		printk("Using stored settings\n");
	} else if (err) {
		printk("Provisioning failed (err %d)\n", err);
		return;
	} else {
		printk("Provisioning completed\n");
		configure();
	}
#if NODE_ADDR != PUBLISHER_ADDR
	/* Heartbeat subcscription is a temporary state (due to there
	 * not being an "indefinite" value for the period, so it never
	 * gets stored persistently. Therefore, we always have to configure
	 * it explicitly.
	 */
	{
		struct bt_mesh_cfg_hb_sub sub = {//self subscribing heart beat message
			.src = PUBLISHER_ADDR,
			.dst = GROUP_ADDR,
			.period = 0x10,
		};

		bt_mesh_cfg_hb_sub_set(net_idx, addr, &sub, NULL);
		printk("Subscribing to heartbeat messages\n");
	}
#endif
}
static u16_t target = GROUP_ADDR; //set 16bit target group address
void board_button_1_pressed(void)
{
	NET_BUF_SIMPLE_DEFINE(msg, 3 + 4);//helper macro in zephyr used to define neet buf simple msg on stack
	struct bt_mesh_msg_ctx ctx = {//when button pressed check mesh keys 
		.net_idx = net_idx,
		.app_idx = app_idx,
		.addr = target,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	/* Bind to Health model */
	bt_mesh_model_msg_init(&msg, OP_VENDOR_BUTTON);

	if (bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL)) {
		printk("Unable to send Vendor Button message\n");
	}

	printk("Button message sent with OpCode 0x%08x\n", OP_VENDOR_BUTTON);
	// shows button pressed on device
}
u16_t board_set_target(void)
{
	switch (target) {
	case GROUP_ADDR:
		target = 1U;//special address for in zephyr 
		break;
	case 9:
		target = GROUP_ADDR;
		break;
	default:
		target++;
		break;
	}

	return target;
}
static K_SEM_DEFINE(tune_sem, 0, 1);//plays board tune if buzzer set
static const char *tune_str;

void board_play(const char *str)
{
	tune_str = str;
	k_sem_give(&tune_sem);
}

void main(void)
{
	int err;

	printk("Initializing...\n");

	board_init(&addr);//looks for address and board

	printk("Unicast address: 0x%04x\n", addr);//displays unicast address set

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	while (1) {//option to remove
		k_sem_take(&tune_sem, K_FOREVER);
		board_play_tune(tune_str);
	}
}
