#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "coap-node.h"

static void ph_get_handler(
  coap_message_t *request,
  coap_message_t *response,
  uint8_t *buffer,
  uint16_t preferred_size,
  int32_t *offset
);

static void ph_put_handler(
  coap_message_t *request,
  coap_message_t *response,
  uint8_t *buffer,
  uint16_t preferred_size,
  int32_t *offset
);

static void ph_event_handler(void);

/*--------------------------------------------*/

static struct ph_str{
  unsigned int ph_raw;
  unsigned int ph_timer;
  struct etimer ph_etimer;
}ph_mem;

EVENT_RESOURCE(
    ph_rsc,
    "title=\"Ph\"; rt = \"Text\"",
    ph_get_handler,
    NULL,
    ph_put_handler,
    NULL,
    ph_event_handler
);

/*---------------------------------------*/

void send_ph_level(char msg[]){

    etimer_set(&ph_mem.ph_ctimer, ph_mem.timer * CLOCK_MINUTE);

    int ph_level = (5 + random_rand()%5);
    ph_mem.ph_level =  ph_level;
    printf("[+] ph level detected: %d\n", ph_level);

    sprintf(msg,"{ \"cmd\": \"%s\", \"value\": %d }",
        "ph", 
        ph_mem.ph_level
        );
    
    printf(" >  %s\n", msg);
}

/*------------------------------------------*/

void send_ph_status(char msg[]){
  sprintf(msg,"{ \"cmd\": \"%s\", \"timer\": %d }",
      "ph-status",
      ph_mem.ph_timer
      );
  printf(" >  %s\n", msg);
}

/*------------------------------------------*/

static void ph_event_handler(void)
{
  coap_notify_observers(&ph_rsc);
}

/*----------------------------------------------*/


static void ph_get_handler(
  coap_message_t *request,
  coap_message_t *response,
  uint8_t *buffer,
  uint16_t preferred_size,
  int32_t *offset
  ){

  const char* value;
  char msg[MSG_SIZE];
  char reply[MSG_SIZE];

  int len = coap_get_query_variable(request, "value", value);
  sprintf(msg, "%s", (char*)value);
  if(len == 0)
    send_ph_level(reply);
  else if(len > 0 and strcmp(value, "status") == 0)
    send_ph_status(reply)
  else{
    printf("[-] error unknown in get ph sensor");
    return;
  } 

  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_payload(response, buffer, snprintf((char *)buffer, preferred_size, "%s", reply));

}

/*-------------------------------------------------*/

static void ph_put_handler(
  coap_message_t *request,
  coap_message_t *response,
  uint8_t *buffer,
  uint16_t preferred_size,
  int32_t *offset
  ){

  const char* arg;
  char msg[MSG_SIZE];
  char reply[MSG_SIZE];
  int len = coap_get_post_variable(request, "value", &arg);
  if (len <= 0){
    printf("[-] no argument obteined from put request of ph_rsc");
    return;
  }
  sprintf(msg, "%s", (char*)arg);
  
  ph_mem.ph_timer = atoi(msg);
  //ctimer_set(&node_timers.ph_ctimer, node_memory.configuration.ph_timer * CLOCK_MINUTE, get_ph_level, NULL);
  etimer_set(&ph_mem.ph_ctimer, ph_mem.timer * CLOCK_MINUTE);
  send_ph_status(reply); 
  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_payload(response, buffer, snprintf((char *)buffer, preferred_size, "%s", reply));
}
