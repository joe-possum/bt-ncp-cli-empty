/* standard library headers */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"

/* Own header */
#include "app.h"
#include "dump.h"
#include "support.h"

// App booted flag
static bool appBooted = false;
static struct {
  char *name;
  uint32 advertising_interval;
  uint16 connection_interval, mtu; 
  bd_addr remote;
  uint8 advertise, connection;
} config = { .remote = { .addr = {0,0,0,0,0,0}},
	     .connection = 0xff,
	     .advertise = 1,
	     .name = NULL,
	     .advertising_interval = 160, // 100 ms
	     .connection_interval = 80, // 100 ms
	     .mtu = 23,
};
  
void parse_address(const char *fmt,bd_addr *address) {
  char buf[3];
  int octet;
  for(uint8 i = 0; i < 6; i++) {
    memcpy(buf,&fmt[3*i],2);
    buf[2] = 0;
    sscanf(buf,"%02x",&octet);
    address->addr[5-i] = octet;
  }
}

uint8 ad_flags(uint8 *buffer, uint flags) {
  if(0 == flags) return 0;
  buffer[0] = 2;
  buffer[1] = 1;
  buffer[2] = flags;
  return 3;
}

uint8 ad_name(uint8 *buffer, char *name) {
  uint8 len = strlen(name);
  buffer[0] = len + 1;
  buffer[1] = 9;
  memcpy(&buffer[2],(uint8*)&name[0],len);
  return len + 2;
}

uint8 ad_manufacturer(uint8 *buffer, uint8 len, uint8 *data) {
  buffer[0] = len + 3;
  buffer[1] = 0xff;
  buffer[2] = 0xff;
  buffer[3] = 0xff;
  memcpy(&buffer[4],data,len);
  return len + 4;
}

const char *getAppOptions(void) {
  return "a<remote-address>n<name>";
}

void appOption(int option, const char *arg) {
  double dv;
  switch(option) {
  case 'a':
    parse_address(arg,&config.remote);
    config.advertise = 0;
    break;
  case 'i':
    sscanf(arg,"%lf",&dv);
    config.advertising_interval = round(dv/0.625);
    config.connection_interval = round(dv/1.25);
    break;
  case 'n':
    config.name = strdup(arg);
    break;
  default:
    fprintf(stderr,"Unhandled option '-%c'\n",option);
    exit(1);
  }
}

void appInit(void) {
  if(config.advertise) return;
  for(int i = 0; i < 6; i++) {
    if(config.remote.addr[i]) return;
  }
  printf("Usage: master [ -a <address> ]\n");
  exit(1);
}

/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
void appHandleEvents(struct gecko_cmd_packet *evt)
{
  if (NULL == evt) {
    return;
  }

  // Do not handle any events until system is booted up properly.
  if ((BGLIB_MSG_ID(evt->header) != gecko_evt_system_boot_id)
      && !appBooted) {
#if defined(DEBUG)
    printf("Event: 0x%04x\n", BGLIB_MSG_ID(evt->header));
#endif
    millisleep(50);
    return;
  }

  /* Handle events */
#ifdef DUMP
  switch (BGLIB_MSG_ID(evt->header)) {
  default:
    dump_event(evt);
  }
#endif
  switch (BGLIB_MSG_ID(evt->header)) {
  case gecko_evt_system_boot_id:
    appBooted = true;
    if(config.advertise) {
      uint8 discoverable_mode = le_gap_general_discoverable;
      if(config.name) {
	uint8 buf[31];
	uint len = 0;
	len += ad_flags(&buf[len],6);
	len += ad_name(&buf[len],config.name);
	gecko_cmd_le_gap_bt5_set_adv_data(0,0,len,buf);
	discoverable_mode = le_gap_user_data;
      }
      gecko_cmd_le_gap_set_advertise_timing(0,config.advertising_interval,config.advertising_interval,0,0);
      gecko_cmd_le_gap_start_advertising(0,discoverable_mode,le_gap_connectable_scannable);
      gecko_cmd_system_get_bt_address();
    } else {
      gecko_cmd_le_gap_connect(config.remote,le_gap_address_type_public,le_gap_phy_1m);
    }
    break;

  case gecko_evt_le_connection_opened_id: /***************************************************************** le_connection_opened **/
#define ED evt->data.evt_le_connection_opened
    config.connection = ED.connection;
    break;
#undef ED

  case gecko_evt_gatt_mtu_exchanged_id: /********************************************************************* gatt_mtu_exchanged **/
#define ED evt->data.evt_gatt_mtu_exchanged
    config.mtu = ED.mtu;
    break;
#undef ED

  case gecko_evt_le_connection_closed_id:
    exit(1);
    break;

  default:
    break;
  }
}
