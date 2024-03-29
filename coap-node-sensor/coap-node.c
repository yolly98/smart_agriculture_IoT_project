#include "coap-node.h"

/*------------------------UTILITY------------------------------------------*/


bool isNumber(char * text){

    int len = strlen(text);
    int i;
    for(i = 0; i < len; i++){
        if(text[i] > 47 && text[i] < 58)
            continue;
        else 
            return false;
    }

    return true;
}

/*--------*/

void print_config(){

    unsigned int land_id = 0;
    unsigned int node_id = 0;
    bool enabled = false;
    unsigned int irr_limit = 0;
    unsigned int irr_duration = 0;
    bool status = false;
    unsigned int mst_timer = 0;
    unsigned int ph_timer = 0;
    unsigned int light_timer = 0;
    unsigned int tmp_timer = 0;

    get_config(&land_id, &node_id);
    get_irr_config(&enabled, &irr_limit, &irr_duration, &status);
    mst_timer = get_mst_timer();
    ph_timer = get_ph_timer();
    light_timer = get_light_timer();
    tmp_timer = get_tmp_timer();
    if(LOG_ENABLED) printf("[!] actual configuration: \n");
    if(LOG_ENABLED) printf("land_id: %d\n", land_id);
    if(LOG_ENABLED) printf("node_id: %d\n", node_id);
    if(LOG_ENABLED) printf("irr_config: { enabled: %s, irr_limit: %d, irr_duration: %d}\n",
        enabled?"true":"false",
        irr_limit,
        irr_duration);
    if(LOG_ENABLED) printf("mst_timer: %d\n", mst_timer);
    if(LOG_ENABLED) printf("ph_timer: %d\n", ph_timer);
    if(LOG_ENABLED) printf("light_timer: %d\n", light_timer);
    if(LOG_ENABLED) printf("tmp_timer: %d\n", tmp_timer);
    if(LOG_ENABLED) printf("-----------------------------\n");

}

/*--------*/

void parse_json(char json[], int n_arguments, char arguments[][100]){

    int value_parsed = 0;
    int len = 0;
    bool value_detected = false;
    int i;
    for(i = 0; json[i] != '\0' && value_parsed < n_arguments; i++){
        
        if(json[i] == ':'){
            i++; //there is the space after ':'
            value_detected = true;
            len = 0;
        }
        else if(value_detected && (json[i] == ',' || json[i] == ' ' || json[i] == '}')){
            value_detected = false;
            arguments[value_parsed][len] = '\0';
            value_parsed++;
        }
        else if(value_detected && json[i] == '{'){
            value_detected = false;
        }
        else if(value_detected){
            if(json[i] =='\'' || json[i] == '\"')
                continue;
            arguments[value_parsed][len] = json[i];
            len++;
        }

    }

    //for(int i = 0; i < n_arguments;i++)
    //    if(LOG_ENABLED) printf("[arg parsed #%d] %s \n", i, arguments[i]);        

}

/*------------------------------------------------*/

void send_config_request(char msg[]){

    unsigned int land_id = 0;
    unsigned int node_id = 0;
    get_config(&land_id, &node_id);
    sprintf(msg, "{\"cmd\":\"%s\",\"body\":{\"land_id\":%d,\"node_id\":%d}}",
        "config_rqst",
        land_id,
        node_id
        );
    if(LOG_ENABLED) printf(" >  %s \n", msg);
}


/*-----------------------------------------*/

void client_chunk_handler(coap_message_t *response){

    const uint8_t *chunk;

    if(response == NULL) {
        puts("Request timed out");
        return;
    }

    coap_get_payload(response, &chunk);
    char msg[MSG_SIZE];
    sprintf(msg,"%s",(char*)chunk);
    
    if(LOG_ENABLED) printf("[!] ASSIGN_CONFIG command elaboration ...\n");

    if(LOG_ENABLED) printf(" <  %s \n", msg);

    if(strcmp(msg, "server-ok") == 0){
        STATE = STATE_CONFIGURED;
        return;
    }
    else if(strcmp(msg, "error_land") == 0){
        if(LOG_ENABLED) printf("[-] land selected doesn't exist\n");
        STATE = STATE_ERROR;
        return;
    }
    else if(strcmp(msg, "error_id") == 0){
        if(LOG_ENABLED) printf("[-] node with the same id already exists\n");
        STATE = STATE_ERROR;
        return;
    }

    STATE = STATE_REGISTERED;
    int n_arguments = 8; 
    char arguments[n_arguments][100];
    parse_json(msg, n_arguments, arguments );

    bool enabled = (strcmp(arguments[1], "true") == 0)?true:false;
    int irr_limit = atoi(arguments[2]);
    int irr_duration = atoi(arguments[3]);
    int mst_timer = atoi(arguments[4]);
    int ph_timer = atoi(arguments[5]);
    int light_timer = atoi(arguments[6]);
    int tmp_timer = atoi(arguments[7]);

    save_irr_config(enabled, irr_limit, irr_duration, false);
    save_mst_timer(mst_timer);
    save_ph_timer(ph_timer);
    save_light_timer(light_timer);
    save_tmp_timer(tmp_timer);

    if(LOG_ENABLED) printf("[+] ASSIGN_CONFIG command elaborated with success\n");
}

