#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "logger.h"
#include "networking.h"

Host localhost = NULL;

struct _HOST {
    long     address;
    long        mask;
    char*   hostname;
    char*       uuid;
    int (*compare)(Host ip, long address);
};

extern int ipInNetRange(Host net, long ip)
{
    long netAddr = net->address;
    long netMask = net->mask;

    long newIp = ip & netMask;
    if (newIp ^ netAddr)
    {
        return 0;
    }

    return 1;
}

static unsigned char enumerateCIDR(char* token, int* len)
{
    int cidrLength = 0;
    if (*len <= 3 && token[1] != '/') //When no CIDR was provided.
    {
        return 32;
    }

    if (*len > 3 && token[*len-3] == '/')
    {
        cidrLength = 2; //Assigning cidrLength to 2 since '/' is before remaining two characters.
    }
    else if (*len > 2 && token[*len-2] == '/')
    {
        cidrLength = 1; //Assigning cidrLength to 1 since '/' is before last character.
    }
    else
    {
        log(WARN, "Invalid CIDR provided. Defaulting to /32. Got %s.", token);
        return 32;
    }

    *len = *len - cidrLength - 1;
    token[*len] = '\0';

    int bottom = *len + 1;              //Some aliasing to make more readable
    int top = *len + cidrLength;

    if (cidrLength == 1 && !isdigit(token[top]) && token[top] <= 32)
    {
        return token[top]; //Very abnormal case, but allow a cidr to be passed as an exact value rather than the representation
    }

    int i;
    unsigned char cidr = '\0';
    for (i=bottom; i < top; i++)
    {
        if (!isdigit(token[i]))
        {
            log(ERR, "Unexpected value in CIDR provided. Defaulting to /32. Got %s.", token);
            return 32;
        }

        cidr += token[i] - '0'; //Drops the character representation and instead pulls the actual value of the token, still using char because it allows up to 255.
        if (cidr < token[i])
        {
            log(ERR, "CIDR has overflown, invalid CIDR provided. Defaulting to /32. Got %s.", token);
            return 32;
        }
    }

    return cidr;
}

static int parseIP(char* address, unsigned long* ip, unsigned long* netmask)
{
    int len = strnlen(address, 18); //same as strnlen(address, sizeof("255.255.255.255/32")) since that is the largest ipv4 address
    if (len < 7 || len > 18)        //same as saying if (len < sizeof("0.0.0.0") || len > sizeof("255.255.255.255/32"))
    {
        return 1;
    }

    char localCopy[18] = { '\0' }; //Creating duplicate because tokenizing overwrites the original string.
    strncpy(localCopy, address, len);

    char* values[4] = { NULL };
    unsigned char octets[5] = { '\0' };

    values[0] = strtok(&(localCopy[0]), ".");
    if (values[0] == NULL) {
        log(ERR, "Invalid ip address provided. Currently, only ipv4 is supported.\nProblem was in the First Octet: %s", address);
        return 1;
    }

    values[1] = strtok(NULL, ".");
    if (values[1] == NULL) {
        log(ERR, "Invalid ip address provided. Currently, only ipv4 is supported.\nProblem was in the Second Octet: %s\n\tFirst was: %s", address, values[0]);
        return 1;
    }

    values[2] = strtok(NULL, ".");
    if (values[2] == NULL) {
        log(ERR, "Invalid ip address provided. Currently, only ipv4 is supported.\nProblem was in the Third Octet: %s\n\tFirst was: %s.\n\tSecond was: %s", address, values[0], values[1]);
        return 1;
    }

    values[3] = strtok(NULL, "");
    if (values[3] == NULL) {
        log(ERR, "Invalid ip address provided. Currently, only ipv4 is supported.\nProblem was in the Fourth Octet: %s\n\tFirst was: %s\n\tSecond was: %s\n\tThird was: %s", address, values[0], values[1], values[2]);
        return 1;
    }

    len = strnlen(values[3], 6); //Doing the cidr check here so that I can put everything else in a for loop.
    if (len > 2)
    {
        octets[4] = enumerateCIDR(values[3], &len); //returns the CIDR as an unsigned char
    }
    else
    {
        octets[4] = 32;
    }

    int i, j;
    for (i=0; i < 4; i++)
    {
        len = strnlen(values[i], 3);
        if (len == 1 && !isdigit(values[i][1])) //bit representation of an octet? Will not work when equal to '0'-'9' (48-57)
        {
            octets[i] = values[i][1]; //Same rare case described above in the enumerateCIDR function.
            continue;
        }

        unsigned char value = '\0';
        for (j=0; j < len; j++)
        {
            value += values[i][j] - '0';
            if (value < values[i][j])
            {
                log(ERR, "Octet %d has overflown. Is '%s' < 256?\nAddress: %s", i, values[i], address);
                return 1;
            }
        }
        if (i == 0 && value == 0 && octets[4] != 0)
        {
            log(ERR, "Invalid 0 value for first octet without a CIDR of 0.\nAddress: %s", address);
            return 1;
        }
        octets[i] = value;
    }

    *ip = (octets[0]<<24) + (octets[1]<<16) + (octets[2]<<8) + octets[3];
    *netmask = (1<<octets[4])-1;

    *ip &= *netmask;

    return 0;
}

static int parseHostname(char* hostname)
{
    if (hostname[0] == '\0' || !isprint(hostname[0]))
    {
        return 1;
    }

    int len = strnlen(hostname, MAX_HOSTNAME_SIZE);
    int i;
    for (i=0; i < len; i++)
    {
        if (!isprint(hostname[i]))
        {
            return 1;
        }
    }

    return 0;
}

static Host _newHost(long address, long netmask, char* hostname, char* uuid)
{
    Host host;
    if ((host = malloc(sizeof(*host))) == NULL)
    {
        return NULL;
    }

    host->address  =       address;
    host->mask     =       netmask;
    host->hostname =      hostname;
    host->uuid     =          uuid;
    host->compare  = &ipInNetRange;
    return host;
}

extern Host newIP(char* address) //address should include CIDR to look like 1.1.1.1/24 or look like 1.1.1.1 for automatic /32 cidr
{
    unsigned long addr;
    unsigned long mask;
    if (parseIP(address, &addr, &mask))
    {
        return NULL;
    }
    
    return _newHost(addr, mask, NULL, NULL); 
}

extern Host newHost(char* address, char* hostname, char* uuid)
{
    unsigned long addr;
    unsigned long mask;
    if (parseIP(address, &addr, &mask))
    {
        return NULL;
    }

    if (parseHostname(hostname))
    {
        return NULL;
    }

    if (uuid == NULL || (strnlen(uuid, 32) < 32))
    {
        return NULL;
    }

    return _newHost(addr, mask, hostname, uuid);
}
