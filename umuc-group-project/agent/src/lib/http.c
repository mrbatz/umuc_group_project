#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "http.h"
#include "logger.h"

char management_console_ip[19] = { '\0' };
char management_console_port[7] = { '\0' };

#define HTTP_DEFAULT_URI "/"
#define HTTP_DEFAULT_USER_AGENT "Service-Beacon-Agent/1.0"

// Static string arrays

static char * const _HTTP_VERSION_ARRAY[] = { "HTTP/1.0" };
static char * const _HTTP_REQUEST_METHOD_ARRAY[] = { "GET" , "POST" };
//static char * const _HTTP_HEADER_ACCEPT_ARRAY[]	= { "*/*" };
static char * const _HTTP_RESPONSE_CODE_ARRAY[]  = { "200 OK", "400 Bad Request" };
static char * const _HTTP_HEADER_CONTENT_TYPE_ARRAY[]  = { "application/json" };

// Static functions

static int HTTP_Parse_Request 				               (PHTTP_REQUEST request);
static int HTTP_Parse_Response 				               (PHTTP_RESPONSE response);
static int HTTP_Parse_Request_Method_Parameters_GET  (PHTTP_REQUEST request);
static int HTTP_Fill_Buffer_Request 			           (PHTTP_REQUEST request);
static int HTTP_Fill_Buffer_Response 			           (PHTTP_SERVER_CTX pctx);
static int HTTP_Fill_Buffer_Params_GET 			         (PHTTP_REQUEST request, char **current);
static int HTTP_Name_Lookup 				                 (PHTTP_REQUEST request);
static int checkPointer 				                     (void *ptr);

// Initializes the Client context structure
PHTTP_CLIENT_CTX HTTP_Init_Client_CTX()
{
    PHTTP_CLIENT_CTX pctx = (PHTTP_CLIENT_CTX)calloc(1, sizeof(*pctx));

    pctx->Request.Addrin.sin_family = AF_INET;
    pctx->Request.Method = _HTTP_REQUEST_METHOD_ARRAY[HTTP_REQUEST_METHOD_GET];
    pctx->Request.MethodLen = strlen(pctx->Request.Method);
    pctx->Request.Uri = HTTP_DEFAULT_URI;
    pctx->Request.UriLen = strlen(HTTP_DEFAULT_URI);
    pctx->Request.Version = _HTTP_VERSION_ARRAY[HTTP_VERSION_1_0];
    pctx->Request.VersionLen = strlen(pctx->Request.Version);

    return pctx;
}

