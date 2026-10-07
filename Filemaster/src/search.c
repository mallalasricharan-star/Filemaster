
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

static int foundCount = 0;

/* ------------------------------------------------------------
   Recursive search
   ------------------------------------------------------------ */
static void searchRecursive(const char *currentPath,
                            const char *relativePath,
                            const char *searchName)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(currentPath);

    if (dir == NULL)
    {
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char fullPath[PATH_MAX];
        char relativeItem[PATH_MAX];
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        /* Full path of current item */
        snprintf(fullPath,
                 sizeof(fullPath),
                 "%s/%s",
                 currentPath,
                 entry->d_name);

        /* Relative path from workspace */
        if (relativePath[0] == '\0')
        {
            snprintf(relativeItem,
                     sizeof(relativeItem),
                     "%s",
                     entry->d_name);
        }
        else
        {
            snprintf(relativeItem,
                     sizeof(relativeItem),
                     "%s/%s",
                     relativePath,
                     entry->d_name);
        }

        if (lstat(fullPath, &st) != 0)
        {
            continue;
        }

        /* ----------------------------------------------------
           Matching item
           ---------------------------------------------------- */
        if (strcmp(entry->d_name, searchName) == 0)
        {
            foundCount++;

            if (S_ISDIR(st.st_mode))
            {
                printf("  [%d] %s [Folder]\n",
                       foundCount,
                       relativeItem);
            }
            else if (S_ISREG(st.st_mode))
            {
                printf("  [%d] %s [File]\n",
                       foundCount,
                       relativeItem);
            }

            /*
             * Show COMPLETE LOCATION
             *
             * currentPath is already the full absolute path
             * of the directory containing the item.
             */
            printf("      Location : %s\n", currentPath);
            printf("\n");
        }

        /* ----------------------------------------------------
           Recursively search inside directories
           ---------------------------------------------------- */
        if (S_ISDIR(st.st_mode))
        {
            searchRecursive(fullPath,
                            relativeItem,
                            searchName);
        }
    }

    closedir(dir);
}

/* ------------------------------------------------------------
   Search Files / Folders
   ------------------------------------------------------------ */
void searchFile(void)
{
    char currentDirectory[PATH_MAX];
    char workspace[PATH_MAX];
    char searchName[PATH_MAX];

    char *workspaceStart;

    printf("==============================================================\n");
    printf("                         SEARCH FILES\n");
    printf("==============================================================\n\n");

    /*
     * Get current directory
     */
    if (getcwd(currentDirectory, sizeof(currentDirectory)) == NULL)
    {
        printf("✗ Unable to get current directory.\n");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }

    /*
     * Find /workspace in the current path.
     */
    workspaceStart = strstr(currentDirectory, "/workspace");

    if (workspaceStart == NULL)
    {
        printf("✗ Workspace directory not found.\n");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }

    /*
     * Copy complete workspace path.
     *
     * Example:
     * /home/charan/FileMaster3/workspace
     */
    {
        size_t workspaceLength =
            (size_t)(workspaceStart - currentDirectory)
            + strlen("/workspace");

        if (workspaceLength >= sizeof(workspace))
        {
            printf("✗ Workspace path is too long.\n");
            printf("\nPress Enter to continue...");
            getchar();
            return;
        }

        memcpy(workspace,
               currentDirectory,
               workspaceLength);

        workspace[workspaceLength] = '\0';
    }

    printf("Workspace : %s\n\n", workspace);

    printf("Enter file or folder name to search: ");

    if (fgets(searchName, sizeof(searchName), stdin) == NULL)
    {
        return;
    }

    searchName[strcspn(searchName, "\n")] = '\0';

    if (strlen(searchName) == 0)
    {
        printf("\n✗ Search name cannot be empty.\n");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }

    printf("\nSearching entire workspace...\n\n");

    printf("--------------------------------------------------------------\n");
    printf("Search Results for: %s\n", searchName);
    printf("--------------------------------------------------------------\n\n");

    foundCount = 0;

    /*
     * Search from the ROOT of workspace.
     */
    searchRecursive(workspace, "", searchName);

    printf("--------------------------------------------------------------\n");
    printf("Total results found: %d\n", foundCount);
    printf("--------------------------------------------------------------\n");

    printf("\nPress Enter to continue...");
    getchar();
}