/*-------------------------------------------------------*/

void assign_config_received_sim(){ //is a simulation of a message received from server

    char msg[MSG_SIZE];
    sprintf(msg, "%s", "{\"land_id\":4,\"node_id\":2,\"irr_config\":{\"enabled\":\"true\",\"irr_limit\":38,\"irr_duration\":20 },\"mst_timer\":720,\"ph_timer\":720,\"light_timer\":60,\"tmp_timer\":60 }");
    if(LOG_ENABLED) printf("[!] ASSIGN_CONFIG command elaboration ...\n");
    STATE = STATE_CONFIGURED;
    int n_arguments = 9; 
    char arguments[n_arguments][100];
    parse_json(msg, n_arguments, arguments );

    bool enabled = (strcmp(arguments[2], "true") == 0)?true:false;
    int irr_limit = atoi(arguments[3]);
    int irr_duration = atoi(arguments[4]);
    int mst_timer = atoi(arguments[5]);
    int ph_timer = atoi(arguments[6]);
    int light_timer = atoi(arguments[7]);
    int tmp_timer = atoi(arguments[8]);

    save_irr_config(enabled, irr_limit, irr_duration, false);
    save_mst_timer(mst_timer);
    save_ph_timer(ph_timer);
    save_light_timer(light_timer);
    save_tmp_timer(tmp_timer);

    if(LOG_ENABLED) printf("[+] ASSIGN_CONFIG command elaborated with success\n");
}

/*------------------------------------------------------------*/

