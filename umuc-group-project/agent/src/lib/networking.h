#ifndef _NET_H
#define _NET_H

#define MAX_HOSTNAME_SIZE 255

typedef struct _HOST _HOST; //Declared in the implementation
typedef _HOST* Host;        //Defining Host as a type of pointer to an instance of a _HOST
typedef Host IPAddr;        //Alias for readability

extern Host localhost;

extern int ipInNetRange(Host h, long address);
extern IPAddr new_ip(char* address);
extern Host new_host(char* address, char* hostname, unsigned char* uuid);

#endif
