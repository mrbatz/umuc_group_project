#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "../logger.h"
#include "../date.h"
#include "package.h"
#include "../hash.h"
#include "../http.h"
#include "../json.h"

#define _ARCH(x) #x,
const char* const ARCH_NAMES[] = {
    _ARCHES
};
#undef _ARCH
#undef _ARCHES

#define _PMAN(x) #x,
const char* const PACMAN_NAMES[] = {
    _PMANS
};
#undef _PMAN
#undef _PMANS

struct _FILE {
    int        unchanged;
    int       customized;
    Date         created;
    Date    last_scanned;
    struct stat    stats;
    size_t          size;
    unsigned char   hash[SHA256_LENGTH];
    char          path[MAX_PATH_LENGTH];
    char expected_path[MAX_PATH_LENGTH];
};

struct _PACKAGE {
    int          unchanged;
    Suite            suite;
    Arch              arch;
    PackageManager  pacman;
    ServiceStatus   status;
    int         customized;
    Date           created;
    Date      last_scanned;
    int          num_files;
    size_t       file_size;
    size_t            size;
    size_t       disk_used;
    File*            files; //Pointer to pointer to _FILE struct, contains num_files File objects.
    ServicePersistence   persistence;
    unsigned char  hash[SHA256_LENGTH];
    char        name[MAX_PACKAGE_NAME];
    char   version[MAX_VERSION_LENGTH];
};

Arch               default_arch = UNK_ARCH;
Suite            default_suite = UNK_SUITE;
PackageManager default_pacman = UNK_PACMAN;

static void upper(char* str, int len)
{
    int i;
    for (i=0; i < len; i++)
    {
        if (islower(str[i]))
        {
            str[i] += 'A'-'a';
        }
    }
}

extern Arch determine_arch(char* str)
{
    int len = strnlen(str, 10);
    if (len < 2)
    {
        return UNK_ARCH;
    }

    Arch ret = UNK_ARCH;

    int closest = 100;
    int i;
    upper(str, len);
    for (i=0; i < END_ARCH; i++)
    {
        int closeness = strncmp(str, ARCH_NAMES[i], 10);
        if (closeness == 0)
        {
            ret = (Arch)i;
            break;
        }
        else if (abs(closeness) < closest)
        {
            closest = closeness;
            ret = (Arch)i;
        }
    }

    return ret;
}

extern int compare_files(File* f, File* f2, int check) //Will update first argument
{
    if ((f == NULL || f2 == NULL) ||
        (f->path[0] == '\0' || f2->path[0] == '\0'))
    {
        log(ERR, "Failed to compare Package* Files.");
        return -1;
    }

    if (strncmp(f->path, f2->path, MAX_PATH_LENGTH) != 0)
    {
        return 1; //New File
    }

    memcpy(&(f->created), &(f2->created), sizeof(f2->created));

    if (check)
    {
        if (memcmp(f->hash, f2->hash, SHA256_LENGTH) != 0)
        {
            f->unchanged = 0;
        }
        else
        {
            f->unchanged = 1;
        }
    }

    return 0;
}

extern int add_file(Package* p, char filepath[MAX_PATH_LENGTH])
{
    if (filepath[0] == '\0')
    {
        return 1;
    }

    File f;

    strncpy(f.path, filepath, MAX_PATH_LENGTH);
    strncpy(f.expected_path, filepath, MAX_PATH_LENGTH);

    f.customized = 0;
    f.unchanged =  0;

    if (new_date(&(f.created)) || new_date(&(f.last_scanned)))
    {
        return 1;
    }
    else if (sha256sum_file(f.path, f.hash))
    {
        return 1;
    }
    
    int i;
    for (i=0; i < p->num_files; i++)
    {
        if (!compare_files(&f, &(p->files[i]), 0))
        {
            log(DBG, "File already present: '%s'", filepath);
            return 0;
        }
    }

    struct stat st;
    stat(filepath, &st);
    
    f.stats = st;
    f.size = st.st_size;

    p->files = realloc(p->files, (p->num_files + 1)*sizeof(f));
    if (p->files == NULL)
    {
        log(CRIT, "Out of memory on package: '%s', file: '%s'", p->name, filepath);
        return 0;
    }

    memcpy(&(p->files[p->num_files]), &f, sizeof(f));
    p->num_files += 1;
    p->file_size += sizeof(f);
    p->size += sizeof(f);
    p->disk_used += f.size;
    return 0;
}

