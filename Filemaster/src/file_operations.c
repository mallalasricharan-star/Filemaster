#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include "file_operations.h"

#define MAX_ITEMS 1000


/* ============================================================
   COMMON FUNCTIONS
   ============================================================ */

static void pressEnter(void)
{
    printf("\nPress Enter to continue...");
    getchar();
}


static int compareStrings(const void *a, const void *b)
{
    const char *sa = a;
    const char *sb = b;

    return strcmp(sa, sb);
}


/* ============================================================
   FIND FILES / FOLDERS RECURSIVELY
   ============================================================ */

static void findItems(const char *base,
                      const char *rel,
                      char items[][PATH_MAX],
                      int *count,
                      int wantDirs,
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

        if ((wantDirs && S_ISDIR(st.st_mode)) ||
            (wantFiles && S_ISREG(st.st_mode)))
        {
            snprintf(items[*count],
                     PATH_MAX,
                     "%s",
                     relativePath);

            (*count)++;
        }

        if (S_ISDIR(st.st_mode))
        {
            findItems(fullPath,
                      relativePath,
                      items,
                      count,
                      wantDirs,
                      wantFiles);
        }
    }

    closedir(dir);
}


/* ============================================================
   GET BASE NAME
   ============================================================ */

static const char *baseName(const char *path)
{
    const char *slash = strrchr(path, '/');

    if (slash)
        return slash + 1;

    return path;
}


/* ============================================================
   GET PARENT DIRECTORY
   ============================================================ */

static void parentOf(const char *full,
                     char *out,
                     size_t size)
{
    snprintf(out,
             size,
             "%s",
             full);

    char *slash = strrchr(out, '/');

    if (slash)
    {
        if (slash == out)
            slash[1] = '\0';
        else
            *slash = '\0';
    }
}


/* ============================================================
   CHOOSE FILE / FOLDER
   ============================================================ */

static int choose(const char *title,
                  const char *label,
                  int wantDirs,
                  int wantFiles,
                  char *out)
{
    char cwd[PATH_MAX];
    char items[MAX_ITEMS][PATH_MAX];
    char input[100];

    int count = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return 0;

    findItems(cwd,
              "",
              items,
              &count,
              wantDirs,
              wantFiles);

    qsort(items,
          (size_t)count,
          sizeof(items[0]),
          compareStrings);

    printf("\n");
    printf("==============================================================\n");
    printf("%s\n", title);
    printf("==============================================================\n\n");

    printf("Current Directory : %s\n\n", cwd);

    printf("%s\n", label);
    printf("--------------------------------------------------------------\n");

    if (count == 0)
    {
        printf("No items found.\n");
        pressEnter();
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n",
               i + 1,
               items[i]);
    }

    printf("\n0. Cancel\n");

    printf("\nEnter Number : ");

    if (!fgets(input, sizeof(input), stdin))
        return 0;

    char *end;
    long number = strtol(input, &end, 10);

    while (*end == ' ' || *end == '\t' || *end == '\n')
        end++;

    if (end == input ||
        *end != '\0' ||
        number < 0 ||
        number > count)
    {
        printf("\n✗ Invalid number.\n");
        pressEnter();
        return 0;
    }

    if (number == 0)
        return 0;

    snprintf(out,
             PATH_MAX,
             "%s/%s",
             cwd,
             items[number - 1]);

    return 1;
}


/* ============================================================
   CHOOSE FILE
   ============================================================ */

static int chooseFile(const char *title,
                       char *out)
{
    return choose(title,
                  "Files Found:",
                  0,
                  1,
                  out);
}


/* ============================================================
   CHOOSE FOLDER
   ============================================================ */

static int chooseFolder(const char *title,
                         char *out)
{
    return choose(title,
                  "Folders Found:",
                  1,
                  0,
                  out);
}


/* ============================================================
   DESTINATION FOLDER
   ============================================================ */

static int destinationFolder(char *out)
{
    return choose("                    SELECT DESTINATION FOLDER",
                  "Folders Found:",
                  1,
                  0,
                  out);
}


/* ============================================================
   GENERATE UNIQUE COPY NAME
   ============================================================ */

