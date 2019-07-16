#include <stdio.h>
#include "logger.h"
#include "networking.h"

const char* const BLACKLIST_DIRECTORIES[] = {
    "/proc"
};

const char* const BLACKLIST_FILES[] = {
    LOG_NAME
};

const int BLACKLIST_PORTS[] = {
    12345
};

Host* BLACKLIST_HOSTS = &localhost;
