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

pthread_mutex_t mutexClientId;

int clientCount = 0;
int cli = 0;
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

pthread_t threadList[5];
pthread_t listenThread;
pthread_t comTh;
pthread_t comTh2;
int flag = 0;
int connected[5];

int task = 0;
int done = 0;

void deleteFile(char *fileName)
{
    if (remove(fileName) == 0)
    {
        printf("File deleted!\n");
    }
    else
    {
        printf("Something wrong appened!\n");
    }

    return NULL;
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
        fclose(file);
    }
}

void *handleClient(void *arg)
{

    printf("---->>>>>>%d\n", done < task);
    while (1)
    {
        // pthread_mutex_lock(&mutexClientId);
        printf("\nThread icinde THID: %d\n", comTh);
        printf("\n\nListening Port%s...\n", portList[portIdx]);
        int fd;
        char arr1[80];
        char *port = strdup(portList[portIdx]);
        fd = open(port, O_RDONLY);
        read(fd, arr1, 80);
        printf("Thread icinde okundu: %s\n", arr1);
        commandToToken(arr1, 80);

        if (token[2] != NULL)
        {
            chooseOperation(token[0], token[1], token[2]);
        }
        else
        {
            chooseOperation(token[0], token[1], NULL);
        }
        close(fd);
        // pthread_mutex_unlock(&mutexClientId);
    }
    return NULL;
}

void connection()
{
    if (clientId < 5)
    {
        //pthread_mutex_lock(&mutexClientId);
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
        // printf("Client Broadcast: %s   %d    %d\n", buff, clientId, clientCount);

        portIdx = clientId;
        clientCount++;
        clientId++;
        printf("Connect Info: %s   %d    \n", buff, (clientId - 1));
        // pthread_mutex_unlock(&mutexClientId);
    }
}

void *listenClients(void *arg)
{
    int fd;
    mkfifo(listenPort, 0666);
    char arr1[80];
    printf("\nListening port...\n");

    while (1)
    {
        printf("\nListening port...\n");
        settings(commandInput, token);
        fd = open(listenPort, O_RDONLY);
        read(fd, arr1, 80);
        printf("Listen Port Reading: %s\n", arr1);
        close(fd);
        printf("Con once\n");
        connection();
        task++;
        if (task > 0)
        {
            printf(">>>>Task Oldu %d\n", task);
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
    memset(connected, 0, sizeof(connected));
    filesInit();
    createPorts();
    pthread_create(&listenThread, NULL, listenClients, NULL);
    pthread_join(listenThread, &status);

    // pthread_create(&comTh2, NULL, handleClient, NULL);
    // pthread_join(comTh2, &status);
    //

    // createThreads(threadList);

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
            fclose(file);
        }
    }
}

int handleError()
{
    return 0;
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

void chooseOperation(char *command, char *file, char *data)
{
    if (strcmp(command, "read") == 0)
    {
        readFile(file);
    }
    if (strcmp(command, "create") == 0)
    {
        createFile(file);
    }
    if (strcmp(command, "delete") == 0)
    {
        deleteFile(file);
    }
    if (strcmp(command, "write") == 0)
    {
        writeFile(file, data);
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

void createPipes()
{
}