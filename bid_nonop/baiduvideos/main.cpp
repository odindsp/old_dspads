#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "baiduvideos_response.h"
#include "../../common/errorcode.h"
#include "../../common/bid_filter.h"
#include "../../common/confoperation.h"
using namespace std;

#define PRIVATE_CONF		"dsp_baiduvideos.conf"

uint8_t cpu_count = 0;
pthread_t *id = NULL;
bool fullreqrecord = false;

static void *doit(void *arg)
{
    int errcode = 0;
    uint8_t index = (uint64_t)arg;
    FCGX_Request request;
    pthread_t threadlog = 0;
    string senddata = "";
    int rc = 0;

    pthread_detach(pthread_self());

    RECVDATA *recvdata = (RECVDATA *)calloc(1, sizeof(RECVDATA));
    if (recvdata == NULL)
    {
        goto exit;
    }

    recvdata->length = 0;
    recvdata->buffer_length = MAX_REQUEST_LENGTH;
    recvdata->data = (char *)malloc(recvdata->buffer_length);
    if (recvdata->data == NULL)
    {
        goto exit;
    }

    FCGX_InitRequest(&request, 0, 0);

    while (1)
    {
        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
        static pthread_mutex_t counts_mutex = PTHREAD_MUTEX_INITIALIZER;

        char *remoteaddr = NULL;
        char *method = NULL;
        char *contenttype = NULL;
        uint32_t contentlength = 0;

        /* Some platforms require accept() serialization, some don't.. */
        pthread_mutex_lock(&accept_mutex);
        rc = FCGX_Accept_r(&request);
        pthread_mutex_unlock(&accept_mutex);
        if (rc < 0)
        {
            break;
        }


        remoteaddr = FCGX_GetParam("REMOTE_ADDR", request.envp);
        if (remoteaddr == NULL)
        {
            goto finish;
        }

        method = FCGX_GetParam("REQUEST_METHOD", request.envp);
        if (method == NULL)
        {
            goto finish;
        }

        if (strcmp(method, "POST") != 0)
        {
            goto finish;
        }

        contenttype = FCGX_GetParam("CONTENT_TYPE", request.envp);
        if (contenttype == NULL)
        {
            goto finish;
        }
		
        if (strcasecmp(contenttype, "application/json"))
        {
            goto finish;
        }

        contentlength = atoi(FCGX_GetParam("CONTENT_LENGTH", request.envp));
        if (contentlength == 0)
        {
            goto finish;
        }
        if (contentlength >= recvdata->buffer_length)
        {
            recvdata->buffer_length = contentlength + 1;
            recvdata->data = (char *)realloc(recvdata->data,recvdata->buffer_length);

            if (recvdata->data == NULL)
            {
                recvdata->buffer_length = 0;
                goto finish;
            }
        }

        recvdata->length = contentlength;

        if (FCGX_GetStr(recvdata->data, recvdata->length, request.in) == 0)
        {
			free(recvdata->data);
			free(recvdata);
            goto finish;
        }
        recvdata->data[recvdata->length] = 0;
        errcode = get_bid_response(recvdata, senddata);
		
		if(errcode ==  E_SUCCESS)
		{
			pthread_mutex_lock(&counts_mutex);
			FCGX_PutStr(senddata.data(), senddata.size(), request.out);
			pthread_mutex_unlock(&counts_mutex);
		}
finish:
        FCGX_Finish_r(&request);
    }

exit:
    if (recvdata->data != NULL)
        free(recvdata->data);
    if (recvdata != NULL)
        free(recvdata);

}

int main(int argc, char *argv[])
{
    int errcode = 0;
    bool is_textdata = false;

    string str_global_conf = string(GLOBAL_PATH) + string(GLOBAL_CONF_FILE);
    char *global_conf = (char *)str_global_conf.c_str();


    cpu_count = GetPrivateProfileInt(global_conf, "default", "cpu_count");
    if (cpu_count < 1)
    {
		return 1;
    }

    id = (pthread_t *)calloc(cpu_count, sizeof(pthread_t));

    FCGX_Init();
	
    id[0] = pthread_self();
    for (uint8_t i = 1; i < cpu_count; i++)
    {
        pthread_create(&id[i], NULL, doit, (void *)i);
    }
	
    doit(0);
exit:

    if (id)
    {
        free(id);
        id = NULL;
    }
	
    return 0;
}
