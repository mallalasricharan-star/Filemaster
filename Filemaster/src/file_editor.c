#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "file_editor.h"

#define MAX_ITEMS 1000


/* ============================================================
   COMMON FUNCTIONS
   ============================================================ */

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
   FIND FILES RECURSIVELY
   ============================================================ */

static void findFiles(const char *base,
                      const char *rel,
                      char files[][PATH_MAX],
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

        if (S_ISREG(st.st_mode))
        {
            snprintf(files[*count],
                     PATH_MAX,
                     "%s",
                     child);

            (*count)++;
        }
        else if (S_ISDIR(st.st_mode))
        {
            findFiles(full,
                      child,
                      files,
                      count);
        }
    }

    closedir(dir);
}


/* ============================================================
   CHOOSE FILE
   ============================================================ */

static int chooseFile(char path[PATH_MAX],
                      const char *title,
                      const char *action)
{
    char cwd[PATH_MAX];
    char files[MAX_ITEMS][PATH_MAX];
    char input[100];

    int count = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return 0;

    findFiles(cwd,
              "",
              files,
              &count);

    qsort(files,
          (size_t)count,
          sizeof(files[0]),
          compareStrings);

    printf("\n");
    printf("==============================================================\n");
    printf("%s\n", title);
    printf("==============================================================\n\n");

    printf("Current Directory : %s\n\n", cwd);

    printf("Files:\n");
    printf("--------------------------------------------------------------\n");

    if (count == 0)
    {
        printf("No files found.\n");
        pressEnter();
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n",
               i + 1,
               files[i]);
    }

    printf("\n0. Cancel\n");

    printf("\nEnter File Number to %s : ",
           action);

    if (!fgets(input, sizeof(input), stdin))
        return 0;

    char *end;
    long number = strtol(input, &end, 10);

    if (end == input ||
        number < 0 ||
        number > count)
    {
        printf("\n✗ Invalid file number.\n");
        pressEnter();
        return 0;
    }

    if (number == 0)
        return 0;

    snprintf(path,
             PATH_MAX,
             "%s/%s",
             cwd,
             files[number - 1]);

    return 1;
}


/* ============================================================
   CREATE FILE
   ============================================================ */

void createFile(void)
{
    char cwd[PATH_MAX];
    char name[PATH_MAX];
    char finalName[PATH_MAX];
    char fullPath[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
        return;

    printf("\n");
    printf("==============================================================\n");
    printf("                       CREATE FILE\n");
    printf("==============================================================\n\n");

    printf("Current Directory : %s\n\n", cwd);

    printf("Enter File Name : ");

    if (!fgets(name, sizeof(name), stdin))
        return;

    name[strcspn(name, "\n")] = '\0';

    if (!validName(name))
    {
        printf("\n✗ Invalid file name.\n");
        pressEnter();
        return;
    }

    /*
       If the user does not provide an extension,
       automatically add .txt
    */
    if (strchr(name, '.') == NULL)
    {
        snprintf(finalName,
                 sizeof(finalName),
                 "%s.txt",
                 name);
    }
    else
    {
        snprintf(finalName,
                 sizeof(finalName),
                 "%s",
                 name);
    }

    snprintf(fullPath,
             sizeof(fullPath),
             "%s/%s",
             cwd,
             finalName);

    struct stat st;

    if (lstat(fullPath, &st) == 0)
    {
        printf("\n✗ A file or folder with this name already exists.\n");
        pressEnter();
        return;
    }

    FILE *file = fopen(fullPath, "w");

    if (!file)
    {
        perror("fopen");
        pressEnter();
        return;
    }

    fclose(file);

    printf("\n✓ File created successfully.\n\n");

    printf("File Name : %s\n", finalName);
    printf("Location  : %s\n", cwd);

    pressEnter();
}


/* ============================================================
   WRITE TEXT TO FILE
   ============================================================ */

void writeTextToFile(void)
{
    char path[PATH_MAX];

    if (!chooseFile(path,
                    "                     WRITE TEXT TO FILE",
                    "write"))
    {
        return;
    }

    FILE *file = fopen(path, "w");

    if (!file)
    {
        perror("fopen");
        pressEnter();
        return;
    }

    printf("\n==============================================================\n");
    printf("                       WRITE TEXT\n");
    printf("==============================================================\n\n");

    printf("File : %s\n\n", path);

    printf("Enter your text below.\n");
    printf("Type END on a new line to finish.\n\n");

    char line[1024];

    while (1)
    {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin))
            break;

        if (strcmp(line, "END\n") == 0 ||
            strcmp(line, "END") == 0)
        {
            break;
        }

        fputs(line, file);
    }

    fclose(file);

    char fileName[PATH_MAX];
    char location[PATH_MAX];

    snprintf(fileName,
             sizeof(fileName),
             "%s",
             path);

    char *slash = strrchr(fileName, '/');

    if (slash)
    {
        *slash = '\0';

        snprintf(location,
                 sizeof(location),
                 "%s",
                 fileName);

        printf("\n✓ Text saved successfully.\n\n");
        printf("File : %s\n", slash + 1);
        printf("Location : %s\n", location);
    }
    else
    {
        printf("\n✓ Text saved successfully.\n");
        printf("File : %s\n", path);
    }

    pressEnter();
}


/* ============================================================
   PREVIEW FILE
   ============================================================ */

void previewFile(void)
{
    char path[PATH_MAX];

    if (!chooseFile(path,
                    "                         PREVIEW FILE",
                    "preview"))
    {
        return;
    }

    FILE *file = fopen(path, "r");

    if (!file)
    {
        perror("fopen");
        pressEnter();
        return;
    }

    printf("\n");
    printf("==============================================================\n");
    printf("                         FILE PREVIEW\n");
    printf("==============================================================\n\n");

    printf("File : %s\n\n", path);

    printf("--------------------------------------------------------------\n");

    char line[1024];
    int lineNumber = 1;
    int hasContent = 0;

    while (fgets(line, sizeof(line), file))
    {
        printf("%4d | %s",
               lineNumber,
               line);

        lineNumber++;
        hasContent = 1;
    }

    if (!hasContent)
    {
        printf("File is empty.\n");
    }

    printf("--------------------------------------------------------------\n");

    fclose(file);

    printf("\n✓ File preview displayed successfully.\n");

    pressEnter();
}
