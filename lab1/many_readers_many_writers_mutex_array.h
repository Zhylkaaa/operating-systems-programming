//
// Created by @Zhylkaaa on 26/10/2020.
//

#ifndef SYSTEMPROGRAMMING_MANY_READERS_MANY_WRITERS_MUTEX_ARRAY_H
#define SYSTEMPROGRAMMING_MANY_READERS_MANY_WRITERS_MUTEX_ARRAY_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <zconf.h>

#define CHECK_ERROR(ret_code, action) {if(ret_code == -1){fprintf(stderr, "Error occured during %s.\n", action); exit (-1);}}

#endif //SYSTEMPROGRAMMING_MANY_READERS_MANY_WRITERS_MUTEX_ARRAY_H