extern int compare_packages(Package* p, Package* p2, int check)
{
    if ((p == NULL || p2 == NULL) ||
        (p->name[0] == '\0' || p2->name[0] == '\0') ||
        (p->version[0] == '\0' || p2->version[0] == '\0'))
    {
        log(ERR, "Invalid package.");
        return 1;
    }

    if (strncmp(p->name, p2->name, MAX_PACKAGE_NAME) != 0) //if package doesnt exist, return new package
    {
        return 1;
    }

    memcpy(&p->created, &p2->created, sizeof(p2->created));

    if (memcmp(p->hash, p2->hash, SHA256_LENGTH) != 0) //if package doesnt match existing hash
    {
        return 0;
    }
    else if (strncmp(p->version, p2->version, MAX_VERSION_LENGTH) != 0) //if package matches hash, but different version
    {
        return 0;
    }

    if (check)
    {
        int completed_new[p->num_files];

        int i, j;
        for (i=0; i < p->num_files; i++)
        {
            completed_new[i] = 0;
        }

        for (i = 0; i < p2->num_files; i++)
        {
            File old = p2->files[i];

            for (j = 0; j < p->num_files; j++)
            {
                if (completed_new[j]){
                    continue;
                }

                File new = p->files[j];

                if (!compare_files(&new, &old, 1))
                {
                    completed_new[j] = 1;
                }
            }
        }

        int match = 0;
        for (i=0; i < p->num_files; i++)
        {
            match += (p->files[i].unchanged) ? 0 : 1;
        }

        if (match == 0)
        {
            p->unchanged = 1;
        }
        else
        {
            p->unchanged = 0;
        }
    }
    
    return 0;
}

extern void destroy_package(Package* p)
{
    if (p == NULL)
    {
        return;
    }

    if (p->files != NULL)
    {
        free(p->files);
        p->files = NULL;
    }

    free(p);
    p = NULL;
}

extern Package* new_package(char filepath[MAX_PATH_LENGTH], char name[MAX_PACKAGE_NAME], char version[MAX_VERSION_LENGTH], PackageManager pm, Suite su, Arch ar)
{
    _PACKAGE temp_p;

    temp_p.files = NULL;
    temp_p.num_files = 0;
    temp_p.size = sizeof(temp_p);
    temp_p.file_size = 0;
    temp_p.disk_used = 0;
    if (add_file(&temp_p, filepath))
    {
        return NULL;
    }

    unsigned char hash[SHA256_LENGTH] = { '\0' };
    strncpy(temp_p.name, name, MAX_PACKAGE_NAME);
    strncpy(temp_p.version, version, MAX_VERSION_LENGTH);
    memcpy(temp_p.hash, hash, SHA256_LENGTH);

    temp_p.unchanged    =                 0;
    temp_p.pacman       =                pm;
    temp_p.suite        =                su;
    temp_p.arch         =                ar;
    temp_p.customized   =                 0;
    temp_p.status       =        UNK_STATUS;
    temp_p.persistence  =   UNK_PERSISTENCE;

    if (new_date(&(temp_p.created)) || new_date(&(temp_p.last_scanned)))
    {
        return NULL;
    }
    
    Package* p = NULL;
    if ((p = malloc(temp_p.size)) == NULL)
    {
        log(CRIT, "Insufficient memory for new package!");
        return NULL;
    }

    memcpy(p, &temp_p, sizeof(temp_p));

    return p;
}

extern int sha256sum_package(Package* p, unsigned char hash[SHA256_LENGTH])
{
    unsigned char buffer[SHA256_LENGTH] = { '\0' };

    SHA256_CTX_t sha256;
    sha256_init(&sha256);

    int i;
    for(i=0; i < p->num_files; i++)
    {
        if (sha256sum_file(p->files[i].path, buffer))
        {
            return 1;
        }
        sha256_update(&sha256, buffer, SHA256_LENGTH);
    }

    sha256_final(&sha256, hash);
    return 0;
}

extern int rehash_package(Package* p)
{
    unsigned char hash[SHA256_LENGTH] = { '\0' };
    if (p == NULL || sha256sum_package(p, hash))
    {
        return 1;
    }

    memcpy(p->hash, hash, SHA256_LENGTH);
    return 0;
}