static int uniqueCopyName(const char *dest,
                          const char *src,
                          char *out)
{
    char temp[PATH_MAX];
    char stem[PATH_MAX];
    char ext[PATH_MAX];

    const char *name = baseName(src);

    const char *dot = strrchr(name, '.');

    if (dot && dot != name)
    {
        size_t stemLength = (size_t)(dot - name);

        if (stemLength >= sizeof(stem))
            return 0;

        memcpy(stem,
               name,
               stemLength);

        stem[stemLength] = '\0';

        snprintf(ext,
                 sizeof(ext),
                 "%s",
                 dot);
    }
    else
    {
        snprintf(stem,
                 sizeof(stem),
                 "%s",
                 name);

        ext[0] = '\0';
    }

    for (int i = 0; i < 10000; i++)
    {
        if (i == 0)
        {
            snprintf(temp,
                     sizeof(temp),
                     "%s/%s_copy%s",
                     dest,
                     stem,
                     ext);
        }
        else
        {
            snprintf(temp,
                     sizeof(temp),
                     "%s/%s_copy_%d%s",
                     dest,
                     stem,
                     i,
                     ext);
        }

        struct stat st;

        if (lstat(temp, &st) == -1 &&
            errno == ENOENT)
        {
            snprintf(out,
                     PATH_MAX,
                     "%s",
                     temp);

            return 1;
        }
    }

    return 0;
}


/* ============================================================
   COPY REGULAR FILE
   ============================================================ */

static int copyRegularFile(const char *src,
                           const char *dst)
{
    int input = open(src, O_RDONLY);

    if (input == -1)
        return -1;

    int output = open(dst,
                      O_WRONLY |
                      O_CREAT |
                      O_TRUNC,
                      0644);

    if (output == -1)
    {
        close(input);
        return -1;
    }

    char buffer[8192];
    ssize_t bytesRead;

    while ((bytesRead = read(input,
                             buffer,
                             sizeof(buffer))) > 0)
    {
        ssize_t totalWritten = 0;

        while (totalWritten < bytesRead)
        {
            ssize_t bytesWritten =
                write(output,
                      buffer + totalWritten,
                      (size_t)(bytesRead - totalWritten));

            if (bytesWritten == -1)
            {
                close(input);
                close(output);
                return -1;
            }

            totalWritten += bytesWritten;
        }
    }

    close(input);
    close(output);

    if (bytesRead == -1)
        return -1;

    return 0;
}


/* ============================================================
   DELETE FILE
   ============================================================ */

void deleteFile(void)
{
    char path[PATH_MAX];

    if (!chooseFile("                      DELETE FILE",
                    path))
    {
        return;
    }

    const char *name = baseName(path);

    printf("\nSelected File : %s\n", name);

    printf("\nAre you sure you want to delete this file? (y/n) : ");

    char confirm[10];

    if (!fgets(confirm,
                sizeof(confirm),
                stdin))
    {
        return;
    }

    if (confirm[0] != 'y' &&
        confirm[0] != 'Y')
    {
        printf("\n✗ File deletion cancelled.\n");
        pressEnter();
        return;
    }

    if (unlink(path) == -1)
    {
        printf("\n✗ File deletion failed.\n");
        perror("unlink");
        pressEnter();
        return;
    }

    char location[PATH_MAX];

    parentOf(path,
             location,
             sizeof(location));

    printf("\n✓ File deleted successfully.\n\n");
    printf("File Name : %s\n", name);
    printf("Location  : %s\n", location);

    pressEnter();
}


/* ============================================================
   COPY FILE
   ============================================================ */

void copyFile(void)
{
    char source[PATH_MAX];
    char destination[PATH_MAX];
    char newPath[PATH_MAX];

    if (!chooseFile("                        COPY FILE",
                    source))
    {
        return;
    }

    if (!destinationFolder(destination))
        return;

    if (!uniqueCopyName(destination,
                        source,
                        newPath))
    {
        printf("\n✗ Could not create a unique copy name.\n");
        pressEnter();
        return;
    }

    if (copyRegularFile(source,
                        newPath) == -1)
    {
        printf("\n✗ File copy failed.\n");
        perror("copy");
        pressEnter();
        return;
    }

    printf("\n✓ File copied successfully.\n\n");

    printf("Source      : %s\n", source);
    printf("Destination : %s\n", newPath);

    pressEnter();
}


/* ============================================================
   MOVE FILE
   ============================================================ */

void moveFile(void)
{
    char source[PATH_MAX];
    char destination[PATH_MAX];
    char newPath[PATH_MAX];

    if (!chooseFile("                        MOVE FILE",
                    source))
    {
        return;
    }

    if (!destinationFolder(destination))
        return;

    snprintf(newPath,
             sizeof(newPath),
             "%s/%s",
             destination,
             baseName(source));

    struct stat st;

    if (lstat(newPath, &st) == 0)
    {
        printf("\n✗ A file or folder with the destination name already exists.\n");
        pressEnter();
        return;
    }

    if (rename(source,
               newPath) == -1)
    {
        printf("\n✗ File move failed.\n");
        perror("rename");
        pressEnter();
        return;
    }

    printf("\n✓ File moved successfully.\n\n");

    printf("Source      : %s\n", source);
    printf("Destination : %s\n", newPath);

    pressEnter();
}


