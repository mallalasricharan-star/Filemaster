#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "directory_operations.h"

#define MAX_ITEMS 1000

static void pressEnter(void)
{
    printf("\nPress Enter to continue...");
    getchar();
}

static int validName(const char *name)
{
    return name != NULL &&
           name[0] != '\0' &&
           strchr(name, '/') == NULL &&
           strcmp(name, ".") != 0 &&
           strcmp(name, "..") != 0;
}

static int insideWorkspace(const char *path)
{
    char resolved[PATH_MAX];
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return 0;

    char *p = strstr(cwd, "/workspace");

    if (p == NULL)
        return 0;

    size_t len = (size_t)(p - cwd) + strlen("/workspace");

    char root[PATH_MAX];

    if (len >= sizeof(root))
        return 0;

    memcpy(root, cwd, len);
    root[len] = '\0';

    if (realpath(path, resolved) == NULL)
        return 0;

    return strcmp(resolved, root) == 0 ||
           (strncmp(resolved, root, strlen(root)) == 0 &&
            resolved[strlen(root)] == '/');
}

static void recursiveItems(const char *base,
                           const char *rel,
                           char items[][PATH_MAX],
                           int *count,
                           int wantDirs,
                           int wantFiles)
{
    DIR *dir = opendir(base);

    if (!dir || *count >= MAX_ITEMS)
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

        char full[PATH_MAX];
        char child[PATH_MAX];

        snprintf(full,
                 sizeof(full),
                 "%s/%s",
                 base,
                 entry->d_name);

        snprintf(child,
                 sizeof(child),
                 "%s%s%s",
                 rel,
                 rel[0] ? "/" : "",
                 entry->d_name);

        struct stat st;

        if (lstat(full, &st) == -1)
            continue;

        if ((wantDirs && S_ISDIR(st.st_mode)) ||
            (wantFiles && S_ISREG(st.st_mode)))
        {
            snprintf(items[*count],
                     PATH_MAX,
                     "%s",
                     child);

            (*count)++;
        }

        if (S_ISDIR(st.st_mode))
        {
            recursiveItems(full,
                           child,
                           items,
                           count,
                           wantDirs,
                           wantFiles);
        }
    }

    closedir(dir);
}

static int compareStrings(const void *a, const void *b)
{
    const char *sa = a;
    const char *sb = b;

    return strcmp(sa, sb);
}


/* ============================================================
   DIRECTORY NAVIGATION
   ============================================================ */

