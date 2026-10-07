#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "tree_view.h"
#define MAX_ITEMS 1000

/* -----------------------------------------------------------
   Wait for Enter
   ----------------------------------------------------------- */
static void pressEnter(void)
{
    printf("\nPress Enter to continue...");
    getchar();
}

/* -----------------------------------------------------------
   Structure for directory entries
   ----------------------------------------------------------- */
typedef struct
{
    char name[NAME_MAX + 1];
    int isDirectory;
} TreeItem;

/* -----------------------------------------------------------
   Compare names alphabetically
   ----------------------------------------------------------- */
static int compareItems(const void *a, const void *b)
{
    const TreeItem *itemA = (const TreeItem *)a;
    const TreeItem *itemB = (const TreeItem *)b;

    /*
       Folders are displayed before files.
    */
    if (itemA->isDirectory != itemB->isDirectory)
    {
        return itemB->isDirectory - itemA->isDirectory;
    }

    return strcasecmp(itemA->name, itemB->name);
}

/* -----------------------------------------------------------
   Print the directory tree
   ----------------------------------------------------------- */
static void printTree(
    const char *path,
    const char *prefix,
    int *folderCount,
    int *fileCount)
{
    DIR *dir = opendir(path);

    if (dir == NULL)
    {
        return;
    }

    TreeItem items[MAX_ITEMS];
    int count = 0;

    struct dirent *entry;

    /* Read directory contents */
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (count >= MAX_ITEMS)
        {
            break;
        }

        char fullPath[PATH_MAX];

        snprintf(
            fullPath,
            sizeof(fullPath),
            "%s/%s",
            path,
            entry->d_name
        );

        struct stat info;

        if (lstat(fullPath, &info) != 0)
        {
            continue;
        }

        strncpy(
            items[count].name,
            entry->d_name,
            sizeof(items[count].name) - 1
        );

        items[count].name[sizeof(items[count].name) - 1] = '\0';

        if (S_ISDIR(info.st_mode))
        {
            items[count].isDirectory = 1;
        }
        else
        {
            items[count].isDirectory = 0;
        }

        count++;
    }

    closedir(dir);

    /* Sort folders and files */
    qsort(
        items,
        count,
        sizeof(TreeItem),
        compareItems
    );

    /* Print each item */
    for (int i = 0; i < count; i++)
    {
        int last = (i == count - 1);

        printf("%s", prefix);

        if (last)
        {
            printf("└── ");
        }
        else
        {
            printf("├── ");
        }

        char fullPath[PATH_MAX];

        snprintf(
            fullPath,
            sizeof(fullPath),
            "%s/%s",
            path,
            items[i].name
        );

        if (items[i].isDirectory)
        {
            printf("📁 %s\n", items[i].name);

            (*folderCount)++;

            char newPrefix[PATH_MAX];

            if (last)
            {
                snprintf(
                    newPrefix,
                    sizeof(newPrefix),
                    "%s    ",
                    prefix
                );
            }
            else
            {
                snprintf(
                    newPrefix,
                    sizeof(newPrefix),
                    "%s│   ",
                    prefix
                );
            }

            printTree(
                fullPath,
                newPrefix,
                folderCount,
                fileCount
            );
        }
        else
        {
            printf("📄 %s\n", items[i].name);

            (*fileCount)++;
        }
    }
}

/* -----------------------------------------------------------
   TREE VIEW
   ----------------------------------------------------------- */
void treeView(void)
{
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        pressEnter();
        return;
    }

    /*
       Find workspace directory.

       Example:
       /home/charan/FileMaster3/workspace
    */
    char workspace[PATH_MAX];

    char *workspacePosition = strstr(cwd, "/workspace");

    if (workspacePosition != NULL)
    {
        size_t length =
            (size_t)(workspacePosition - cwd)
            + strlen("/workspace");

        if (length >= sizeof(workspace))
        {
            printf("\nWorkspace path is too long.\n");
            pressEnter();
            return;
        }

        memcpy(workspace, cwd, length);
        workspace[length] = '\0';
    }
    else
    {
        /*
           If already running from workspace,
           use current directory.
        */
        snprintf(
            workspace,
            sizeof(workspace),
            "%s",
            cwd
        );
    }

    int folderCount = 0;
    int fileCount = 0;

    printf("\n");
    printf("┌──────────────────────────────────────────────┐\n");
    printf("│                  TREE VIEW                   │\n");
    printf("├──────────────────────────────────────────────┤\n");

    /*
       Root workspace
    */
    printf("│ 📁 workspace                                │\n");

    /*
       Print contents
    */
    printTree(
        workspace,
        "│ ",
        &folderCount,
        &fileCount
    );

    printf("├──────────────────────────────────────────────┤\n");

    printf(
        "│ 📁 Folders : %-29d│\n",
        folderCount
    );

    printf(
        "│ 📄 Files   : %-29d│\n",
        fileCount
    );

    printf("└──────────────────────────────────────────────┘\n");

    pressEnter();
}
