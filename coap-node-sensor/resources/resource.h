#ifndef RESOURCE_H_
#define RESOURCE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coap-engine.h"
#include "random.h"

#define CLOCK_MINUTE        CLOCK_SECOND * 60
#define MSG_SIZE            200
#define STATE_CONFIGURED    0
#define STATE_ERROR         1

void parse_json(char json[], int n_arguments, char arguments[][100]);
bool isNumber(char * text);

#endif