void directoryNavigation(void)
{
    char cwd[PATH_MAX];
    char input[PATH_MAX];
    char target[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return;

    printf("\n");
    printf("==============================================================\n");
    printf("                 DIRECTORY NAVIGATION\n");
    printf("==============================================================\n\n");

    printf("Current Directory :\n");
    printf("%s\n\n", cwd);

    printf("Enter folder name (or .. for parent, 0 to cancel): ");

    if (!fgets(input, sizeof(input), stdin))
        return;

    input[strcspn(input, "\n")] = '\0';

    if (strcmp(input, "0") == 0)
    {
        pressEnter();
        return;
    }

    /* Go to parent directory */
    if (strcmp(input, "..") == 0)
    {
        char parent[PATH_MAX];

        snprintf(parent,
                 sizeof(parent),
                 "%s",
                 cwd);

        char *slash = strrchr(parent, '/');

        if (slash)
            *slash = '\0';

        if (!insideWorkspace(parent) ||
            strcmp(parent, cwd) == 0)
        {
            printf("\n✗ You cannot leave the workspace.\n");
            pressEnter();
            return;
        }

        if (chdir(parent) == -1)
        {
            perror("chdir");
        }
        else
        {
            printf("\n✓ Directory changed successfully.\n\n");
            printf("Current Directory :\n");
            printf("%s\n", parent);
        }

        pressEnter();
        return;
    }

    if (!validName(input))
    {
        printf("\n✗ Invalid folder name.\n");
        pressEnter();
        return;
    }

    snprintf(target,
             sizeof(target),
             "%s/%s",
             cwd,
             input);

    struct stat st;

    if (lstat(target, &st) == -1 ||
        !S_ISDIR(st.st_mode))
    {
        printf("\n✗ Folder not found.\n");
        pressEnter();
        return;
    }

    if (!insideWorkspace(target))
    {
        printf("\n✗ You cannot leave the workspace.\n");
        pressEnter();
        return;
    }

    if (chdir(target) == -1)
    {
        perror("chdir");
        pressEnter();
        return;
    }

    printf("\n✓ Directory changed successfully.\n\n");
    printf("Current Directory :\n");
    printf("%s\n", target);

    pressEnter();
}


/* ============================================================
   LIST FILES AND FOLDERS
   ============================================================ */

void listFiles(void)
{
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return;

    char folders[MAX_ITEMS][PATH_MAX];
    char files[MAX_ITEMS][PATH_MAX];

    int folderCount = 0;
    int fileCount = 0;

    DIR *dir = opendir(cwd);

    if (!dir)
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char fullPath[PATH_MAX];

        snprintf(fullPath,
                 sizeof(fullPath),
                 "%s/%s",
                 cwd,
                 entry->d_name);

        struct stat st;

        if (lstat(fullPath, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode))
        {
            if (folderCount < MAX_ITEMS)
            {
                snprintf(folders[folderCount],
                         PATH_MAX,
                         "%s",
                         entry->d_name);

                folderCount++;
            }
        }
        else if (S_ISREG(st.st_mode))
        {
            if (fileCount < MAX_ITEMS)
            {
                snprintf(files[fileCount],
                         PATH_MAX,
                         "%s",
                         entry->d_name);

                fileCount++;
            }
        }
    }

    closedir(dir);

    /* Sort alphabetically */
    qsort(folders,
          (size_t)folderCount,
          sizeof(folders[0]),
          compareStrings);

    qsort(files,
          (size_t)fileCount,
          sizeof(files[0]),
          compareStrings);

    /* Header */
    printf("\n");
    printf("==============================================================\n");
    printf("                 LIST FILES AND FOLDERS\n");
    printf("==============================================================\n\n");

    /* Current Directory */
    printf("Current Directory:\n");
    printf("%s\n\n", cwd);

    /* Folders */
    printf("Folders:\n");
    printf("--------------------------------------------------------------\n");

    if (folderCount == 0)
    {
        printf("No folders found.\n");
    }
    else
    {
        for (int i = 0; i < folderCount; i++)
        {
            printf("📁 %s\n", folders[i]);
        }
    }

    /* Files */
    printf("\nFiles:\n");
    printf("--------------------------------------------------------------\n");

    if (fileCount == 0)
    {
        printf("No files found.\n");
    }
    else
    {
        for (int i = 0; i < fileCount; i++)
        {
            printf("📄 %s\n", files[i]);
        }
    }

    /* Summary */
    printf("\n--------------------------------------------------------------\n");
    printf("Total Folders : %d\n", folderCount);
    printf("Total Files   : %d\n", fileCount);
    printf("--------------------------------------------------------------\n");

    pressEnter();
}


/* ============================================================
   CREATE FOLDER
   ============================================================ */

void createFolder(void)
{
    char cwd[PATH_MAX];
    char name[PATH_MAX];
    char path[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return;

    printf("\n");
    printf("==============================================================\n");
    printf("                    CREATE FOLDER\n");
    printf("==============================================================\n\n");

    printf("Current Directory : %s\n\n", cwd);
    printf("Enter Folder Name : ");

    if (!fgets(name, sizeof(name), stdin))
        return;

    name[strcspn(name, "\n")] = '\0';

    if (!validName(name))
    {
        printf("\n✗ Invalid folder name.\n");
        pressEnter();
        return;
    }

    snprintf(path,
             sizeof(path),
             "%s/%s",
             cwd,
             name);

    struct stat st;

    if (lstat(path, &st) == 0)
    {
        printf("\n✗ A file or folder with this name already exists.\n");
        pressEnter();
        return;
    }

    if (mkdir(path, 0755) == -1)
    {
        perror("mkdir");
        pressEnter();
        return;
    }

    printf("\n✓ Folder created successfully.\n\n");
    printf("Folder Name : %s\n", name);
    printf("Location    : %s\n", cwd);

    pressEnter();
}


/* ============================================================
   OPEN FOLDER
   ============================================================ */

void openFolder(void)
{
    char cwd[PATH_MAX];
    char folders[MAX_ITEMS][PATH_MAX];
    char input[100];

    int count = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return;

    DIR *dir = opendir(cwd);

    if (!dir)
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL &&
           count < MAX_ITEMS)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char path[PATH_MAX];

        snprintf(path,
                 sizeof(path),
                 "%s/%s",
                 cwd,
                 entry->d_name);

        struct stat st;

        if (lstat(path, &st) == 0 &&
            S_ISDIR(st.st_mode))
        {
            snprintf(folders[count],
                     PATH_MAX,
                     "%s",
                     entry->d_name);

            count++;
        }
    }

    closedir(dir);

    qsort(folders,
          (size_t)count,
          sizeof(folders[0]),
          compareStrings);

    printf("\n");
    printf("==============================================================\n");
    printf("                     OPEN FOLDER\n");
    printf("==============================================================\n\n");

    printf("Current Directory : %s\n\n", cwd);

    printf("Folders:\n\n");

    if (count == 0)
    {
        printf("No folders found.\n");
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            printf("%d. %s\n",
                   i + 1,
                   folders[i]);
        }
    }

    printf("\n0. Cancel\n");
    printf("\nEnter Folder Number : ");

    if (!fgets(input, sizeof(input), stdin))
        return;

    char *end;
    long n = strtol(input, &end, 10);

    if (end == input ||
        n < 0 ||
        n > count)
    {
        printf("\n✗ Invalid folder number.\n");
        pressEnter();
        return;
    }

    if (n == 0)
        return;

    char path[PATH_MAX];

    snprintf(path,
             sizeof(path),
             "%s/%s",
             cwd,
             folders[n - 1]);

    if (chdir(path) == -1)
    {
        perror("chdir");
        pressEnter();
        return;
    }

    printf("\n✓ Folder opened successfully.\n\n");
    printf("Current Directory : %s\n", path);

    pressEnter();
}