// Initializes the HTTPS Client context structure
PHTTPS_CLIENT_CTX HTTPS_Init_Client_CTX()
{
    PHTTPS_CLIENT_CTX pctx = NULL;
    SSL_CTX *psslctx = NULL;
    SSL *pssl = NULL;
    EVP_PKEY *pkey = NULL;
    X509 *cert = NULL;
    X509 *inter = NULL;
    X509 *root = NULL;
    FILE *fp = NULL;

    if ((pctx = (PHTTPS_CLIENT_CTX)calloc(1, sizeof(*pctx))) == NULL)
    {
        log(ERR, "Not enough memory to allocate for HTTPS_CLIENT context.");
        return NULL;
    }

    // Create new context object that negotiates highest mutually accepted version (SSLv3, TLSv1, TLSv1.1, TLSv1.2, TLSv1.3)
    if ((psslctx = SSL_CTX_new(TLS_method())) == NULL)
    {
        log(ERR, "Error create SSL_CTX context structure.");
        return NULL;
    }

    // Set minimum allowed version to TLSv1 (No SSLv3)
    SSL_CTX_set_min_proto_version(psslctx, TLS1_VERSION);

    // Load private key
    if ((fp = fopen("/var/service-beacon/certs/agent.key.pem", "r")) == NULL)
    {
        log(ERR, "Error opening agent key file.");
        SSL_CTX_free(psslctx);
        return NULL;
    }

    if (!PEM_read_PrivateKey(fp, &pkey, 0, NULL))
    {
        log(ERR, "Error reading agent key file.");
        fclose(fp);
        SSL_CTX_free(psslctx);
        return NULL;
    }

    fclose(fp);

    // Load agent certificate
    if ((fp = fopen("/var/service-beacon/certs/agent.cert.pem", "r")) == NULL)
    {
        log(ERR, "Error opening agent cert file.");
        SSL_CTX_free(psslctx);
        return NULL;
    }

    if (!PEM_read_X509(fp, &cert, 0, NULL))
    {
        log(ERR, "Error reading agent cert file.");
        fclose(fp);
        SSL_CTX_free(psslctx);
        return NULL;
    }

    fclose(fp);

    // Load intermediate certificate
    if ((fp = fopen("/var/service-beacon/certs/intermediate.cert.pem", "r")) == NULL)
    {
        log(ERR, "Error opening intermediate cert file.");
        SSL_CTX_free(psslctx);
        return NULL;
    }

    if (!PEM_read_X509(fp, &inter, 0, NULL))
    {
        log(ERR, "Error reading intermediate cert file.");
        fclose(fp);
        SSL_CTX_free(psslctx);
        return NULL;
    }

    fclose(fp);

    // Load root certificate
    if ((fp = fopen("/var/service-beacon/certs/ca.cert.pem", "r")) == NULL)
    {
        log(ERR, "Error opening root ca cert file.");
        SSL_CTX_free(psslctx);
        return NULL;
    }

    if (!PEM_read_X509(fp, &root, 0, NULL))
    {
        log(ERR, "Error reading root ca cert file.");
        fclose(fp);
        SSL_CTX_free(psslctx);
        return NULL;
    }

    fclose(fp);

    // Load agent cert into sslctx
    if (!SSL_CTX_use_certificate(psslctx, cert))
    {
        log(ERR, "Error loading agent cert into context.");
        SSL_CTX_free(psslctx);
        return NULL;
    }

    // Set the private key to use
    if (!SSL_CTX_use_PrivateKey(psslctx, pkey))
    {
        log(ERR, "Error loading agent key into context.");
        SSL_CTX_free(psslctx);
        return NULL;
    }

    // Load extra chain certs into sslctx
    if (!SSL_CTX_add_extra_chain_cert(psslctx, inter))
    {
        log(ERR, "Error loading intermediate cert into context.");
        SSL_CTX_free(psslctx);
        return NULL;
    }

    if (!SSL_CTX_add_extra_chain_cert(psslctx, root))
    {
        log(ERR, "Error loading root ca cert into context.");
        SSL_CTX_free(psslctx);
        return NULL;
    }

    // Get certificate verification store
    X509_STORE *store = SSL_CTX_get_cert_store(psslctx);

    // Add intermediate and ca root certs to verify store
    X509_STORE_add_cert(store, inter);
    X509_STORE_add_cert(store, root);

    // Set agent cert as end entity certificate
    if (SSL_CTX_select_current_cert(psslctx, cert) == 0) { SSL_CTX_free(psslctx); return NULL; }

    // Check the private key and cert match
    if (!SSL_CTX_check_private_key(psslctx)) { SSL_CTX_free(psslctx); return NULL; }

    // Create a new SSL object
    if ((pssl = SSL_new(psslctx)) == NULL) { SSL_CTX_free(psslctx); return NULL; }

    // Set to work in client mode
    SSL_set_connect_state(pssl);

    // Set to verify server certificate
    SSL_set_verify(pssl, SSL_VERIFY_PEER, NULL);

    pctx->sslctx = psslctx;
    pctx->ssl = pssl;
    pctx->Request.Addrin.sin_family = AF_INET;
    pctx->Request.Method = _HTTP_REQUEST_METHOD_ARRAY[HTTP_REQUEST_METHOD_GET];
    pctx->Request.MethodLen = strlen(pctx->Request.Method);
    pctx->Request.Uri = HTTP_DEFAULT_URI;
    pctx->Request.UriLen = strlen(HTTP_DEFAULT_URI);
    pctx->Request.Version = _HTTP_VERSION_ARRAY[HTTP_VERSION_1_0];
    pctx->Request.VersionLen = strlen(pctx->Request.Version);

    return pctx;
}

