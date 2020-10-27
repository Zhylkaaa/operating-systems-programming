//
// Created by @Zhylkaaa on 27/10/2020.
//

#ifndef SYSTEMPROGRAMMING_WRITERS_READERS_TURNS_H
#define SYSTEMPROGRAMMING_WRITERS_READERS_TURNS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <zconf.h>

#define CHECK_ERROR(ret_code, action) {if(ret_code == -1){fprintf(stderr, "Error occured during %s.\n", action);fflush(stderr);exit (-1);}}

#endif //SYSTEMPROGRAMMING_WRITERS_READERS_TURNS_H
