#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "permission.h"

#define MAX_ITEMS 1000


/* ============================================================
   COMMON FUNCTION
   ============================================================ */

static void pressEnter(void)
{
    printf("\nPress Enter to continue...");
    getchar();
}


/* ============================================================
   STRING COMPARISON
   ============================================================ */

static int compareStrings(const void *a, const void *b)
{
    const char *sa = a;
    const char *sb = b;

    return strcmp(sa, sb);
}


/* ============================================================
   FIND FILES AND FOLDERS
   ============================================================ */

static void findItems(const char *base,
                      const char *rel,
                      char items[][PATH_MAX],
                      int *count)
{
    if (*count >= MAX_ITEMS)
        return;

    DIR *dir = opendir(base);

    if (!dir)
        return;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL &&
           *count < MAX_ITEMS)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char fullPath[PATH_MAX];
        char relativePath[PATH_MAX];

        snprintf(fullPath,
                 sizeof(fullPath),
                 "%s/%s",
                 base,
                 entry->d_name);

        snprintf(relativePath,
                 sizeof(relativePath),
                 "%s%s%s",
                 rel,
                 rel[0] ? "/" : "",
                 entry->d_name);

        struct stat st;

        if (lstat(fullPath, &st) == -1)
            continue;

        /*
         * Add both files and folders.
         */
        snprintf(items[*count],
                 PATH_MAX,
                 "%s",
                 relativePath);

        (*count)++;

        /*
         * Search inside folders also.
         */
        if (S_ISDIR(st.st_mode))
        {
            findItems(fullPath,
                      relativePath,
                      items,
                      count);
        }
    }

    closedir(dir);
}


/* ============================================================
   CONVERT PERMISSION TO rwxrwxrwx FORMAT
   ============================================================ */

static void permissionString(mode_t mode,
                             char permissions[11])
{
    permissions[0] = S_ISDIR(mode) ? 'd' : '-';

    /* Owner */
    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* Group */
    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* Others */
    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';

    permissions[10] = '\0';
}


/* ============================================================
   VALIDATE PERMISSION
   ============================================================ */

static int validPermission(const char *permission)
{
    if (permission == NULL)
        return 0;

    if (strlen(permission) != 3)
        return 0;

    for (int i = 0; i < 3; i++)
    {
        if (permission[i] < '0' ||
            permission[i] > '7')
        {
            return 0;
        }
    }

    return 1;
}


/* ============================================================
   CONVERT 777 / 755 / 644 etc. TO MODE
   ============================================================ */

static mode_t toMode(const char *permission)
{
    int owner = permission[0] - '0';
    int group = permission[1] - '0';
    int others = permission[2] - '0';

    mode_t mode = 0;

    /* Owner */
    if (owner & 4)
        mode |= S_IRUSR;

    if (owner & 2)
        mode |= S_IWUSR;

    if (owner & 1)
        mode |= S_IXUSR;

    /* Group */
    if (group & 4)
        mode |= S_IRGRP;

    if (group & 2)
        mode |= S_IWGRP;

    if (group & 1)
        mode |= S_IXGRP;

    /* Others */
    if (others & 4)
        mode |= S_IROTH;

    if (others & 2)
        mode |= S_IWOTH;

    if (others & 1)
        mode |= S_IXOTH;

    return mode;
}


/* ============================================================
   CHANGE PERMISSIONS
   ============================================================ */

void changePermissions(void)
{
    char cwd[PATH_MAX];
    char items[MAX_ITEMS][PATH_MAX];
    char input[100];

    int count = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return;

    findItems(cwd,
              "",
              items,
              &count);

    qsort(items,
          (size_t)count,
          sizeof(items[0]),
          compareStrings);

    printf("\n");
    printf("==============================================================\n");
    printf("                    CHANGE PERMISSIONS\n");
    printf("==============================================================\n\n");

    printf("Current Directory : %s\n\n", cwd);

    printf("Files and Folders:\n");
    printf("--------------------------------------------------------------\n");

    if (count == 0)
    {
        printf("No files or folders found.\n");
        pressEnter();
        return;
    }

    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n",
               i + 1,
               items[i]);
    }

    printf("\n0. Cancel\n");

    printf("\nEnter File/Folder Number : ");

    if (!fgets(input,
               sizeof(input),
               stdin))
    {
        return;
    }

    char *end;
    long number = strtol(input, &end, 10);

    if (end == input ||
        number < 0 ||
        number > count)
    {
        printf("\n✗ Invalid item number.\n");
        pressEnter();
        return;
    }

    if (number == 0)
        return;

    char path[PATH_MAX];

    snprintf(path,
             sizeof(path),
             "%s/%s",
             cwd,
             items[number - 1]);

    struct stat st;

    if (lstat(path, &st) == -1)
    {
        perror("lstat");
        pressEnter();
        return;
    }

    const char *name = strrchr(items[number - 1], '/');

    if (name)
        name++;
    else
        name = items[number - 1];

    char location[PATH_MAX];

    snprintf(location,
             sizeof(location),
             "%s",
             path);

    char *slash = strrchr(location, '/');

    if (slash)
        *slash = '\0';

    char permissions[11];

    permissionString(st.st_mode,
                     permissions);

    printf("\n");
    printf("==============================================================\n");
    printf("                     PERMISSION DETAILS\n");
    printf("==============================================================\n\n");

    printf("Selected Item : %s\n\n",
           name);

    printf("Current Permissions : %03o (%s)\n",
           st.st_mode & 0777,
           permissions);

    printf("\n");
    printf("Permission Examples:\n");
    printf("--------------------------------------------------------------\n");

    printf("777 - rwxrwxrwx - Everyone can read, write, execute\n");
    printf("755 - rwxr-xr-x - Owner full; others read + execute\n");
    printf("700 - rwx------ - Owner full only\n");
    printf("644 - rw-r--r-- - Owner read/write; others read\n");
    printf("600 - rw------- - Owner read/write only\n");

    printf("\nPermission Symbols:\n");
    printf("--------------------------------------------------------------\n");

    printf("r = Read\n");
    printf("w = Write\n");
    printf("x = Execute\n");
    printf("- = No permission\n");

    printf("\nEnter New Permission (777/755/700/644/600) : ");

    char newPermission[20];

    if (!fgets(newPermission,
               sizeof(newPermission),
               stdin))
    {
        return;
    }

    newPermission[strcspn(newPermission, "\n")] = '\0';

    if (!validPermission(newPermission))
    {
        printf("\n✗ Invalid permission.\n");
        printf("Please enter a 3-digit value using digits 0-7.\n");
        pressEnter();
        return;
    }

    mode_t newMode = toMode(newPermission);

    if (chmod(path, newMode) == -1)
    {
        perror("chmod");
        pressEnter();
        return;
    }

    struct stat updated;

    if (lstat(path, &updated) == -1)
    {
        perror("lstat");
        pressEnter();
        return;
    }

    permissionString(updated.st_mode,
                     permissions);

    printf("\n");
    printf("✓ Permission changed successfully.\n\n");

    printf("Name        : %s\n", name);

    printf("Permission  : %03o (%s)\n",
           updated.st_mode & 0777,
           permissions);

    printf("Location    : %s\n",
           location);

    pressEnter();
}
