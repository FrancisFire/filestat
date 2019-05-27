#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <linux/limits.h>
#include <errno.h>
#include "../include/scan.h"
#include "../include/main.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "../include/tree.h"

ScanInfo scan_info = {0, 0, 0, 0, 0, 0, 0};

RecordNode *readOutputFile(FILE *, RecordNode *);
RecordNode *readInputFile(FILE *, RecordNode *);
RecordNode *analisiSingolaRiga(char *, RecordNode *);
RecordNode *scanFilePath(char *, int, int, RecordNode *);
RecordNode *addFileAnalisis(struct stat *, char *, RecordNode *);
void printOnFile(RecordNode *node, FILE *output);

int startScan(FILE *input, FILE *output)
{
    RecordNode *tree = malloc(sizeof(RecordNode));
    tree = emptyNode();
    tree = readOutputFile(output, tree);
    tree = readInputFile(input, tree);
    printf("\nEffettuo la stampa dell'albero\n");
    printInOrder(tree);
    printf("###\n");
    printOnFile(tree, output);
    freeTree(tree);
}

RecordNode *readOutputFile(FILE *output, RecordNode *tree)
{
    int t = 2;
    char *line = (char *)calloc(t, sizeof(char));
    char *currentPath = (char *)malloc(sizeof(char));
    char *temp_line = (char *)calloc(t, sizeof(char));
    while (fgets(temp_line, t * sizeof(char), output))
    {
        strcat(line, temp_line);
        if (strchr(line, '\n'))
        {
            strtok(line, "\r\n");

            if (line[0] == '#' && line[1] == ' ')
            {
                //Ho trovato un path
                currentPath = (char *)realloc(currentPath, strlen(line) * sizeof(char));
                strcpy(currentPath, strtok(line + 2, " "));
            }
            else if (line[0] != '#' && line[1] != '#' && line[2] != '#')
            {
                //Ho trovato un record
                tree = addRecordByPath(tree, currentPath, line);
            }
            //### è ignorato
            free(line);
            line = (char *)calloc(t, sizeof(char));
        }
        else
        {
            line = (char *)realloc(line, strlen(line) + t);
        }
    };
    free(line);
    free(currentPath);
    free(temp_line);
    return tree;
}
void increaseMonitorati()
{
    scan_info.nr_monitorati++;
};
void increaseLink()
{
    scan_info.nr_link++;
}
void increaseDirectory()
{
    scan_info.nr_directory++;
}

void updateDimMedia()
{
    scan_info.dim_media = scan_info.dim_totale / scan_info.nr_monitorati;
}

void updateDimMax(int data)
{
    if (data > scan_info.dim_max)
    {
        scan_info.dim_max = data;
    }
};
void updateDimMin(int data)
{
    if (data < scan_info.dim_min)
    {
        scan_info.dim_min = data;
    }
}
void increaseDimTotale(int data)
{
    scan_info.dim_totale = scan_info.dim_totale + data;
    updateDimMedia();
    updateDimMax(data);
    updateDimMin(data);
}

RecordNode *readInputFile(FILE *input, RecordNode *tree)
{
    int t = 2;
    char *line = (char *)calloc(t, sizeof(char));
    char *temp_line = (char *)calloc(t, sizeof(char));
    while (fgets(temp_line, t * sizeof(char), input))
    {
        strcat(line, temp_line);
        if (strchr(line, '\n'))
        {
            tree = analisiSingolaRiga(strtok(line, "\r\n"), tree);
            free(line);
            line = (char *)calloc(t, sizeof(char));
        }
        else
        {
            line = (char *)realloc(line, strlen(line) + t);
        }
    };
    tree = analisiSingolaRiga(line, tree);
    free(line);
    free(temp_line);
    return tree;
}

RecordNode *analisiSingolaRiga(char *riga, RecordNode *tree)
{
    char *path = (char *)calloc(strlen(riga), sizeof(char));
    int isR = 0;
    int isL = 0;
    int pathRead = 0;
    char *token;
    for (token = strtok(riga, " "); token; token = strtok(NULL, " "))
    {
        if (strcmp(token, "r") == 0)
        {
            isR = 1;
        }
        else if (strcmp(token, "l") == 0)
        {
            isL = 1;
        }
        else if (!pathRead)
        {
            strcpy(path, token);
            pathRead = 1;
        }
    }

    tree = scanFilePath(path, isR, isL, tree);
    free(path);
    return tree;
}