extern Package* read_package(FILE* f, size_t* length)
{
    size_t start = ftell(f);
    size_t data_length = 0;
    size_t current_pos = start;

    fseek(f, 0, SEEK_END);
    size_t end = ftell(f);
    fseek(f, current_pos, SEEK_SET);

    if (end - ftell(f) < sizeof(size_t))
    {
        fseek(f, 0, SEEK_END);
        return NULL;
    }

    fread(&data_length, sizeof(size_t), 1, f);
    if (data_length == 0)
    {
        return NULL;
    }
    *length += data_length + sizeof(size_t);

    current_pos = ftell(f);
    if (end - current_pos < data_length)
    {
        fseek(f, 0, SEEK_END);
        return NULL;
    }

    size_t test;
    int all_nulls = 1;
    for (test=0; test < data_length; test++)
    {
        if (fgetc(f) != '\0')
        {
            all_nulls = 0;
            break;
        }
    }

    if (all_nulls)
    {
        return NULL;
    }
    fseek(f, current_pos, SEEK_SET);


    Package* p = malloc(sizeof(*p));
    if (p == NULL)
    {
        log(CRIT, "Insufficient memory for storing a package!");
        fseek(f, data_length, SEEK_CUR);
        return NULL;
    }

    fread(p, sizeof(*p), 1, f);
    current_pos = ftell(f);

    p->files = NULL;
    p->files = (File*)malloc(p->file_size);
    if (p->files == NULL)
    {
        free(p);
        p = NULL;
        log(CRIT, "Insufficient memory for storing all of the package files!");
        fseek(f, start + data_length, SEEK_SET);
        return NULL;
    }

    fread(p->files, sizeof(File), p->num_files, f);
    current_pos = ftell(f);

    if (data_length + sizeof(size_t) != current_pos - start)
    {
        log(WARN, "Package file '%s' was not read entirely. This WILL cause errors.\nExpected: %ld\nReceived %ld", p->name, sizeof(size_t)+data_length, current_pos-start);
        fseek(f, data_length-current_pos, SEEK_CUR);
    }

    return p;
}

extern int write_package(FILE* f, Package* p)
{
    if (p == NULL)
    {
        return 1;
    }

    fwrite(&(p->size), sizeof(size_t), 1, f);
    fwrite(p, sizeof(*p), 1, f);
    fwrite(p->files, sizeof(File), p->num_files, f);
    if (ferror(f))
    {
        log(ERR, "Something went wrong with writing package '%s'.", p->name);
        destroy_package(p);
        return 1;
    }

    destroy_package(p);
    return 0;
}

