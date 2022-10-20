
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_


// Set the max response payload before enable fragmentation:

#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE    64

#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     10

#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES   10

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    240

#define LOG_LEVEL_APP LOG_LEVEL_DBG

#ifndef WEBSERVER_CONF_CFS_CONNS
#define WEBSERVER_CONF_CFS_CONNS 2
#endif

#ifndef BORDER_ROUTER_CONF_WEBSERVER
#define BORDER_ROUTER_CONF_WEBSERVER 1
#endif

#if BORDER_ROUTER_CONF_WEBSERVER
#define UIP_CONF_TCP 1
#endif

/* for launchpad */
#define CCXXWARE_CONF_ROM_BOOTLOADER_ENABLE 1
#define IEEE802154_CONF_PANID   0xABCD
#define IEEE802154_CONF_DEFAULT_CHANNEL 25
#define RF_BLE_CONF_ENABLED 1
/* --- */


#endif /* PROJECT_CONF_H_ */