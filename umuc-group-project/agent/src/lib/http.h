#ifndef _HTTP_H
#define _HTTP_H

#include <netinet/in.h>
#include <openssl/ssl.h>

#define MANAGEMENT_CONSOLE_IP "10.1.0.10"
#define MANAGEMENT_CONSOLE_PORT    "5000"

extern char management_console_ip[19];
extern char management_console_port[7];

/*
// EXPORTED ENUMS
// Types currently supported by this library.
*/

// Enum type for HTTP versions.
typedef enum _HTTP_VERSION        {HTTP_VERSION_1_0} HTTP_VERSION;
// Enum type for request header Method types.
typedef enum _HTTP_REQUEST_METHOD {HTTP_REQUEST_METHOD_GET, HTTP_REQUEST_METHOD_POST} HTTP_REQUEST_METHOD;
// Enum type for request header Accept types.
typedef enum _HTTP_HEADER_ACCEPT  {HTTP_HEADER_ACCEPT_ANY} HTTP_HEADER_ACCEPT;
// Enum type for request header Content-Type.
typedef enum _HTTP_HEADER_CONTENT_TYPE  {HTTP_HEADER_CONTENT_TYPE_JSON} HTTP_HEADER_CONTENT_TYPE;
// Enum type for response codes.
typedef enum _HTTP_RESPONSE_CODE  {HTTP_RESPONSE_OK, HTTP_RESPONSE_BAD_REQUEST} HTTP_RESPONSE_CODE;

// Buffer size for request and response buffers
#define BUFFER_MAX_BYTES 16384

/*
// EXPORTED STRUCTS
*/

// Defines an HTTP parameter
typedef struct _HTTP_PARAM
{
    char            *Name, *Value;
    size_t           NameLen, ValueLen;

} HTTP_PARAM, *PHTTP_PARAM;

// Defines an HTTP request
typedef struct _HTTP_REQUEST
{
    char            	  *Method, *Uri, *Version, *Host, *ContentType;
    size_t          	  MethodLen, UriLen, VersionLen, HostLen, ContentTypeLen;
    HTTP_PARAM      	  Params[10];
    int             	  ParamCount;
    uint8_t         	  HostSet;
    ssize_t         	  RequestLen;
    char	    	      *RemotePort;
    size_t	    	      RemotePortLen;
    struct sockaddr_in  Addrin;
    const char          *Content;
    size_t              ContentLen;
    char            	  Buffer[BUFFER_MAX_BYTES];

} HTTP_REQUEST, *PHTTP_REQUEST;

// Defines an HTTP response
typedef struct _HTTP_RESPONSE
{
    char            *Version, *Code;
    size_t          VersionLen, CodeLen;
    ssize_t         ResponseLen;
    char            Buffer[BUFFER_MAX_BYTES];

} HTTP_RESPONSE, *PHTTP_RESPONSE;

// Defines the Client context
typedef struct _HTTP_CLIENT_CTX
{
    HTTP_REQUEST        Request;
    HTTP_RESPONSE       Response;

} HTTP_CLIENT_CTX, *PHTTP_CLIENT_CTX;

// Defines the Server context
typedef struct _HTTP_SERVER_CTX
{
    HTTP_REQUEST        Request;
    HTTP_RESPONSE       Response;
    int                 ClientSocket;

} HTTP_SERVER_CTX, *PHTTP_SERVER_CTX;

typedef struct _HTTPS_CLIENT_CTX
{
    HTTP_REQUEST	Request;
    HTTP_RESPONSE	Response;
    SSL_CTX		    *sslctx;
    SSL			      *ssl;

} HTTPS_CLIENT_CTX, *PHTTPS_CLIENT_CTX;

/*
// EXPORTED FUNCTIONS
*/

// Client Functions

PHTTP_CLIENT_CTX	      HTTP_Init_Client_CTX			              ();
int                     HTTP_Client_Set_Request_Hostname        (PHTTP_REQUEST request, char *host);
int                     HTTP_Client_Set_Request_Address         (PHTTP_REQUEST request, char *address);
int                     HTTP_Client_Set_Request_Port            (PHTTP_REQUEST request, char *port);
int                     HTTP_Client_Set_Request_Method          (PHTTP_REQUEST request, HTTP_REQUEST_METHOD method);
int                     HTTP_Client_Set_Request_Uri             (PHTTP_REQUEST request, char *Uri);
int                     HTTP_Client_Set_Request_Params          (PHTTP_REQUEST request, HTTP_PARAM params[], int ParamCount);
int                     HTTP_Client_Set_Request_Content_Type    (PHTTP_REQUEST request, HTTP_HEADER_CONTENT_TYPE type);
int                     HTTP_Client_Set_Request_Content         (PHTTP_REQUEST request, const char *content, size_t length);
int                     HTTP_Client_Send_Request                (PHTTP_CLIENT_CTX pctx);
void                    HTTP_Free_Client_CTX                    (PHTTP_CLIENT_CTX pctx);

// Server Functions

PHTTP_SERVER_CTX        HTTP_Init_Server_CTX                    (unsigned int port);
int                     HTTP_Server_Send_Response               (PHTTP_SERVER_CTX pctx);
int                     HTTP_Server_Set_Response_Code   	      (PHTTP_SERVER_CTX pctx, HTTP_RESPONSE_CODE code);
int                     HTTP_Server_Wait_For_Request    	      (PHTTP_SERVER_CTX pctx);
void                    HTTP_Free_Server_CTX                    (PHTTP_SERVER_CTX pctx);

// HTTPS Functions

PHTTPS_CLIENT_CTX       HTTPS_Init_Client_CTX();
int                     HTTPS_Client_Send_Request (PHTTPS_CLIENT_CTX pctx);
void                    HTTPS_Free_Client_CTX     (PHTTPS_CLIENT_CTX pctx);

#endif