// Initializes the Server context structure
PHTTP_SERVER_CTX HTTP_Init_Server_CTX(unsigned int port)
{
    if (port > 65535)
    {
        return NULL;
    }

    PHTTP_SERVER_CTX pctx = (PHTTP_SERVER_CTX)calloc(1, sizeof(*pctx));

    PHTTP_REQUEST request = &pctx->Request;
    PHTTP_RESPONSE response = &pctx->Response;

    // Prep sockaddr_in struct
    request->Addrin.sin_family = AF_INET;
    request->Addrin.sin_addr.s_addr = INADDR_ANY;
    request->Addrin.sin_port = htons(port);
    response->Version = _HTTP_VERSION_ARRAY[HTTP_VERSION_1_0];
    response->VersionLen = strlen(pctx->Response.Version);

    return pctx;
}

// Frees the Client context structure
void HTTP_Free_Client_CTX(PHTTP_CLIENT_CTX pctx)
{
    if (pctx == NULL) { return; }
    free(pctx);
}

// Frees the Server context structure
void HTTP_Free_Server_CTX(PHTTP_SERVER_CTX pctx)
{
    if (pctx == NULL) { return; }
    free(pctx);
}

void HTTPS_Free_Client_CTX(PHTTPS_CLIENT_CTX pctx)
{
    if (pctx == NULL) { return; }
    if (pctx->ssl != NULL) { SSL_free(pctx->ssl); }
    if (pctx->sslctx != NULL) { SSL_CTX_free(pctx->sslctx); }
    free(pctx);
}

// Sends an HTTP request
int HTTP_Client_Send_Request(PHTTP_CLIENT_CTX pctx)
{
    if (checkPointer(pctx)) { return -1; }

    PHTTP_REQUEST   request = &pctx->Request;
    PHTTP_RESPONSE  response = &pctx->Response;

    if (request->HostSet == 1)
    {
    	if (HTTP_Name_Lookup(request) < 0) { return -1; }
    }

    request->RequestLen = HTTP_Fill_Buffer_Request(request);

    ssize_t bytesSent, bytesReceived = 0;

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) { return -2; }

    // Connect to server
    if (connect(socketfd, (struct sockaddr *)&request->Addrin, sizeof(request->Addrin)) < 0)
    {
        return -3;
    }

    // Send request
    bytesSent = send(socketfd, request->Buffer, request->RequestLen, 0);
    if (bytesSent == 0)
    {
        return -4;
    }

    bytesReceived = recv(socketfd, response->Buffer, BUFFER_MAX_BYTES, 0);

    response->ResponseLen = bytesReceived;

    if (bytesReceived == 0) {  return -5; }

    if (HTTP_Parse_Response(response) < 0) { return -6; }

    close(socketfd);

    return 0;
}

// Sets the request hostname for resolution and Host header
int HTTP_Client_Set_Request_Hostname(PHTTP_REQUEST request, char *host)
{
    if (request == NULL) { return -1; }
    if (host == NULL) { return -1; }

    request->Host = host;
    request->HostLen = strlen(host);
    request->HostSet = 1;

    return 0;
}

// Sets the request IPv4 address to connect to
int HTTP_Client_Set_Request_Address(PHTTP_REQUEST request, char *address)
{
    if (request == NULL) { return -1; }
    if (address == NULL) { return -1; }

    request->Host = address;
    request->HostLen = strlen(address);

    if (inet_pton(AF_INET, address, &request->Addrin.sin_addr) < 1) { return -2; }

    request->HostSet = 0;

    return 0;
}

