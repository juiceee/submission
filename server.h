#pragma once
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <cstring>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

#define _POSIX_C_SOURCE  200809L
#define ADDRESS "224.10.50.50"
#define PORT 5000
using namespace std;

struct ClockSyncMessage{
    uint32_t clock_id  = 0;
    uint64_t server_ts = 0;
    uint64_t client_ts = 0;
    uint16_t check_sum = 0;
    uint64_t final_ts = 0;
    double offset_us = 0.0;

    void setOffset_us(){
        offset_us =  (final_ts + server_ts) / 2.0 - client_ts;
        return;
    }

    void setCheckSum(){
        uint32_t clockID = clock_id;
        uint64_t serverTS = server_ts;
        uint64_t clientTS = client_ts;
        int unit = (1<<8)-1;
        while( clockID || serverTS || clientTS){

            check_sum += ( clockID & unit );
            check_sum += ( serverTS & unit );
            check_sum += ( clientTS & unit );
            clockID = clockID >> 8;
            serverTS = serverTS >> 8;
            clientTS = clientTS >> 8;
    
        }
        return;

    }

};


uint64_t get_current_time_ts()
{
    time_t          s;  // Seconds
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    s  = spec.tv_sec;
    uint64_t ms =  s *1000000 + round(spec.tv_nsec / 1e3); // Convert nanoseconds to milliseconds
    return ms;
}


