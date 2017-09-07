#ifndef _CONFIGURE_H
#define _CONFIGURE_H

typedef struct _configure
{
    char service_address[32];
    int service_port;
    int service_route_port;

}configure;

extern const char* conf_file;
configure* configure_load(const char* config_path);

#endif