RecordNode *scanFilePath(char *path, int isR, int isL, RecordNode *tree)
{
    printf("\nPath: %s\n", path);
    printf("R: %d\n", isR);
    printf("L: %d\n", isL);
    struct stat *currentStat = (struct stat *)malloc(sizeof(struct stat));
    if (isL == 1)
    {

        // printf("Del link analizzo il file referenziato\n");
        if (stat(path, currentStat) < 0)
        {
            return tree;
        }
    }
    else if (isL == 0)
    {
        //  printf("Del link analizzo il file link\n");
        if (lstat(path, currentStat) < 0)
        {
            return tree;
        }
    }

    //Analisi effettiva del file e scrittura su albero

    char buf[PATH_MAX];
    char *res = realpath(path, buf);
    if (!res)
    {
        perror("realpath");
        exit(EXIT_FAILURE);
    }
    tree = addFileAnalisis(currentStat, path, tree);

    if (isR && S_ISDIR(currentStat->st_mode))
    {
        //  printf("IL PERCORSO: %s E' UNA DIRECTORY IN CUI ENTRARE RICORSIVAMENTE\n", path);

        DIR *dir;
        struct dirent *entry;
        dir = opendir(path);
        while (entry = readdir(dir))
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char *copy = (char *)calloc(strlen(path) + strlen(entry->d_name) + 2, sizeof(char));
            strcpy(copy, path);
            if (copy[strlen(path) - 1] != '/')
            {
                strcat(copy, "/");
            }
            strcat(copy, entry->d_name);
            tree = scanFilePath(copy, isR, isL, tree);
            free(copy);
        }
        closedir(dir);
    }

    free(currentStat);
    return tree;
}

RecordNode *addFileAnalisis(struct stat *currentStat, char *path, RecordNode *curTree)
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);

    timeinfo = localtime(&rawtime);
    char dinfo = S_ISDIR(currentStat->st_mode) ? 'd' : '-';
    char irusr = currentStat->st_mode & S_IRUSR ? 'r' : '-';
    char iwusr = currentStat->st_mode & S_IWUSR ? 'w' : '-';
    char ixusr = currentStat->st_mode & S_IXUSR ? 'x' : '-';
    char irgrp = currentStat->st_mode & S_IRGRP ? 'r' : '-';
    char iwgrp = currentStat->st_mode & S_IWGRP ? 'w' : '-';
    char ixgrp = currentStat->st_mode & S_IXGRP ? 'x' : '-';
    char iroth = currentStat->st_mode & S_IROTH ? 'r' : '-';
    char iwoth = currentStat->st_mode & S_IWOTH ? 'w' : '-';
    char ixoth = currentStat->st_mode & S_IXOTH ? 'x' : '-';
    struct passwd *pwsUID = getpwuid(currentStat->st_uid);
    struct group *grpGID = getgrgid(currentStat->st_gid);
    char size[21];
    sprintf(size, "%ld", currentStat->st_size);
    char *record = malloc((200 + strlen(pwsUID->pw_name) + strlen(grpGID->gr_name) + strlen(size)) * sizeof(char));
    
    sprintf(record, "data - %s | uid - %s | gid - %s | dim - %s| perm - %c%c%c%c%c%c%c%c%c%c | acc - %s | change - %s | mod - %s | nlink - %ld",
            strtok(asctime(timeinfo), "\n"),
            pwsUID->pw_name,
            grpGID->gr_name,
            size,
            dinfo,
            irusr,
            iwusr,
            ixusr,
            irgrp,
            iwgrp,
            ixgrp,
            iroth,
            iwoth,
            ixoth,
            ctime(&currentStat->st_atime), ctime(&currentStat->st_mtime), ctime(&currentStat->st_ctime), currentStat->st_nlink);
    curTree = addRecordByPath(curTree, path, strtok(record, "\r\n"));
    free(record);
    return curTree;
}

/*void printOnFile(RecordNode *node, FILE *output)
{
    ftruncate(fileno(output), 0);
    if (isEmpty(node))
    {
        return;
    }
    else
    {
        if (node->isPath)
        {
            fputs("# ", output);
            //printf("# ");
        }
        fputs(output,"%s\n", node->value);
        //printf("%s\n", node->value);
        if (node->nextRecord == NULL)
        {
            fputs(output, "###\n",NULL);
            //printf("###\n");
        }
        printInOrder(node->nextRecord);
        printInOrder(node->nextPath);
    }
}*/