PROCESS_THREAD(coap_node, ev, data){
    
    static unsigned int btn_count;
    static bool led_status;
    static bool land_id_setted;
    static int land_id;
    static int node_id;

    PROCESS_BEGIN();

    /* for launchpad   */
    //cc26xx_uart_set_input(serial_line_input_byte);
    //serial_line_init();
    /* ----------------*/

    /*------------INITIALIZATION---------------*/
    printf("[!] initialization COAP node...\n");
    if(LOG_ENABLED)
        printf("log mode enabled\n");
    else
        printf("log mode disabled\n");

    //set land_id and node_id

    if(LOG_ENABLED) printf("[!] manual land_id setting\n");

    etimer_set(&led_etimer,0.5 * CLOCK_SECOND);
    btn_count = 0;
    led_status = false;
    leds_single_off(LEDS_RED);
    land_id_setted = false;
    land_id = 0;
    node_id = 0;

    //default configuration
    is_alive_init();
    save_irr_config(true, 0, 60, false);
    save_mst_timer(60);
    save_ph_timer(60);
    save_light_timer(60);
    save_tmp_timer(60);

    while(1){
        PROCESS_YIELD();

        if(ev == PROCESS_EVENT_TIMER){
            if(etimer_expired(&led_etimer)){
                led_status = !led_status;
                leds_single_toggle(LEDS_RED);
                etimer_restart(&led_etimer);
            }

            if(btn_count > 0 && etimer_expired(&btn_etimer)){
                if(!land_id_setted){ 
                    land_id = btn_count;
                    land_id_setted = true;
                    leds_single_on(LEDS_RED);
                    led_status = true;
                    etimer_reset_with_new_interval(&led_etimer, 2 * CLOCK_SECOND);
                    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led_etimer));
                    etimer_reset_with_new_interval(&led_etimer, 0.5 * CLOCK_SECOND);
                    if(LOG_ENABLED) printf("[!] manual node_id setting\n");
                    btn_count = 0;
                }
                else{
                    node_id = btn_count;
                    break;
                }
            }
        }
        if(ev == serial_line_event_message){
            char * msg = (char*)data;
            if(LOG_ENABLED) printf("[!] recevived '%s' by serial\n", msg);
            if(isNumber(msg) && atoi(msg) > 0){
               if(!land_id_setted){
                    land_id = atoi(msg);
                    land_id_setted = true;
                    leds_single_on(LEDS_RED);
                    led_status = true;
                    etimer_reset_with_new_interval(&led_etimer, 2 * CLOCK_SECOND);
                    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led_etimer));
                    etimer_reset_with_new_interval(&led_etimer, 0.5 * CLOCK_SECOND);
                    if(LOG_ENABLED) printf("[!] manual node_id setting\n");
                    btn_count = 0;
                }
                else{
                    node_id = atoi(msg);
                    break;
                }
            }
            else    
                if(LOG_ENABLED) printf("[-] is not a number\n");
        }
        if(ev == button_hal_press_event){
            if(LOG_ENABLED) printf("[!] button pressed\n");
            btn_count++;
            if(btn_count == 1 && !land_id_setted)
                etimer_set(&btn_etimer, 3 * CLOCK_SECOND);
            else    
                etimer_restart(&btn_etimer);
        }
    }

    leds_single_off(LEDS_RED);
    led_status = false;

    if(LOG_ENABLED) printf("[+] land %d selected \n", land_id);
    if(LOG_ENABLED) printf("[+] id %d selected \n", node_id);

    save_config(land_id, node_id); 
    
    if(LOG_ENABLED) printf("[!] intialization ended\n");
    coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &coap_module.server_ep);
    coap_activate_resource(&config_rsc, "configuration");
    coap_activate_resource(&irr_rsc, "irrigation");
    coap_activate_resource(&is_alive_rsc, "is_alive");
    coap_activate_resource(&mst_rsc, "sensor/mst");
    coap_activate_resource(&ph_rsc, "sensor/ph");
    coap_activate_resource(&light_rsc, "sensor/light");
    coap_activate_resource(&tmp_rsc, "sensor/tmp");
    STATE = STATE_INITIALIZED;
    /*---------------CONFIGURATION-------------------*/
    
    if(LOG_ENABLED) printf("[!] configuration ... \n");

    char msg[MSG_SIZE];
    sprintf(msg, "{\"land_id\":%d,\"node_id\":%d}", land_id, node_id);
    
    coap_init_message(coap_module.request, COAP_TYPE_CON, COAP_GET, 0);
    coap_set_header_uri_path(coap_module.request, "/new_config");
    coap_set_payload(coap_module.request, (uint8_t *)msg, strlen(msg));
    while(true){
        COAP_BLOCKING_REQUEST(&coap_module.server_ep, coap_module.request, client_chunk_handler);
        if(STATE == STATE_REGISTERED)
            break;
        else if(STATE == STATE_ERROR){
            if(LOG_ENABLED) printf("[-] configuration failed\n");
            is_alive_error();
            config_error();
            irr_error();
            mst_error();
            ph_error();
            light_error();
            tmp_error();
            led_status = !led_status;
            leds_single_toggle(LEDS_RED);
            etimer_restart(&led_etimer);
            break;
        }
    }

    if(STATE != STATE_ERROR){

        if(LOG_ENABLED) printf("[!] observing phase ...\n");
        sprintf(msg, "{\"land_id\":%d,\"node_id\":%d}", land_id, node_id);
        coap_init_message(coap_module.request, COAP_TYPE_CON, COAP_GET, 0);
        coap_set_header_uri_path(coap_module.request, "/new_config");
        coap_set_payload(coap_module.request, (uint8_t *)msg, strlen(msg));
        while(true){
            COAP_BLOCKING_REQUEST(&coap_module.server_ep, coap_module.request, client_chunk_handler);
            if(STATE == STATE_CONFIGURED)
                break;
        }
        //assign_config_received_sim(); //simulation (use this and comment the while above to jump the configuration phase)
        print_config();

        set_mst_timer();
        set_ph_timer();
        set_light_timer();
        set_tmp_timer();

        if(LOG_ENABLED) printf("[!] node  in online\n");
    }

    while(true){

        PROCESS_YIELD();
        
        if (STATE == STATE_ERROR){
            if(etimer_expired(&led_etimer)){
                led_status = !led_status;
                leds_single_toggle(LEDS_RED);
                etimer_restart(&led_etimer);
            }
            continue;
        }

        if(ev == serial_line_event_message){ //to test if the node can comunicate
            char * msg = (char*)data;
            if(strcmp(msg, "help") == 0){
                if(LOG_ENABLED) printf("---- command list ----\n");
                if(LOG_ENABLED) printf(". config\n");
                if(LOG_ENABLED) printf(". trigger\n");
                if(LOG_ENABLED) printf("----------------------\n");
            }
            else if(strcmp(msg, "config") == 0){
                char to_send[MSG_SIZE];
                sprintf(to_send, "{\"land_id\":%d,\"node_id\":%d}", land_id, node_id);
                coap_init_message(coap_module.request, COAP_TYPE_CON, COAP_GET, 0);
                coap_set_header_uri_path(coap_module.request, "/new_config");
                coap_set_payload(coap_module.request, (uint8_t *)to_send, strlen(to_send));
                COAP_BLOCKING_REQUEST(&coap_module.server_ep, coap_module.request, client_chunk_handler);   
            }
            else if(strcmp(msg, "trigger") == 0)
                ph_rsc.trigger();
        }

        if(check_mst_timer_expired()){
            mst_rsc.trigger();
            int moisture = get_mst_value(); 
            irr_starting(moisture);
            restart_mst_timer();
        }

        if(check_ph_timer_expired()){
            ph_rsc.trigger();
            restart_ph_timer();
        }

        if(check_light_timer_expired()){
            light_rsc.trigger();
            restart_light_timer();
        }

        if(check_tmp_timer_expired()){
            tmp_rsc.trigger();
            restart_tmp_timer();
        }

        if(check_irr_timer_expired())
            irr_stopping();
    }

    PROCESS_END();
}
