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

#define MAX_LINE 200
#define MAX_LEN 1000

#define AVG_LENGTH 200
#define FILE_COUNT 10

pthread_mutex_t file_lock;

int portIdx = 0;
int tokenCount = 0;
int clientId = 0;
int threadCount = 5;
int fileCount = 10;
int portCount = 5;
void *status;

int createFlag = 0;
int writeFlag = 0;
int deleteFlag = 0;

char *listenPort = "/tmp/file_manager_named_pipe";
char *commPort;

char commandInput[BUFFER];
char *token[MAX_TOK];
char *portList[5];
char fileList[FILE_COUNT][50];
char readFileStr[MAX_LINE][MAX_LEN];
char response[MAX_LEN];

pthread_t threadList[5];
pthread_t listenThread;

int task = 0;

void deleteFile(char *fileName)
{
    pthread_mutex_lock(&file_lock);
    char str[40];
    memset(str, '\0', sizeof(str));
    strcpy(str, fileName);
    deleteFlag = 0;
    if (remove(str) == 0)
    {
        printf("File deleted!\n");
        deleteFlag = 1;
    }
    else
    {
        printf("Something wrong happened!\n");
        deleteFlag = 0;
    }
    pthread_mutex_unlock(&file_lock);
}

void readFile(char *fileName)
{
    memset(response, '\0', sizeof(response));
    FILE *file;
    char ch;
    file = fopen(fileName, "r");

    if (NULL == file)
    {
        printf("File can't be opened!\n");
        handleError();
    }

    int line = 0;
    pthread_mutex_lock(&file_lock);
    while (!ferror(file) && !feof(file))
    {
        if (fgets(readFileStr[line], MAX_LEN, file) != NULL)
        {
            char temp[50];
            strcpy(temp, readFileStr[line]);
            strcat(response, temp);
            strcat(response, " ");
            line++;
        }
    }
    fclose(file);
    printf("Mutexe geldi Read-->\n");
    pthread_mutex_unlock(&file_lock);
}

void writeFile(char *fileName, char *data)
{
    FILE *file = NULL;
    file = fopen(fileName, "a");
    writeFlag = 0;
    printf("dosya: %s    %s\n", fileName, data);
    if (file)
    {
        fputs(data, file);
        fputs("\n", file);
        fclose(file);
        writeFlag = 1;
    }
    else
    {
        printf("Couldn' t write file! \n");
        writeFlag = 0;
    }
    printf("write flag -->%d\n", writeFlag);
}

void createFile(char *fileName)
{
    pthread_mutex_lock(&file_lock);
    FILE *file = NULL;
    file = fopen(fileName, "w");

    if (file == NULL)
    {
        printf("File couldn' t created!\n");
        createFlag = 0;
    }
    else
    {
        printf("%d--->\n", createFlag);
        addFileList(fileName);
        fclose(file);
        createFlag = 1;
    }
    pthread_mutex_unlock(&file_lock);
}

void sendMessage(int operation, char *port)
{
    int fd1 = open(port, O_WRONLY);
    char msg[40];
    memset(msg, '\0', sizeof(msg));
    printf(">>>mesaja geldi\n");
    if (operation == 0) // read file
    {
        write(fd1, response, strlen(response) + 1);
        close(fd1);
        return 0;
    }
    if (operation == 1) // create
    {

        if (createFlag == 1)
        {
            strcpy(msg, "Dosyaya olusturuldu!\n");
        }
        else
        {
            strcpy(msg, "Dosyaya olusturulurken hata olustu!\n");
        }
        printf("olustur sonunda geldi> \n");
    }
    if (operation == 2) // delete
    {
        if (deleteFlag == 1)
        {
            strcpy(msg, "Dosyaya silindi!\n");
        }
        else
        {
            strcpy(msg, "Dosyaya silinirken hata olustu!\n");
        }
        printf("silme sonunda geldi> \n");
    }
    if (operation == 3) // write
    {
        if (writeFlag == 1)
        {
            strcpy(msg, "Dosyaya yazildi!\n");
        }
        else
        {
            strcpy(msg, "Dosyaya yazilirken hata olustu!\n");
        }
        printf("yazma sonunda geldi> \n");
    }
    write(fd1, msg, strlen(msg) + 1);
    printf("mesaj gonderildi> %s\n", msg);
    close(fd1);
}

void *handleClient(void *arg)
{
    while (1)
    {
        printf("\nThread icinde THID: %d\n", portIdx);
        printf("\nListening Port%s...\n", portList[portIdx]);
        int fd;
        char arr1[AVG_LENGTH];
        char *port = strdup(portList[portIdx]);
        fd = open(port, O_RDONLY);
        read(fd, arr1, AVG_LENGTH);
        printf("Thread Okundu: %s\n", arr1);
        commandToToken(arr1, AVG_LENGTH);

        if (token[0] != NULL && strcmp(token[0], "exit") == 0)
        {
            printf("exited> %s\n", token[0]);
            close(fd);
            break;
        }
        printf("operasyon oncesi: \n");
        int operation = chooseOperation(token[0]);
        printf("->%s %s %d\n", token[0], token[1], operation);
        printf("operasyon sonrasi: \n");
        int result = doTask(operation);
        printf("result sonrasi: %d--%d\n", operation, result);
        sendMessage(operation, port);
        close(fd);
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
            printf("create geldi?> \n");
            createFile(token[1]);
            printf("create cikti?> \n");
            addFileList(token[1]);
            printf("file olusut> \n");
            return 1;
        }
    }
}

int isFileExists(char *file)
{
    pthread_mutex_lock(&file_lock);
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strcmp(fileList[i], file) == 0)
        {
            pthread_mutex_unlock(&file_lock);
            return 1;
        }
    }
    pthread_mutex_unlock(&file_lock);
    return 0;
}

int findFileIndex(char *file)
{
    pthread_mutex_lock(&file_lock);
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strcmp(fileList[i], file) == 0)
        {
            pthread_mutex_unlock(&file_lock);
            return i;
        }
    }
    pthread_mutex_unlock(&file_lock);
    return -1;
}

void addFileList(char *file)
{
    printf("--qweqweqweqweqw flie\n", 1);
    // pthread_mutex_lock(&file_lock);
    printf("-->%d geldi qweqweqweqweqw flie\n", 1);
    for (int i = 0; i < FILE_COUNT; i++)
    {
        printf("-->%d geldi add flie\n", i);
        if (strlen(fileList[i]) == 0)
        {
            strcpy(fileList[i], file);
            printf("-->sonra geldi add flie\n");
            pthread_mutex_unlock(&file_lock);
            break;
        }
    }
    printf("-->en sonda geldi add flie\n");
}

void deleteFileList(int index)
{
    pthread_mutex_lock(&file_lock);
    strcpy(fileList[index], "");
    pthread_mutex_unlock(&file_lock);
}

void connection()
{
    if (clientId < 5)
    {
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
        clientId++;
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

    pthread_mutex_init(&file_lock, NULL);
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

int chooseOperation(char *command)
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
        printf("--------burda"); //TODO BURADA STRCMP OLMUYO WRITETA DIKKAT !!! 
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