// Sets the request port to connect to
int HTTP_Client_Set_Request_Port(PHTTP_REQUEST request, char *port)
{
    if (request == NULL) { return -1; }

    int iport = atoi(port);
    request->Addrin.sin_port = htons((uint16_t)iport);
    request->RemotePort = port;
    request->RemotePortLen = strlen(port);

    return 0;
}

// Sets the request Method header
int HTTP_Client_Set_Request_Method(PHTTP_REQUEST request, HTTP_REQUEST_METHOD method)
{
    if (request == NULL) { return -1; }

    request->Method = _HTTP_REQUEST_METHOD_ARRAY[method];
    request->MethodLen = strlen(request->Method);

    return 0;
}

// Sets the request URI header
int HTTP_Client_Set_Request_Uri(PHTTP_REQUEST request, char *Uri)
{
    if (request == NULL) { return -1; }
    if (Uri == NULL) { return -1; }

    request->Uri = Uri;
    request->UriLen = strlen(Uri);

    return 0;
}

// Sets the request parameters given an array of parameter objects
int HTTP_Client_Set_Request_Params(PHTTP_REQUEST request, HTTP_PARAM params[], int ParamCount)
{
    if (request == NULL) { return -1; }
    if (params == NULL) { return -1; }

    if (ParamCount > 10) { return -3; }

    PHTTP_PARAM pparam = request->Params;

    for (int x = 0; x < ParamCount; x++)
    {
        pparam[x].Name = params[x].Name;
        pparam[x].NameLen = strlen(params[x].Name);
        pparam[x].Value = params[x].Value;
        pparam[x].ValueLen = strlen(params[x].Value);

        // Check for newlines at end of parameters as they mess up the HTTP Request
        if (*(pparam[x].Name + pparam[x].NameLen - 1) == '\n')
        {
            pparam[x].NameLen--;
        }

        if (*(pparam[x].Value + pparam[x].ValueLen - 1) == '\n')
        {
            pparam[x].ValueLen--;
        }
    }

    request->ParamCount = ParamCount;

    return 0;
}

// Sends an HTTP response
int HTTP_Server_Send_Response(PHTTP_SERVER_CTX pctx)
{
    if (checkPointer(pctx)) { return -1; }

    PHTTP_RESPONSE  response = &pctx->Response;

    response->ResponseLen = HTTP_Fill_Buffer_Response(pctx);

    ssize_t bytesSent = 0;

    // Send response
    bytesSent = send(pctx->ClientSocket, response->Buffer, response->ResponseLen, 0);

    if (bytesSent == 0) { return -2; }

    close(pctx->ClientSocket);

    return 0;
}

// Sets the response Code
int HTTP_Server_Set_Response_Code(PHTTP_SERVER_CTX pctx, HTTP_RESPONSE_CODE code)
{
    if (checkPointer(pctx)) { return -1; }

    PHTTP_RESPONSE response = &pctx->Response;

    response->Code = _HTTP_RESPONSE_CODE_ARRAY[code];
    response->CodeLen = strlen(response->Code);

    return 0;
}

// Sets the Content-Type header
int HTTP_Client_Set_Request_Content_Type(PHTTP_REQUEST request, HTTP_HEADER_CONTENT_TYPE type)
{
    if (request == NULL) { return -1; }

    request->ContentType = _HTTP_HEADER_CONTENT_TYPE_ARRAY[type];
    request->ContentTypeLen = strlen(request->ContentType);

    return 0;
}

// Sets the request Content
int HTTP_Client_Set_Request_Content(PHTTP_REQUEST request, const char *content, size_t length)
{
    if (request == NULL) { return -1; }
    if (content == NULL) { return -1; }

    request->Content = content;
    request->ContentLen = length;

    return 0;
}