/* ============================================================
   RENAME FILE
   ============================================================ */

static void renameFileOnly(void)
{
    char path[PATH_MAX];
    char newName[PATH_MAX];
    char newPath[PATH_MAX];

    if (!chooseFile("                     RENAME FILE",
                    path))
    {
        return;
    }

    printf("\nCurrent File Name : %s\n",
           baseName(path));

    printf("\nEnter New File Name : ");

    if (!fgets(newName,
               sizeof(newName),
               stdin))
    {
        return;
    }

    newName[strcspn(newName, "\n")] = '\0';

    if (newName[0] == '\0' ||
        strchr(newName, '/') != NULL ||
        strcmp(newName, ".") == 0 ||
        strcmp(newName, "..") == 0)
    {
        printf("\n✗ Invalid file name.\n");
        pressEnter();
        return;
    }

    char location[PATH_MAX];

    parentOf(path,
             location,
             sizeof(location));

    snprintf(newPath,
             sizeof(newPath),
             "%s/%s",
             location,
             newName);

    struct stat st;

    if (lstat(newPath, &st) == 0)
    {
        printf("\n✗ A file or folder with this name already exists.\n");
        pressEnter();
        return;
    }

    if (rename(path,
               newPath) == -1)
    {
        printf("\n✗ File rename failed.\n");
        perror("rename");
        pressEnter();
        return;
    }

    printf("\n✓ File renamed successfully.\n\n");

    printf("Old Name : %s\n", baseName(path));
    printf("New Name : %s\n", newName);
    printf("Location : %s\n", location);

    pressEnter();
}


/* ============================================================
   RENAME FOLDER
   ============================================================ */

static void renameFolderOnly(void)
{
    char path[PATH_MAX];
    char newName[PATH_MAX];
    char newPath[PATH_MAX];

    if (!chooseFolder("                    RENAME FOLDER",
                      path))
    {
        return;
    }

    printf("\nCurrent Folder Name : %s\n",
           baseName(path));

    printf("\nEnter New Folder Name : ");

    if (!fgets(newName,
               sizeof(newName),
               stdin))
    {
        return;
    }

    newName[strcspn(newName, "\n")] = '\0';

    if (newName[0] == '\0' ||
        strchr(newName, '/') != NULL ||
        strcmp(newName, ".") == 0 ||
        strcmp(newName, "..") == 0)
    {
        printf("\n✗ Invalid folder name.\n");
        pressEnter();
        return;
    }

    char location[PATH_MAX];

    parentOf(path,
             location,
             sizeof(location));

    snprintf(newPath,
             sizeof(newPath),
             "%s/%s",
             location,
             newName);

    struct stat st;

    if (lstat(newPath, &st) == 0)
    {
        printf("\n✗ A file or folder with this name already exists.\n");
        pressEnter();
        return;
    }

    if (rename(path,
               newPath) == -1)
    {
        printf("\n✗ Folder rename failed.\n");
        perror("rename");
        pressEnter();
        return;
    }

    printf("\n✓ Folder renamed successfully.\n\n");

    printf("Old Name : %s\n", baseName(path));
    printf("New Name : %s\n", newName);
    printf("Location : %s\n", location);

    pressEnter();
}


/* ============================================================
   RENAME FILE / FOLDER MENU
   ============================================================ */

void renameFile(void)
{
    char input[50];

    printf("\n");
    printf("==============================================================\n");
    printf("                 RENAME FILES / FOLDERS\n");
    printf("==============================================================\n\n");

    printf("1. Rename File\n");
    printf("2. Rename Folder\n");
    printf("3. Cancel\n");

    printf("\nEnter your choice : ");

    if (!fgets(input,
               sizeof(input),
               stdin))
    {
        return;
    }

    int choice = atoi(input);

    switch (choice)
    {
        case 1:
            renameFileOnly();
            break;

        case 2:
            renameFolderOnly();
            break;

        case 3:
            return;

        default:
            printf("\n✗ Invalid choice.\n");
            pressEnter();
            break;
    }
}


/* ============================================================
   PERMISSION STRING
   ============================================================ */

static void permissionString(mode_t mode,
                             char permissions[11])
{
    permissions[0] = S_ISDIR(mode) ? 'd' : '-';

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';

    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';

    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';

    permissions[10] = '\0';
}


/* ============================================================
   DISPLAY INFORMATION FOR SELECTED ITEM
   ============================================================ */

