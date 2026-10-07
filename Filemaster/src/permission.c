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
                      int *count,
                      int wantFolders,
                      int wantFiles)
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
         * Add only the requested type.
         */
        if ((S_ISDIR(st.st_mode) && wantFolders) ||
            (S_ISREG(st.st_mode) && wantFiles))
        {
            snprintf(items[*count],
                     PATH_MAX,
                     "%s",
                     relativePath);

            (*count)++;
        }

        /*
         * Always search inside folders.
         */
        if (S_ISDIR(st.st_mode))
        {
            findItems(fullPath,
                      relativePath,
                      items,
                      count,
                      wantFolders,
                      wantFiles);
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

    int typeChoice;

    /*
     * Get current directory.
     */
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return;

    printf("\n");
    printf("==============================================================\n");
    printf("                    CHANGE PERMISSIONS\n");
    printf("==============================================================\n\n");

    printf("Current Directory : %s\n\n", cwd);

    /*
     * Select Folder / File.
     */
    printf("1. Folder\n");
    printf("2. File\n");
    printf("0. Cancel\n");

    printf("\nEnter your choice : ");

    if (!fgets(input,
               sizeof(input),
               stdin))
    {
        return;
    }

    char *end;
    long choice = strtol(input, &end, 10);

    while (*end == ' ' ||
           *end == '\t' ||
           *end == '\n')
    {
        end++;
    }

    if (end == input ||
        *end != '\0' ||
        choice < 0 ||
        choice > 2)
    {
        printf("\n✗ Invalid choice. Please select 1, 2, or 0.\n");
        pressEnter();
        return;
    }

    if (choice == 0)
        return;

    typeChoice = (int)choice;

    /*
     * Find selected type recursively.
     */
    int count = 0;

    if (typeChoice == 1)
    {
        findItems(
            cwd,
            "",
            items,
            &count,
            1,
            0
        );
    }
    else
    {
        findItems(
            cwd,
            "",
            items,
            &count,
            0,
            1
        );
    }

    /*
     * Sort results.
     */
    qsort(
        items,
        (size_t)count,
        sizeof(items[0]),
        compareStrings
    );

    printf("\n");

    if (typeChoice == 1)
    {
        printf("==============================================================\n");
        printf("                    SELECT FOLDER\n");
        printf("==============================================================\n\n");

        printf("Folders Found:\n");
    }
    else
    {
        printf("==============================================================\n");
        printf("                     SELECT FILE\n");
        printf("==============================================================\n\n");

        printf("Files Found:\n");
    }

    printf("--------------------------------------------------------------\n");

    if (count == 0)
    {
        if (typeChoice == 1)
            printf("No folders found.\n");
        else
            printf("No files found.\n");

        pressEnter();
        return;
    }

    /*
     * Display items.
     */
    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n",
               i + 1,
               items[i]);
    }

    printf("\n0. Cancel\n");

    if (typeChoice == 1)
        printf("\nEnter Folder Number : ");
    else
        printf("\nEnter File Number : ");

    if (!fgets(input,
               sizeof(input),
               stdin))
    {
        return;
    }

    end = NULL;

    long number = strtol(input, &end, 10);

    while (*end == ' ' ||
           *end == '\t' ||
           *end == '\n')
    {
        end++;
    }

    if (end == input ||
        *end != '\0' ||
        number < 0 ||
        number > count)
    {
        printf("\n✗ Invalid item number.\n");
        pressEnter();
        return;
    }

    if (number == 0)
        return;

    /*
     * Build complete path.
     */
    char path[PATH_MAX];

    snprintf(
        path,
        sizeof(path),
        "%s/%s",
        cwd,
        items[number - 1]
    );

    struct stat st;

    if (lstat(path, &st) == -1)
    {
        perror("lstat");
        pressEnter();
        return;
    }

    /*
     * Get basename.
     */
    const char *name =
        strrchr(items[number - 1], '/');

    if (name)
        name++;
    else
        name = items[number - 1];

    /*
     * Get parent location.
     */
    char location[PATH_MAX];

    snprintf(
        location,
        sizeof(location),
        "%s",
        path
    );

    char *slash =
        strrchr(location, '/');

    if (slash)
        *slash = '\0';

    /*
     * Current permissions.
     */
    char permissions[11];

    permissionString(
        st.st_mode,
        permissions
    );

    printf("\n");
    printf("==============================================================\n");
    printf("                     PERMISSION DETAILS\n");
    printf("==============================================================\n\n");

    printf("Selected Item : %s\n\n",
           name);

    if (S_ISDIR(st.st_mode))
    {
        printf("Type                : Folder\n");
    }
    else
    {
        printf("Type                : File\n");
    }

    printf("Location            : %s\n",
           location);

    printf("Current Permissions : %03o (%s)\n",
           st.st_mode & 0777,
           permissions);

    /*
     * Permission examples.
     */
    printf("\n");
    printf("Permission Examples:\n");
    printf("--------------------------------------------------------------\n");

    printf("777 - rwxrwxrwx - Everyone can read, write, execute\n");
    printf("755 - rwxr-xr-x - Owner full; others read + execute\n");
    printf("700 - rwx------ - Owner full only\n");
    printf("644 - rw-r--r-- - Owner read/write; others read\n");
    printf("600 - rw------- - Owner read/write only\n");

    /*
     * Permission symbols.
     */
    printf("\nPermission Symbols:\n");
    printf("--------------------------------------------------------------\n");

    printf("r = Read\n");
    printf("w = Write\n");
    printf("x = Execute\n");
    printf("- = No permission\n");

    /*
     * Ask for new permission.
     */
    printf("\nEnter New Permission (777/755/700/644/600) : ");

    char newPermission[20];

    if (!fgets(newPermission,
               sizeof(newPermission),
               stdin))
    {
        return;
    }

    newPermission[
        strcspn(newPermission, "\n")
    ] = '\0';

    if (!validPermission(newPermission))
    {
        printf("\n✗ Invalid permission.\n");
        printf("Please enter a 3-digit value using digits 0-7.\n");
        pressEnter();
        return;
    }

    /*
     * Convert permission to mode.
     */
    mode_t newMode =
        toMode(newPermission);

    /*
     * Change permission.
     */
    if (chmod(path, newMode) == -1)
    {
        perror("chmod");
        pressEnter();
        return;
    }

    /*
     * Read updated permissions.
     */
    struct stat updated;

    if (lstat(path, &updated) == -1)
    {
        perror("lstat");
        pressEnter();
        return;
    }

    permissionString(
        updated.st_mode,
        permissions
    );

    printf("\n");
    printf("✓ Permission changed successfully.\n\n");

    printf("Name        : %s\n",
           name);

    printf("Permission  : %03o (%s)\n",
           updated.st_mode & 0777,
           permissions);

    printf("Location    : %s\n",
           location);

    pressEnter();
}