// Waits for an incoming HTTP request
int HTTP_Server_Wait_For_Request(PHTTP_SERVER_CTX pctx)
{
    if (checkPointer(pctx)) { return -1; }

    PHTTP_REQUEST   request = &pctx->Request;

    ssize_t bytesReceived = 0;

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) { return -2; }

    int optlen = 1;

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optlen, sizeof(optlen)) < 0)
    {
        return -3;
    }

    if (bind(socketfd, (struct sockaddr *)&request->Addrin, sizeof(request->Addrin)) < 0)
    {
        return -4;
    }

    if (listen(socketfd, 3) < 0) { return -5; }

    int clientsocket = 0;
    int addrlen = sizeof(request->Addrin);

    clientsocket = accept(socketfd, (struct sockaddr *)&request->Addrin, (socklen_t*)&addrlen);

    if (clientsocket < 0) { return -6; }

    pctx->ClientSocket = clientsocket;

    bytesReceived = recv(clientsocket, request->Buffer, BUFFER_MAX_BYTES, 0);

    if (bytesReceived < 0) { return -7; }

    request->RequestLen = bytesReceived;

    if (HTTP_Parse_Request(request) < 0) { return -8; }

    return 0;
}

// Parses the request buffer
static int HTTP_Parse_Request(PHTTP_REQUEST request)
{
    char *current = NULL;
    int lenDiff = 0;

    request->Method = request->Buffer;
    if (checkPointer(current = memchr(request->Method, ' ', BUFFER_MAX_BYTES)))
    {
        request->Method = NULL;
        return -1;
    }
    request->MethodLen = current - request->Method;
    lenDiff = BUFFER_MAX_BYTES - request->MethodLen;

    request->Uri = current + 1;
    if (checkPointer(current = memchr(request->Uri, ' ', --lenDiff))) { return -1; }
    request->UriLen = current - request->Uri;
    lenDiff = lenDiff - request->UriLen;

    request->Version = current + 1;
    if (checkPointer(current = memchr(request->Version, '\r', --lenDiff))) { return -1; }
    request->VersionLen = current - request->Version;
    lenDiff = lenDiff - request->VersionLen;

    if (strncmp(request->Method, "GET", request->MethodLen) == 0)
    {
        if (HTTP_Parse_Request_Method_Parameters_GET(request) < 0)
        {
            return -2;
        }
    }
    else
    {
        return -3;
    }

    return 0;
}

// Parses parameters passed via the URI for a GET request
static int HTTP_Parse_Request_Method_Parameters_GET(PHTTP_REQUEST request)
{
    char *current = NULL;
    int x, lenDiff = 0;

    if (checkPointer(current = memchr(request->Uri, '?', request->UriLen)))
    {
        request->ParamCount = 0;
        return 0;
    }

    lenDiff = request->UriLen - (current - request->Uri);

    for (x = 0; (x < 10) && (current != NULL); x++)
    {
        request->Params[x].Name = current + 1;
        if (checkPointer(current = memchr(request->Params[x].Name, '=', --lenDiff)))
        {
            request->Params[x].Name = NULL;
            return -1;
        }
        request->Params[x].NameLen = current - request->Params[x].Name;
        lenDiff = lenDiff - request->Params[x].NameLen;

        request->Params[x].Value = current + 1;
        if (checkPointer(current = memchr(request->Params[x].Value, '&', --lenDiff)))
        {
            request->Params[x].ValueLen = lenDiff;
        }
        else
        {
            request->Params[x].ValueLen = current - request->Params[x].Value;
            lenDiff = lenDiff - request->Params[x].ValueLen;
        }
    }

    request->ParamCount = x;

    return 0;
}

