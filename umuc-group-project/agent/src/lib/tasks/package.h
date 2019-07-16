#ifndef _PACKAGE_H
#define _PACKAGE_H

#include "dirwalk.h"
#include "../hash.h"

#define MAX_PACKAGE_NAME 128
#define MAX_VERSION_LENGTH 64
#define PACKAGE_FILE "/tmp/agent.packages"
#define PACKAGE_FILE_TMP "/tmp/agent.packages.tmp"

typedef enum SUITES {
    UNK_SUITE,
    TRUSTY,
    XENIAL,
    BIONIC,
    COSMIC,
    DISCO,
    END_SUITE
} Suite;

#define _ARCHES _ARCH(UNK_ARCH)_ARCH(AMD64)_ARCH(ARM64)_ARCH(ARMHF)_ARCH(I386)_ARCH(PPC64EL)_ARCH(S390X)
#define _ARCH(x) x,
typedef enum ARCHITECHTURES {
    _ARCHES
    END_ARCH
} Arch;
#undef _ARCH

#define _PMANS _PMAN(UNK_PACMAN)_PMAN(DPKG)_PMAN(APTITUDE)_PMAN(APT)_PMAN(YUM)_PMAN(PIP)_PMAN(PACMAN)_PMAN(PKG)_PMAN(PIP3)_PMAN(RPM)
#define _PMAN(x) x,
typedef enum PACKAGE_MANAGERS {
    _PMANS
    END_PACMAN
} PackageManager;
#undef _PMAN

typedef enum SERVICE_PERSISTENCE {
    UNK_PERSISTENCE,
    DISABLED,
    ENABLED,
    AUTORUN,
    END_PERSISTENCE
} ServicePersistence;

typedef enum SERVICE_STATUSES {
    UNK_STATUS,
    STOPPED,
    STARTED,
    RESTARTING,
    HUNG,
    SERVICE_ERROR,
    END_STATUS
} ServiceStatus;

extern Suite            default_suite;
extern Arch              default_arch;
extern PackageManager  default_pacman;

typedef struct _FILE _FILE;
typedef _FILE File;

typedef struct _PACKAGE _PACKAGE;
typedef _PACKAGE Package;


extern Arch determine_arch(char* str);
extern int compare_files(File* f, File* f2, int check);
extern int add_file(Package* p, char filepath[MAX_PATH_LENGTH]);
extern int compare_packages(Package* p, Package* p2, int check);
extern void destroy_package(Package* p);
extern Package* new_package(char filepath[MAX_PATH_LENGTH], char name[MAX_PACKAGE_NAME], char version[MAX_VERSION_LENGTH], PackageManager pm, Suite su, Arch ar);
extern int sha256sum_package(Package* p, unsigned char hash[SHA256_LENGTH]);
extern int rehash_package(Package* p);
extern Package* read_package(FILE* f, size_t* length);
extern int write_package(FILE* f, Package* p);
extern int add_package(Package* p);
extern int collapse_nulls();
extern int update_package(Package* p);
extern int ship_packages();
#endif
