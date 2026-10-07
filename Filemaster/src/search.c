
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

static int foundCount = 0;

/* ------------------------------------------------------------
   Check whether an item name matches the search name
   Case-sensitive search
   ------------------------------------------------------------ */
static int nameMatches(const char *itemName,
                        const char *searchName)
{
    /*
     * Exact match.
     *
     * Example:
     *
     * Ossp_copy.txt
     * Ossp_copy.txt
     *
     * Result: MATCH
     */
    if (strcmp(itemName, searchName) == 0)
    {
        return 1;
    }

    /*
     * Search without extension.
     *
     * Example:
     *
     * Search:
     * Ossp_copy
     *
     * Actual file:
     * Ossp_copy.txt
     *
     * Result: MATCH
     */
    const char *dot = strrchr(itemName, '.');

    if (dot != NULL && dot != itemName)
    {
        size_t nameLength =
            (size_t)(dot - itemName);

        /*
         * Search name must have exactly the same
         * number of characters as the filename
         * before the extension.
         */
        if (strlen(searchName) == nameLength)
        {
            char nameWithoutExtension[PATH_MAX];

            if (nameLength >= sizeof(nameWithoutExtension))
            {
                return 0;
            }

            memcpy(
                nameWithoutExtension,
                itemName,
                nameLength
            );

            nameWithoutExtension[nameLength] = '\0';

            /*
             * strcmp() is CASE-SENSITIVE.
             */
            if (strcmp(nameWithoutExtension,
                       searchName) == 0)
            {
                return 1;
            }
        }
    }

    return 0;
}

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

        /*
         * Ignore . and ..
         */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        /*
         * Full path of current item.
         */
        snprintf(
            fullPath,
            sizeof(fullPath),
            "%s/%s",
            currentPath,
            entry->d_name
        );

        /*
         * Relative path from workspace.
         */
        if (relativePath[0] == '\0')
        {
            snprintf(
                relativeItem,
                sizeof(relativeItem),
                "%s",
                entry->d_name
            );
        }
        else
        {
            snprintf(
                relativeItem,
                sizeof(relativeItem),
                "%s/%s",
                relativePath,
                entry->d_name
            );
        }

        /*
         * Get file/folder information.
         */
        if (lstat(fullPath, &st) != 0)
        {
            continue;
        }

        /* ----------------------------------------------------
           Check whether current item matches
           ---------------------------------------------------- */
        if (nameMatches(entry->d_name, searchName))
        {
            /*
             * Folder result
             */
            if (S_ISDIR(st.st_mode))
            {
                foundCount++;

                printf("  [%d] %s [Folder]\n",
                       foundCount,
                       relativeItem);

                printf("      Location : %s\n",
                       currentPath);

                printf("\n");
            }

            /*
             * Regular file result
             */
            else if (S_ISREG(st.st_mode))
            {
                foundCount++;

                printf("  [%d] %s [File]\n",
                       foundCount,
                       relativeItem);

                printf("      Location : %s\n",
                       currentPath);

                printf("\n");
            }
        }

        /* ----------------------------------------------------
           Recursively search inside folders
           ---------------------------------------------------- */
        if (S_ISDIR(st.st_mode))
        {
            searchRecursive(
                fullPath,
                relativeItem,
                searchName
            );
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
     * Get current directory.
     */
    if (getcwd(currentDirectory,
               sizeof(currentDirectory)) == NULL)
    {
        printf("✗ Unable to get current directory.\n");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }

    /*
     * Find /workspace in the current path.
     */
    workspaceStart =
        strstr(currentDirectory, "/workspace");

    if (workspaceStart == NULL)
    {
        printf("✗ Workspace directory not found.\n");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }

    /*
     * Build complete workspace path.
     *
     * Example:
     *
     * /home/charan/Filemaster/Filemaster/workspace
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

        memcpy(
            workspace,
            currentDirectory,
            workspaceLength
        );

        workspace[workspaceLength] = '\0';
    }

    printf("Workspace : %s\n\n", workspace);

    /*
     * Get search name.
     */
    printf("Enter file or folder name to search: ");

    if (fgets(searchName,
              sizeof(searchName),
              stdin) == NULL)
    {
        return;
    }

    /*
     * Remove newline.
     */
    searchName[strcspn(searchName, "\n")] = '\0';

    /*
     * Empty search is not allowed.
     */
    if (strlen(searchName) == 0)
    {
        printf("\n✗ Search name cannot be empty.\n");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }

    printf("\nSearching entire workspace...\n\n");

    printf("--------------------------------------------------------------\n");
    printf("Search Results for: %s\n",
           searchName);
    printf("--------------------------------------------------------------\n\n");

    /*
     * Reset result counter.
     */
    foundCount = 0;

    /*
     * Search from workspace root.
     */
    searchRecursive(
        workspace,
        "",
        searchName
    );

    /*
     * No result.
     */
    if (foundCount == 0)
    {
        printf("No files or folders found matching : %s\n\n",
               searchName);
    }

    printf("--------------------------------------------------------------\n");
    printf("Total results found: %d\n",
           foundCount);
    printf("--------------------------------------------------------------\n");

    printf("\nPress Enter to continue...");
    getchar();
}