// Parses the response buffer
static int HTTP_Parse_Response(PHTTP_RESPONSE response)
{
    char *current = NULL;
    int lenDiff = 0;

    response->Version = response->Buffer;
    if (checkPointer(current = memchr(response->Version, ' ', response->ResponseLen))) { return -1; }
    response->VersionLen = current - response->Version;
    lenDiff = response->ResponseLen - response->VersionLen;

    response->Code = current + 1;
    if (checkPointer(current = memchr(response->Code, '\r', --lenDiff))) { return -2; }
    response->CodeLen = current - response->Code;
    lenDiff = lenDiff - response->CodeLen;

    return 0;
}

// Fills request buffer with parameters passed via the URI for a GET request
static int HTTP_Fill_Buffer_Params_GET(PHTTP_REQUEST request, char **current)
{
    size_t currentLen = 0;

    // Copy param identifier
    memcpy(*current, "?", 1);
    currentLen++;
    (*current)++;

    // Copy all params to buffer
    for (int x = 0; x < request->ParamCount; x++)
    {
        memcpy(*current, request->Params[x].Name, request->Params[x].NameLen);
        *current += request->Params[x].NameLen;

        memcpy(*current, "=", 1);
        (*current)++;

        memcpy(*current, request->Params[x].Value, request->Params[x].ValueLen);
        *current += request->Params[x].ValueLen;

        currentLen += request->Params[x].NameLen + request->Params[x].ValueLen + 1;

        if ((x + 1) < request->ParamCount)
        {
            memcpy(*current, "&", 1);
            currentLen++;
            (*current)++;
        }
    }

    return currentLen;
}

// Fills the request buffer for an HTTP request
static int HTTP_Fill_Buffer_Request(PHTTP_REQUEST request)
{
    char *current = NULL;
    size_t currentLen = 0;
    int bytesCopied = 0;

    // Copy Method to buffer
    memcpy(request->Buffer, request->Method, request->MethodLen);

    // Set current to buffer address immediately after where Method was copied
    current = request->Buffer + request->MethodLen;

    // Copy space to buffer
    memcpy(current, " ", 1);

    // Increment current buffer length and pointer
    currentLen += request->MethodLen + 1;
    current++;

    // Copy URI to buffer
    memcpy(current, request->Uri, request->UriLen);

    // Increment current buffer length and pointer
    currentLen += request->UriLen;
    current += request->UriLen;

    // Check to see if there are params to copy
    if (request->ParamCount > 0)
    {
        if (request->Method == _HTTP_REQUEST_METHOD_ARRAY[HTTP_REQUEST_METHOD_GET])
        {
            currentLen += HTTP_Fill_Buffer_Params_GET(request, &current);
        }
    }

    // Copy space to buffer
    memcpy(current, " ", 1);
    currentLen++;
    current++;

    // Copy Version to buffer
    memcpy(current, request->Version, request->VersionLen);

    // Increment current buffer length and pointer
    currentLen += request->VersionLen;
    current += request->VersionLen;

    // Copy to to buffer
    memcpy(current, "\r\n", 2);
    currentLen += 2;
    current += 2;

    // Copy Host header to buffer
    memcpy(current, "Host: ", 6);
    currentLen += 6;
    current += 6;

    memcpy(current, request->Host, request->HostLen);
    currentLen += request->HostLen;
    current += request->HostLen;

    memcpy(current, ":", 1);
    currentLen++;
    current++;

    memcpy(current, request->RemotePort, request->RemotePortLen);
    currentLen += request->RemotePortLen;
    current += request->RemotePortLen;

    bytesCopied = snprintf(current, BUFFER_MAX_BYTES - currentLen, \
    	"\r\nUser-Agent: %s\r\n", HTTP_DEFAULT_USER_AGENT);

    currentLen += bytesCopied;
    current += bytesCopied;

    if (request->Method == _HTTP_REQUEST_METHOD_ARRAY[HTTP_REQUEST_METHOD_POST])
    {
        strncpy(current, "Content-Type: ", 14);
        currentLen += 14;
        current += 14;

        strncpy(current, request->ContentType, request->ContentTypeLen);
        currentLen += request->ContentTypeLen;
        current += request->ContentTypeLen;

        strncpy(current, "\r\n", 2);
        currentLen += 2;
        current += 2;

        bytesCopied = snprintf(current, BUFFER_MAX_BYTES - currentLen, \
        	"Content-Length: %zu\r\n", request->ContentLen);

        currentLen += bytesCopied;
        current += bytesCopied;

        strncpy(current, "\r\n", 2);
        currentLen += 2;
        current += 2;

        strncpy(current, request->Content, request->ContentLen);
        currentLen += request->ContentLen;
        current += request->ContentLen;
    }

    else
    {
      strncpy(current, "\r\n", 2);
      currentLen += 2;
      current += 2;
    }

    return currentLen;
}

