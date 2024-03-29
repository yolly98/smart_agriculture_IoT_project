#include "resource.h"

static void tmp_get_handler(
  coap_message_t *request,
  coap_message_t *response,
  uint8_t *buffer,
  uint16_t preferred_size,
  int32_t *offset
);

static void tmp_put_handler(
  coap_message_t *request,
  coap_message_t *response,
  uint8_t *buffer,
  uint16_t preferred_size,
  int32_t *offset
);

static void tmp_event_handler(void);

/*--------------------------------------------*/

static struct tmp_str{
  int soil_temperature;
  unsigned int tmp_timer;
  struct etimer tmp_etimer;
  int state;
}tmp_mem;

EVENT_RESOURCE(
    tmp_rsc,
    "title=\"Soil Temperature\"; rt = \"Text\"",
    tmp_get_handler,
    NULL,
    tmp_put_handler,
    NULL,
    tmp_event_handler
);

/*--------------------------------------------*/

void save_tmp_timer(int timer){
  tmp_mem.tmp_timer = timer;
  tmp_mem.state = STATE_CONFIGURED;
}

void tmp_error(){
  tmp_mem.state = STATE_ERROR;
}

int get_tmp_timer(){
  return tmp_mem.tmp_timer; 
}

void set_tmp_timer(){
  etimer_set(&tmp_mem.tmp_etimer, tmp_mem.tmp_timer * CLOCK_MINUTE);
}

void reset_tmp_timer(int new_interval){
  int old_interval = - (tmp_mem.tmp_timer * CLOCK_MINUTE);
  tmp_mem.tmp_timer = new_interval;
  etimer_adjust(&tmp_mem.tmp_etimer, old_interval);
}

void restart_tmp_timer(){
  etimer_reset_with_new_interval(&tmp_mem.tmp_etimer, tmp_mem.tmp_timer * CLOCK_MINUTE);
}

bool check_tmp_timer_expired(){
  return etimer_expired(&tmp_mem.tmp_etimer);
}

/*----------------------------------*/

void send_soil_tmp(char msg[]){

  int tmp = (5 + random_rand()%35);
  tmp_mem.soil_temperature =  tmp;
  if(LOG_ENABLED) printf("[+] soil temperature detected: %d\n", tmp);

  sprintf(msg,"{\"cmd\":\"%s\",\"value\":%d}",
      "tmp",
      tmp_mem.soil_temperature
      );
  if(LOG_ENABLED) printf(" >  %s\n", msg);
}

/*------------------------------------------*/

void send_tmp_status(char msg[]){
  sprintf(msg,"{\"cmd\":\"%s\",\"timer\":%d}",
      "tmp-status",
      tmp_mem.tmp_timer
      );
  if(LOG_ENABLED) printf(" >  %s\n", msg);
}

/*------------------------------------------*/

static void tmp_event_handler(void)
{
  coap_notify_observers(&tmp_rsc);
}

/*----------------------------------------------*/


static void tmp_get_handler(
  coap_message_t *request,
  coap_message_t *response,
  uint8_t *buffer,
  uint16_t preferred_size,
  int32_t *offset
  ){

  if(tmp_mem.state == STATE_ERROR)
      return;

  char reply[MSG_SIZE];

  if(LOG_ENABLED) printf(" <  get sensor/tmp\n");
  send_soil_tmp(reply);
  
  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_payload(response, buffer, snprintf((char *)buffer, preferred_size, "%s", reply));
}

/*-------------------------------------------*/

static void tmp_put_handler(
  coap_message_t *request,
  coap_message_t *response,
  uint8_t *buffer,
  uint16_t preferred_size,
  int32_t *offset
  ){

    if(tmp_mem.state == STATE_ERROR)
      return;

    if(LOG_ENABLED) printf(" <  put sensor/tmp\n");
    const uint8_t* arg;
    char msg[MSG_SIZE];
    char reply[MSG_SIZE];
    int len = coap_get_payload(request, &arg);
    if (len <= 0){
      if(LOG_ENABLED) printf("[-] no argument obteined from put request of tmp_rsc\n");
      return;
    }
    
    sprintf(msg, "%s", (char*)arg);
    
    if(strcmp(msg, "status") == 0){
      if(LOG_ENABLED) printf(" <  get sensor/tmp-status\n");
      send_tmp_status(reply);
      coap_remove_observer_by_uri(NULL, tmp_rsc.url); //remove observer
    } 
    else{
      reset_tmp_timer(atoi(msg));
      send_tmp_status(reply);
    }
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, buffer, snprintf((char *)buffer, preferred_size, "%s", reply));
}