extern int add_package(Package* p)
{
    FILE* f = fopen(PACKAGE_FILE, "ab");
    if (f == NULL)
    {
        log(ERR, "Failed to open package file for appending. Name: %s", PACKAGE_FILE);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    int ret = write_package(f, p);
    fclose(f);
    return ret;
}

extern int collapse_nulls()
{
    FILE* f  = fopen(PACKAGE_FILE, "r");
    if (f == NULL || ferror(f))
    {
        log(ERR, "Failed to open package file for reading. File: %s", PACKAGE_FILE);
        return 1;
    }

    FILE* f2 = fopen(PACKAGE_FILE_TMP, "w");
    if (f2 == NULL || ferror(f2))
    {
        fclose(f);
        log(ERR, "Failed to open temporary package file for writing. TMP File: %s", PACKAGE_FILE_TMP);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t p_file_len = ftell(f);
    rewind(f);

    while ( p_file_len - ftell(f) >= sizeof(Package) ) //Reading the file for package structs
    {
        size_t end = 0;
        Package* p = read_package(f, &end);
        if (p == NULL)
        {
            continue;
        }

        if (write_package(f2, p))
        {
            fclose(f);
            fclose(f2);
            return 1;
        }
    }

    if (fclose(f) || fclose(f2))
    {
        log(ERR, "Failed to close package files. Files: '%s' and '%s'", PACKAGE_FILE, PACKAGE_FILE_TMP);
        return 1;
    }
    else if (remove(PACKAGE_FILE))
    {
        log(ERR, "Failed to remove the package file '%s'.", PACKAGE_FILE);
        return 1;
    }

    if ((f2 = fopen(PACKAGE_FILE_TMP, "r")) == NULL || ferror(f2))
    {
        log(ERR, "Failed to reopen temporary package file for reading.\nTemporary file still on disk! TMP File: %s", PACKAGE_FILE_TMP);
        return 1;
    }

    fseek(f2, 0, SEEK_END);
    p_file_len = ftell(f2);
    rewind(f2);

    while ( p_file_len - ftell(f2) >= sizeof(Package) ) //Reading the file for package structs
    {
        size_t end = 0;
        Package* p = read_package(f2, &end);
        if (add_package(p))
        {
            fclose(f);
            fclose(f2);
            log(ERR, "Failed to write package to file. File: %s\nTemporary file still on disk! TMP File: %s", PACKAGE_FILE, PACKAGE_FILE_TMP);
            return 1;
        }
    }

    if (remove(PACKAGE_FILE_TMP))
    {
        log(ERR, "Failed to remove temporary package file. File: %s", PACKAGE_FILE_TMP);
        return 1;
    }

    return 0;
}

extern int update_package(Package* p)
{
    FILE* f = NULL;
    if ((f = fopen(PACKAGE_FILE, "r+b")) == NULL)
    {
        return add_package(p);
    }

    size_t end = 0;
    Package* old_p;
    int package_found = 0;

    fseek(f, 0, SEEK_END);
    size_t p_file_len = ftell(f);
    rewind(f);

    while ( p_file_len - ftell(f) >= sizeof(Package) ) //Reading the file for package structs
    {
        old_p = read_package(f, &end);
        if (old_p == NULL)
        {
            continue;
        }

        if (!compare_packages(p, old_p, 1))
        {
            ++package_found;
            break;
        }

        destroy_package(old_p);
    }

    if (!package_found)
    {
        log(INFO, "New package found!\n\tName: %s", p->name);
        fclose(f);
        return add_package(p);
    }

    fseek(f, -(old_p->size), SEEK_CUR);

    if (p->unchanged)
    {
        if (write_package(f, p))
        {
            log(ERR, "Already discovered package failed to be updated in Package File.\n\tPackage: %s", p->name);
        }

        fclose(f);
        return 0;
    }

    log(INFO, "Package updated! Name: %s", p->name);
    unsigned char buf[old_p->size];
    size_t i;
    for (i=0; i < old_p->size; i++)
    {
        buf[i] = '\0';
    }

    fwrite(buf, old_p->size, 1, f);
    fclose(f);

    if (collapse_nulls())
    {
        log(ERR, "Removing null characters from the Package File failed.\n\tFile: %s\n\tPackage: %s", PACKAGE_FILE, p->name);
    }

    return add_package(p);
}

extern int ship_packages()
{
    FILE* f = fopen(PACKAGE_FILE, "rb");
    if (f == NULL)
    {
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t p_file_len = ftell(f);
    rewind(f);

    while ( p_file_len - ftell(f) >= sizeof(Package) ) //Reading the file for package structs
    {
        PHTTP_CLIENT_CTX client_ctx = HTTP_Init_Client_CTX();
        if (client_ctx == NULL)
        {
            fclose(f);
            return 1;
        }

        PHTTP_REQUEST request = &(client_ctx->Request);
        if (HTTP_Client_Set_Request_Address(request, management_console_ip) ||
            HTTP_Client_Set_Request_Port(request, management_console_port) ||
            HTTP_Client_Set_Request_Uri(request, "/beacon"))
        {
            fclose(f);
            return 1;
        }

        size_t end = 0;
        Package* p = read_package(f, &end);
        if (p == NULL)
        {
            HTTP_Free_Client_CTX(client_ctx);
            continue;
        }

        size_t bufferSize = 0;
        JSON_OBJECT arr[8];
        JSON_Set_Object_String(arr, "Package Name", p->name);
        JSON_Set_Object_String(arr+1, "Version", p->version);
        JSON_Set_Object_String(arr+2, "Arch", ARCH_NAMES[p->arch]);
        JSON_Set_Object_String(arr+3, "Package Manager", PACMAN_NAMES[p->pacman]);

        char num_files[30] = { '\0' };
        sprintf(num_files, "%d", p->num_files);

        JSON_Set_Object_Number(arr+4, "Number of files", num_files);
        
        char space[30] = { '\0' };
        sprintf(space, "%ld", p->disk_used);

        JSON_Set_Object_Number(arr+5, "Total Disk Used", space);

        size_t possible_len = ((p->num_files+2)*MAX_PATH_LENGTH) + 1;
        char filenames[possible_len];
        size_t i;
        for (i=0; i < possible_len; i++)
        {
            filenames[i] = '\0';
        }

        for (i=0; i < p->num_files; i++)
        {
            strncat(filenames, (p->files[i]).path, MAX_PATH_LENGTH);
            if (i != p->num_files - 1) strncat(filenames, ", ", sizeof(", "));
        }

        JSON_Set_Object_String(arr+6, "Filenames", filenames);

        char hash_str[(SHA256_LENGTH*2)+1] = { '\0' };
        for (i=0; i < SHA256_LENGTH; i++)
        {
            sprintf(hash_str+2*i, "%.2x", p->hash[i]);
        }

        JSON_Set_Object_String(arr+7, "SHA256", hash_str);

        const char* buffer = JSON_Get_Structured_Buffer(arr, 7, &bufferSize);

        HTTP_Client_Set_Request_Method(request,  HTTP_REQUEST_METHOD_POST);
        HTTP_Client_Set_Request_Content_Type(request, HTTP_HEADER_CONTENT_TYPE_JSON);
        HTTP_Client_Set_Request_Content(request, buffer, bufferSize);

        int ret = 0;
        if (HTTP_Client_Send_Request(client_ctx))
        {
            ret = 1;
        }

        HTTP_Free_Client_CTX(client_ctx);
        destroy_package(p);
        if (ret)
        {
            fclose(f);
            return 1;
        }
    }

    return 0;
}
