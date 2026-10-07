#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "main.h"
#include "menu.h"
#include "directory_operations.h"
#include "file_editor.h"
#include "file_operations.h"
#include "search.h"
#include "permission.h"
#include "tree_view.h"

static int readMenuChoice(void)
{
    char input[100];
    char *endptr;
    long value;

    while (1)
    {
        printf("Enter your choice : ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            return 17;

        input[strcspn(input, "\n")] = '\0';

        if (input[0] == '\0')
        {
            printf("\n✗ Invalid choice. Please enter a number from 1 to 17.\n\n");
            continue;
        }

        errno = 0;
        value = strtol(input, &endptr, 10);

        while (*endptr == ' ' || *endptr == '\t')
            endptr++;

        if (errno != 0 || endptr == input || *endptr != '\0' || value < 1 || value > 17)
        {
            printf("\n✗ Invalid choice. Please enter a number from 1 to 17.\n\n");
            continue;
        }

        return (int)value;
    }
}

void runFileMaster(void)
{
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        perror("getcwd");
        return;
    }

    /* The executable is expected to be started from the project root. */
    if (chdir("workspace") == -1)
    {
        fprintf(stderr, "✗ Could not open workspace directory.\n");
        return;
    }

    while (1)
    {
        char currentPath[PATH_MAX];

        displayMenu();

        if (getcwd(currentPath, sizeof(currentPath)) != NULL)
            printf("Current Directory : %s\n\n", currentPath);

        int choice = readMenuChoice();

        switch (choice)
        {
            case 1:  directoryNavigation(); break;
            case 2:  listFiles(); break;
            case 3:  createFolder(); break;
            case 4:  openFolder(); break;
            case 5:  createFile(); break;
            case 6:  writeTextToFile(); break;
            case 7:  previewFile(); break;
            case 8:  deleteFile(); break;
            case 9:  deleteFolder(); break;
            case 10: copyFile(); break;
            case 11: moveFile(); break;
            case 12: renameFile(); break;
            case 13: searchFile(); break;
            case 14: fileInformation(); break;
            case 15: treeView(); break;
            case 16: changePermissions(); break;
            case 17:
                printf("\n==============================================================\n");
                printf("                 Thank you for using FileMaster!\n");
                printf("==============================================================\n\n");
                printf("Exiting FileMaster...\n");
                return;
        }
    }
}

int main(void)
{
    runFileMaster();
    return 0;
}