/* ============================================================
   RECURSIVE DELETE
   ============================================================ */

static int removeRecursive(const char *path)
{
    struct stat st;

    if (lstat(path, &st) == -1)
        return -1;

    if (S_ISDIR(st.st_mode))
    {
        DIR *dir = opendir(path);

        if (!dir)
            return -1;

        struct dirent *entry;

        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            char childPath[PATH_MAX];

            snprintf(childPath,
                     sizeof(childPath),
                     "%s/%s",
                     path,
                     entry->d_name);

            if (removeRecursive(childPath) == -1)
            {
                closedir(dir);
                return -1;
            }
        }

        closedir(dir);

        return rmdir(path);
    }

    return unlink(path);
}


/* ============================================================
   DELETE FOLDER
   ============================================================ */

void deleteFolder(void)
{
    char cwd[PATH_MAX];
    char folders[MAX_ITEMS][PATH_MAX];
    char input[100];

    int count = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return;

    recursiveItems(cwd,
                   "",
                   folders,
                   &count,
                   1,
                   0);

    qsort(folders,
          (size_t)count,
          sizeof(folders[0]),
          compareStrings);

    printf("\n");
    printf("==============================================================\n");
    printf("                     DELETE FOLDER\n");
    printf("==============================================================\n\n");

    printf("Current Directory : %s\n\n", cwd);

    printf("Folders Found:\n\n");

    if (count == 0)
    {
        printf("No folders found.\n");
        pressEnter();
        return;
    }

    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n",
               i + 1,
               folders[i]);
    }

    printf("\n0. Cancel\n");
    printf("\nEnter Folder Number : ");

    if (!fgets(input, sizeof(input), stdin))
        return;

    char *end;
    long n = strtol(input, &end, 10);

    if (end == input ||
        n < 0 ||
        n > count)
    {
        printf("\n✗ Invalid folder number.\n");
        pressEnter();
        return;
    }

    if (n == 0)
        return;

    char path[PATH_MAX];

    snprintf(path,
             sizeof(path),
             "%s/%s",
             cwd,
             folders[n - 1]);

    char *slash = strrchr(folders[n - 1], '/');

    const char *name;

    if (slash)
        name = slash + 1;
    else
        name = folders[n - 1];

    char location[PATH_MAX];

    snprintf(location,
             sizeof(location),
             "%s",
             path);

    slash = strrchr(location, '/');

    if (slash)
        *slash = '\0';

    printf("\nSelected Folder : %s\n", name);

    printf("\nAre you sure you want to delete this folder ");
    printf("and all its contents? (y/n) : ");

    char confirm[10];

    if (!fgets(confirm, sizeof(confirm), stdin))
        return;

    if (confirm[0] != 'y' &&
        confirm[0] != 'Y')
    {
        printf("\n✗ Folder deletion cancelled.\n");
        pressEnter();
        return;
    }

    if (removeRecursive(path) == -1)
    {
        printf("\n✗ Failed to delete folder.\n");
        perror("delete");
    }
    else
    {
        printf("\n✓ Folder and all its contents deleted successfully.\n\n");
        printf("Folder Name : %s\n", name);
        printf("Location    : %s\n", location);
    }

    pressEnter();
}
