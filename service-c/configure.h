#ifndef _CONFIGURE_H
#define _CONFIGURE_H

typedef struct _configure
{
    char service_address[32];
    int service_port;

    int netlogic_service_port;
    int game01_service_port;
    int game02_service_port;
    int game03_service_port;
    int game04_service_port;
    int netlogic_listen_port;   
}configure;

const char* conf_file = "../config/config"

#endif

