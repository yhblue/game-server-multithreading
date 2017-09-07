#include "configure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#define MAX_BUF_LEN 1024
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 64

const char* service_address = "service_address";
const char* service_port = "service_port";
const char* service_route_port = "service_route_port";

const char* conf_file = "../config/config";

#define STRING_EQUAL  0

int configure_transform(configure* conf,char* key,char* val)
{
    if(conf == NULL)
        return -1;

    if(strcmp(key,service_address) == STRING_EQUAL)
    {
        strcpy(conf->service_address,val);
        return 0;
    }
    if(strcmp(key,service_port) == STRING_EQUAL)
    {
        conf->service_port = atoi(val);
        return 0;
    }
    if(strcmp(key,service_route_port) == STRING_EQUAL)
    {
        conf->service_route_port = atoi(val);
        return 0;
    }
    return -1;
}

//裁剪
int Trim(char* s)
{
    int n;
    for(n = strlen(s) - 1; n >= 0; n--)
    {
        if(s[n]!=' ' && s[n]!='\t' && s[n]!='\n')
            break;
        s[n+1] = '\0';
    }
    return n;
}


configure* configure_load(const char* config_path)
{
    FILE * file = fopen(config_path, "r");
    if (file == NULL)
    {
        printf("open %s failed.\n", config_path);
        return NULL;
    }

    configure* conf = (configure*)malloc(sizeof(configure));
    if(conf == NULL)
        return NULL;

    char buf[MAX_BUF_LEN];

    while(fgets(buf, MAX_BUF_LEN, file) != NULL) 
    {
        Trim(buf);

        int text_comment = 0;
        if (buf[0] != '#' && (buf[0] != '/' || buf[1] != '/'))
        {
            if (strstr(buf, "/*") != NULL)
            {
                text_comment = 1;
                continue;
            }
            else if (strstr(buf, "*/") != NULL)
            {
                text_comment = 0;
                continue;
            }
        }
        if (text_comment == 1)
        {
            continue;
        }

        int buf_len = strlen(buf);
        if (buf_len <= 1 || buf[0] == '#' || buf[0] == '=' || buf[0] == '/')
        {
            continue;
        }
        buf[buf_len-1] = '\0';

        char key[MAX_KEY_LEN] = {0};
        char val[MAX_VAL_LEN] = {0};

        int kv=0, klen=0, vlen=0;
        int i = 0;
        for (i=0; i<buf_len; ++i)
        {
            if (buf[i] == ' ')
                continue;

            if (kv == 0 && buf[i] != '=')
            {
                if (klen >= MAX_KEY_LEN)
                    break;
                key[klen++] = buf[i];
                continue;
            }
            else if (buf[i] == '=')
            {
                kv = 1;
                continue;
            }
            if (vlen >= MAX_VAL_LEN || buf[i] == '#')
                break;
            val[vlen++] = buf[i];
        }
        //printf("%s=%s\n", key, val);
        configure_transform(conf,key,val);

    }
    printf("\n\n\nservice_address = %s\n",conf->service_address);
    printf("service_port= %d\n",conf->service_port);
    printf("service_route_port= %d\n",conf->service_route_port);

    return conf;
}



#if 0 
int main()
{
    configure* conf = configure_load("./test.conf");
    if(conf == NULL)
        return -1;
    else
    {
        printf("\n\n\n");
        printf("service_address = %s\n",conf->service_address);
        printf("service_port = %d\n",conf->service_port);
        printf("netlogic_service_port= %d\n",conf->netlogic_service_port);
        printf("game01_service_port = %d\n",conf->game01_service_port);
        printf("game02_service_port = %d\n",conf->game02_service_port);
        printf("game03_service_port = %d\n",conf->game03_service_port);
        printf("game04_service_port = %d\n",conf->game04_service_port);
        printf("netlogic_listen_port = %d\n",conf->netlogic_listen_port);
    }
    return 0;
}

// const char* netlogic_service_port = "netlogic_service_port";
// const char* game01_service_port = "game01_service_port";
// const char* game02_service_port = "game02_service_port";
// const char* game03_service_port = "game03_service_port";
// const char* game04_service_port = "game04_service_port";
// const char* netlogic_listen_port = "netlogic_listen_port";

    if(strcmp(key,game01_service_port) == STRING_EQUAL)
    {
        conf->game01_service_port = atoi(val);
        return 0;
    }
    if(strcmp(key,game02_service_port) == STRING_EQUAL)
    {
        conf->game02_service_port = atoi(val);
        return 0;
    }
    if(strcmp(key,game03_service_port) == STRING_EQUAL)
    {
        conf->game03_service_port = atoi(val);
        return 0;
    }
    if(strcmp(key,game04_service_port) == STRING_EQUAL)
    {
        conf->game04_service_port = atoi(val);
        return 0;
    }   
    if(strcmp(key,netlogic_listen_port) == STRING_EQUAL)
    {
        conf->netlogic_listen_port = atoi(val);
        return 0;
    } 

#endif
