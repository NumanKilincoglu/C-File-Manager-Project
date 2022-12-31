#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_TOK 32
#define BUFFER 64
#define MAX_IN 64

#define AVG_LENGTH 64
#define FILE_COUNT 10

pthread_mutex_t mutexClientId;

int clientCount = 0;
int portIdx = 0;
int tokenCount = 0;
int clientId = 0;
int threadCount = 5;
int fileCount = 10;
int portCount = 5;
void *status;

char *listenPort = "/tmp/file_manager_named_pipe";
char *commPort;

char commandInput[BUFFER];
char *token[MAX_TOK];
char *portList[5];
char fileList[FILE_COUNT][50];

pthread_t threadList[5];
pthread_t listenThread;

int task = 0;
int done = 0;

void deleteFile(char *fileName)
{
    char str[25];
    strcpy(str, fileName);
    printf("File deleted! %s\n", str);
    if (remove(str) == 0)
    {
        printf("File deleted!\n");
    }
    else
    {
        printf("Something wrong happened!\n");
    }
}

void readFile(char *fileName)
{
    FILE *file;
    char ch;
    file = fopen(fileName, "r");
    printf("but\n");
    if (NULL == file)
    {
        printf("File can't be opened \n");
        handleError();
    }

    do
    {
        ch = fgetc(file);
        printf("%c", ch);
    } while (ch != EOF);
    fclose(file);

    return 0;
}

void writeFile(char *fileName, char *data)
{
    FILE *file = NULL;
    file = fopen(fileName, "a");
    if (file)
    {
        fputs(data, file);
        fputs("\n", file);
        fclose(file);
    }
    else
    {
        printf("Couldn' t write file! \n");
    }
}

void createFile(char *fileName)
{
    FILE *file = NULL;
    file = fopen(fileName, "w");

    if (file == NULL)
    {
        printf("File couldn' t created!\n");
    }
    else
    {
        addFileList(fileName);
        fclose(file);
    }
}

void *handleClient(void *arg)
{
    while (1)
    {
        if (task < 6)
        {
            // pthread_mutex_lock(&mutexClientId);
            printf("\nThread icinde THID: %d\n", portIdx);
            printf("\nListening Port%s...\n", portList[portIdx]);
            int fd;
            char arr1[AVG_LENGTH];
            char *port = strdup(portList[portIdx]);
            fd = open(port, O_RDONLY);
            read(fd, arr1, AVG_LENGTH);
            printf("Thread Okundu: %s\n", arr1);
            commandToToken(arr1, AVG_LENGTH);

            int operation = chooseOperation(token[0], token[1]);
            int result = doTask(operation);
            printTokens();
            close(fd);
        }

        // pthread_mutex_unlock(&mutexClientId);
    }
    return NULL;
}

int doTask(int op)
{
    int fileExist = isFileExists(token[1]);
    if (fileExist)
    {
        if (op == 0)
        {
            readFile(token[1]);
            return 1;
        }

        if (op == 2)
        {
            deleteFile(token[1]);
            int idx = findFileIndex(token[1]);
            deleteFileList(idx);
            return 1;
        }
        if (op == 3)
        {
            writeFile(token[1], token[2]);
            return 1;
        }
    }
    else
    {
        if (op == 1)
        {
            createFile(token[1]);
            addFileList(token[1]);
            return 1;
        }
    }
}

int isFileExists(char *file)
{
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strcmp(fileList[i], file) == 0)
            return 1;
    }
    return 0;
}

int findFileIndex(char *file)
{
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strcmp(fileList[i], file) == 0)
            return i;
    }
    return -1;
}

void addFileList(char *file)
{
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strlen(fileList[i]) == 0)
        {
            strcpy(fileList[i], file);
            break;
        }
    }
}

void deleteFileList(int index)
{
    strcpy(fileList[index], "");
}