static void displayItemInformation(const char *path)
{
    struct stat st;

    if (lstat(path, &st) == -1)
    {
        printf("\n✗ Unable to get item information.\n");
        perror("lstat");
        pressEnter();
        return;
    }

    char permissions[11];

    permissionString(st.st_mode,
                     permissions);

    struct passwd *owner =
        getpwuid(st.st_uid);

    struct group *group =
        getgrgid(st.st_gid);

    char timeBuffer[100];

    struct tm timeInfo;

    if (localtime_r(&st.st_mtime, &timeInfo) == NULL)
    {
        strcpy(timeBuffer, "Unknown");
    }
    else
    {
        strftime(timeBuffer,
                 sizeof(timeBuffer),
                 "%d-%m-%Y %H:%M:%S",
                 &timeInfo);
    }

    char location[PATH_MAX];

    parentOf(path,
             location,
             sizeof(location));

    const char *name = baseName(path);

    int isFolder = S_ISDIR(st.st_mode);

    printf("\nSelected %s : %s\n",
           isFolder ? "Folder" : "File",
           name);

    printf("\n");

    printf("==============================================================\n");

    if (isFolder)
        printf("                    FOLDER INFORMATION\n");
    else
        printf("                     FILE INFORMATION\n");

    printf("==============================================================\n\n");

    printf("Name          : %s\n",
           name);

    if (isFolder)
        printf("Type          : Folder\n");
    else if (S_ISREG(st.st_mode))
        printf("Type          : File\n");
    else if (S_ISLNK(st.st_mode))
        printf("Type          : Symbolic Link\n");
    else
        printf("Type          : Other\n");

    printf("Location      : %s\n",
           location);

    printf("Full Path     : %s\n",
           path);

    printf("\n");

    printf("Size          : %lld bytes\n",
           (long long)st.st_size);

    printf("Permissions   : %03o (%s)\n",
           st.st_mode & 0777,
           permissions);

    printf("Owner         : %s\n",
           owner ? owner->pw_name : "Unknown");

    printf("Group         : %s\n",
           group ? group->gr_name : "Unknown");

    printf("Owner UID     : %u\n",
           (unsigned int)st.st_uid);

    printf("Group GID     : %u\n",
           (unsigned int)st.st_gid);

    printf("Hard Links    : %lu\n",
           (unsigned long)st.st_nlink);

    printf("Last Modified : %s\n",
           timeBuffer);

    printf("\n--------------------------------------------------------------\n");

    printf("Permission Meaning:\n\n");

    printf("r = Read\n");
    printf("w = Write\n");
    printf("x = Execute\n");
    printf("- = No permission\n");

    printf("\nPermission Examples:\n\n");

    printf("777 - rwxrwxrwx - Everyone can read, write, execute\n");
    printf("755 - rwxr-xr-x - Owner full; others read + execute\n");
    printf("700 - rwx------ - Owner full only\n");
    printf("644 - rw-r--r-- - Owner read/write; others read\n");
    printf("600 - rw------- - Owner read/write only\n");

    printf("\n");

    if (isFolder)
        printf("✓ Folder information displayed successfully.\n");
    else
        printf("✓ File information displayed successfully.\n");

    pressEnter();
}


/* ============================================================
   VIEW FILE INFORMATION
   FIRST SELECT FOLDER OR FILE
   ============================================================ */

void fileInformation(void)
{
    char input[50];

    printf("\n");
    printf("==============================================================\n");
    printf("                  VIEW FILE INFORMATION\n");
    printf("==============================================================\n\n");

    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("✗ Unable to get current directory.\n");
        pressEnter();
        return;
    }

    printf("Current Directory : %s\n\n",
           cwd);

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
    long choice = strtol(input,
                         &end,
                         10);

    while (*end == ' ' ||
           *end == '\t' ||
           *end == '\n')
    {
        end++;
    }

    if (end == input ||
        *end != '\0')
    {
        printf("\n✗ Invalid choice.\n");
        pressEnter();
        return;
    }

    if (choice == 0)
        return;

    /* ========================================================
       FOLDER
       ======================================================== */

    if (choice == 1)
    {
        char folderPath[PATH_MAX];

        if (!chooseFolder("                    SELECT FOLDER",
                          folderPath))
        {
            return;
        }

        displayItemInformation(folderPath);
        return;
    }

    /* ========================================================
       FILE
       ======================================================== */

    if (choice == 2)
    {
        char filePath[PATH_MAX];

        if (!chooseFile("                      SELECT FILE",
                        filePath))
        {
            return;
        }

        displayItemInformation(filePath);
        return;
    }

    printf("\n✗ Invalid choice. Please select 1, 2, or 0.\n");

    pressEnter();
}