// Fills the response buffer for an HTTP response
static int HTTP_Fill_Buffer_Response(PHTTP_SERVER_CTX pctx)
{
    PHTTP_RESPONSE response = &pctx->Response;
    char *current = NULL;
    int currentLen = 0;

    // Copy Version to buffer
    memcpy(response->Buffer, response->Version, response->VersionLen);

    // Set current to buffer address immediately after where Version was copied
    current = response->Buffer + response->VersionLen;

    // Copy space to buffer
    memcpy(current, " ", 1);

    // Increment current buffer length and pointer
    currentLen += response->VersionLen + 1;
    current++;

    // Copy Code to buffer
    memcpy(current, response->Code, response->CodeLen);

    // Set current to buffer address immediately after where Code was copied
    current += response->CodeLen;

    // Increment current buffer length and pointer
    currentLen += response->CodeLen;

    // Copy the rest to buffer
    currentLen += snprintf(current, BUFFER_MAX_BYTES - currentLen, \
        "\r\nServer: %s\r\n\r\n", HTTP_DEFAULT_USER_AGENT);

    return currentLen;
}

static int HTTP_Name_Lookup(PHTTP_REQUEST request)
{
    if (request == NULL) { return -1; }

    struct addrinfo hints, *result;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(request->Host, NULL, &hints, &result) != 0) { return -2; }

    request->Addrin.sin_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;

    freeaddrinfo(result);

    return 0;
}

int HTTPS_Client_Send_Request(PHTTPS_CLIENT_CTX pctx)
{
    if (checkPointer(pctx)) { return -1; }

    PHTTP_REQUEST	request = &pctx->Request;
    PHTTP_RESPONSE	response = &pctx->Response;
    SSL *ssl = pctx->ssl;

    if (request->HostSet == 1)
    {
        if (HTTP_Name_Lookup(request) < 0) { return -1; }
    }

    request->RequestLen = HTTP_Fill_Buffer_Request(request);

    int bytesSent, bytesReceived = 0;

    int socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if (socketfd < 0) { return -2; }

    // Connect to server
    if (connect(socketfd, (struct sockaddr *)&request->Addrin, sizeof(request->Addrin)) < 0)
    {
        return -3;
    }

    // Connect SSL object with file descriptor and perform handshake
    if (!SSL_set_fd(ssl, socketfd)) { return -2; }

    // Send request
    bytesSent = SSL_write(ssl, (const void *)request->Buffer, (int)request->RequestLen);

    if (bytesSent <= 0)
    {
        return -4;
    }

    bytesReceived = SSL_read(ssl, (void *)response->Buffer, BUFFER_MAX_BYTES);

    response->ResponseLen = bytesReceived;

    if (bytesReceived <= 0) { return -5; }

    if (HTTP_Parse_Response(response) < 0) { return -6; }

    if (!SSL_shutdown(ssl))
    {
        // Something did not shutdown right..
    }

    close(socketfd);

    return 0;
}

// Checks if the given pointer is a NULL pointer
static int checkPointer(void *ptr)
{
    if (ptr == NULL) { return 1; }

    return 0;
}