void connection()
{
    if (clientId < 5)
    {
        // pthread_mutex_lock(&mutexClientId);
        char str[2];
        char buff[35];
        char tempPort[30];
        memset(tempPort, '\0', sizeof(tempPort));
        memset(buff, '\0', sizeof(buff));
        memset(tempPort, '\0', sizeof(commPort));

        snprintf(str, 2, "%d", clientId);
        strcat(buff, str);
        strcpy(tempPort, portList[clientId]);
        strcat(buff, " ");
        strcat(buff, tempPort);

        int fd1 = open(listenPort, O_WRONLY);
        write(fd1, buff, 35);
        close(fd1);

        portIdx = clientId;
        clientCount++;
        clientId++;
        printf("Connection Info: %s   %d    \n", buff, (clientId - 1));
        // pthread_mutex_unlock(&mutexClientId);
    }
}

void *listenClients(void *arg)
{
    int fd;
    mkfifo(listenPort, 0666);
    char arr1[AVG_LENGTH];

    while (1)
    {
        printf("\nListening Port...\n");
        settings(commandInput, token);
        fd = open(listenPort, O_RDONLY);
        read(fd, arr1, AVG_LENGTH);
        close(fd);
        connection();
        task++;
        if (task > 0)
        {
            pthread_create(threadList + task, NULL, handleClient, NULL);
        }
        printf("\n---------------------------------------------\n");
    }
    pthread_join(threadList + task, &status);
    return NULL;
}

int main()
{

    pthread_mutex_init(&mutexClientId, NULL);
    settings();
    filesInit();
    // printTokens();
    createPorts();
    pthread_create(&listenThread, NULL, listenClients, NULL);
    pthread_join(listenThread, &status);
    printf("cikti\n");

    return 0;
}

void createThreads(pthread_t threadList[])
{
    for (int i = 0; i < threadCount; ++i)
    {
        pthread_create(threadList + i, NULL, handleClient, i);
    }
    for (int j = 0; j < threadCount; ++j)
    {
        pthread_join(threadList[j], &status);
    }
}

int handleError()
{
    return 0;
}

int commandLength(char *input)
{
    return strlen(input);
}

void settings()
{
    tokenCount = 0;
    memset(commandInput, '\0', MAX_IN);
    memset(token, '\0', sizeof(token));
}

void filesInit()
{
    char *files[] = {"bir.txt", "iki.txt", "uc.txt", "dort.txt",
                     "bes.txt", "alti.txt", "yedi.txt",
                     "sekiz.txt", "dokuz.txt", "on.txt"};
    for (int i = 0; i < fileCount; i++)
    {
        FILE *file = NULL;
        file = fopen(files[i], "a");
        if (file)
        {
            strcpy(fileList[i], files[i]);
            // printf(">filelist[%d] -> %s\n", i, fileList[i]);
            fclose(file);
        }
    }
}

int chooseOperation(char *command, char *file)
{
    if (strcmp(command, "read") == 0)
    {
        return 0;
    }
    if (strcmp(command, "create") == 0)
    {
        return 1;
    }
    if (strcmp(command, "delete") == 0)
    {
        return 2;
    }
    if (strcmp(command, "write") == 0)
    {
        return 3;
    }
}

void createPorts()
{
    portList[0] = "/tmp/named_pipe_0";
    portList[1] = "/tmp/named_pipe_1";
    portList[2] = "/tmp/named_pipe_2";
    portList[3] = "/tmp/named_pipe_3";
    portList[4] = "/tmp/named_pipe_4";
}

int commandToToken(char *in, int length)
{
    int index = 0;
    char *bufferArray = strdup(in);
    char *str = strtok(bufferArray, " ");

    while (str != NULL)
    {
        tokenCount++;
        token[index++] = str;
        str = strtok(NULL, " ");
    }
}

void printTokens()
{
    printf("\nPrint Files>>\n");
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strlen(fileList[i]) > 0)
        {
            printf("%d: %s\n", i, fileList[i]);
        }
    }
}
