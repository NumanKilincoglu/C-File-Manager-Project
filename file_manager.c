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

#define AVG_LENGTH 1024
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
int readFlag = 0;

char *listenPort = "/tmp/file_manager_named_pipe";
char *commPort;

char *token[MAX_TOK];
char *portList[5];
char fileList[FILE_COUNT][50];
char readFileStr[MAX_LINE][MAX_LEN];
char response[MAX_LEN];

pthread_t threadList[5];
pthread_t listenThread;

int task = 0;
int connectedClient = 0;

void deleteFile(char *fileName)
{
    pthread_mutex_lock(&file_lock);
    char str[40];
    memset(str, 0, sizeof(str));
    strcpy(str, fileName);
    deleteFlag = 0;
    if (remove(str) == 0)
    {
        printf("LOG: File deleted!\n");
        deleteFlag = 1;
    }
    else
    {
        printf("LOG: Something wrong happened!\n");
        deleteFlag = 0;
    }
    pthread_mutex_unlock(&file_lock);
}

void readFile(char *fileName, char *port)
{
    pthread_mutex_lock(&file_lock);
    memset(response, 0, sizeof(response));
    FILE *file;
    char ch;
    file = fopen(fileName, "r");

    if (NULL == file)
    {
        printf("LOG: File can't be opened!\n");
    }

    int line = 0;

    while (!ferror(file) && !feof(file))
    {
        if (fgets(readFileStr[line], MAX_LEN, file) != NULL)
        {
            char temp[200];
            memset(temp, 0, sizeof(temp));
            strcpy(temp, readFileStr[line]);
            strcat(response, temp);
            line++;
        }
    }
    fclose(file);
    int fd2;
    fd2 = open(port, O_WRONLY);
    write(fd2, response, strlen(response) + 1);
    close(fd2);
    pthread_mutex_unlock(&file_lock);
}

void writeFile(char *fileName, char *data)
{
    pthread_mutex_lock(&file_lock);
    FILE *file = NULL;
    file = fopen(fileName, "a");

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
    pthread_mutex_unlock(&file_lock);
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
        fclose(file);
        createFlag = 1;
    }
    pthread_mutex_unlock(&file_lock);
}

void sendMessage(int operation, char *port) // islem sonucunda client a gerekli bilgi mesajini gonderir
{
    char msg[80];
    int fd1;
    memset(msg, 0, sizeof(msg));

    if (operation == 1)
    { // read file messages
        return;
    }
    else if (operation == -1)
    {
        strcpy(msg, "Dosya okunamadi veya bulunamadi!");
    }

    else if (operation == 2) // create file messages
    {
        strcpy(msg, "Dosya olusturuldu!");
    }
    else if (operation == -2)
    {
        strcpy(msg, "Dosya mevcut veya hatali!");
    }
    else if (operation == 3) // delete file messages
    {
        strcpy(msg, "Dosya silindi!\n");
    }
    else if (operation == -3)
    {
        strcpy(msg, "Dosya silinirken hata olustu!");
    }
    else if (operation == 4) // write file messages
    {
        strcpy(msg, "Dosyaya yazildi!");
    }
    else if (operation == -4)
    {
        strcpy(msg, "Dosyaya yazilirken hata olustu!");
    }
    else
    {
        strcpy(msg, "Genel hata!");
    }
    fd1 = open(port, O_WRONLY);
    write(fd1, msg, strlen(msg) + 1);
    printf("Log: Client' a mesaj gonderildi>> %s %d\n", msg, operation);
    close(fd1);
}

int doTask(char port[]) // gelen komuta gore islemi yapar
{
    char fileName[20];
    if (token[0] == NULL)
    {
        strcpy(token[0], "");
    }

    if (token[1] == NULL)
    {
        strcpy(fileName, "");
    }
    else
    {
        strcpy(fileName, token[1]);
    }

    int fileExist = isFileExists(fileName);

    if (strcmp(token[0], "read") == 0)
    {
        if (fileExist)
        {
            readFile(fileName, port);
            readFlag = 1;
            return 1;
        }
        readFlag = 0;
        return -1;
    }
    if (strcmp(token[0], "create") == 0)
    {
        if (!fileExist)
        {
            createFile(token[1]);
            addFileList(token[1]);
            return 2;
        }
        createFlag = 0;
        return -2;
    }
    if (strcmp(token[0], "delete") == 0)
    {
        if (fileExist)
        {
            deleteFile(token[1]);
            int idx = findFileIndex(token[1]);
            deleteFileList(idx);
            deleteFlag = 1;
            return 3;
        }
        deleteFlag = 0;
        return -3;
    }
    if (tokenCount == 3)
    {
        if (fileExist)
        {
            writeFile(token[1], token[2]);
            writeFlag = 1;
            return 4;
        }
        writeFlag = 0;
        return -4;
    }
}

int isFileExists(char *file) // dosyanin varligini sorgular
{
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strcmp(fileList[i], file) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int findFileIndex(char *file) // verilen dosyanin indexini bulur
{
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strcmp(fileList[i], file) == 0)
        {
            return i;
        }
    }
    return -1;
}

void addFileList(char *file) // file list e dosya ekler
{
    pthread_mutex_lock(&file_lock);
    for (int i = 0; i < FILE_COUNT; i++)
    {
        if (strlen(fileList[i]) == 0)
        {
            strcpy(fileList[i], file);
            break;
        }
    }
    pthread_mutex_unlock(&file_lock);
}

void deleteFileList(int index) // dosyayi listeden siler
{
    pthread_mutex_lock(&file_lock);
    strcpy(fileList[index], "");
    pthread_mutex_unlock(&file_lock);
}

void *handleClient(void *arg) // her client icin bir thread gelecek olan komutlari bekler
{
    char port[30];
    strcpy(port, arg);
    mkfifo(port, 0666);
    while (1)
    {
        printf("\nListening Port -> '%s'\n", port);
        char clientRequest[AVG_LENGTH];
        memset(clientRequest, 0, sizeof(clientRequest));
        int fd = open(port, O_RDONLY);
        printf("Read pipe sonuc:%d\n", read(fd, clientRequest, AVG_LENGTH));
        close(fd);
        printf("Log: Client Request: %s\n", clientRequest);
        commandToToken(clientRequest, AVG_LENGTH);

        // clienttan exit komutu geldiginde bu thread kapanir
        if (token[0] != NULL && strcmp(token[0], "exit") == 0)
        {
            printf("Log: Thread Exited> %s\n", token[0]);
            task--;
            break;
        }

        int result = doTask(port);
        sendMessage(result, port);
    }
    return NULL;
}

void connection() // clientlar ile baglantiyi saglar
{
    if (clientId < 5)
    {
        int fd1;
        char str[2];
        char buff[35];
        char tempPort[30];
        memset(tempPort, '\0', sizeof(tempPort));
        memset(buff, '\0', sizeof(buff));
        snprintf(str, 2, "%d", clientId);
        strcat(buff, str);
        strcpy(tempPort, portList[clientId]);
        strcat(buff, " ");
        strcat(buff, tempPort);
        fd1 = open(listenPort, O_WRONLY);
        write(fd1, buff, 35);
        close(fd1);
        portIdx = clientId;
        clientId++;
        connectedClient++;
    }
    else
    {
        int fd1 = open(listenPort, O_WRONLY);
        write(fd1, "Client limit exceeded!", 24);
        close(fd1);
    }
}

void *listenClients(void *arg) // client in baglanmasini bekler
{
    int fd;
    mkfifo(listenPort, 0666);
    char clientRequest[AVG_LENGTH];

    while (1)
    {
        printf("\nListening Port...\n");
        fd = open(listenPort, O_RDONLY);
        read(fd, clientRequest, AVG_LENGTH);
        close(fd);
        connection();

        // client baglanirsa yeni thread olusturur ve client i ona atar
        if (connectedClient >= 1 && connectedClient < 6)
        {
            pthread_create(threadList + task, NULL, handleClient, portList[portIdx]);
            task++;
        }

        printf("\n---------------------------------------------\n");
    }
    pthread_join(threadList + task, &status);
    return NULL;
}

int main()
{
    pthread_mutex_init(&file_lock, NULL);
    createPorts();
    memset(threadList, NULL, sizeof(threadList));
    pthread_create(&listenThread, NULL, listenClients, NULL);
    pthread_join(listenThread, &status);
    return 0;
}

int commandLength(char *input)
{
    return strlen(input);
}

void settings()
{
    tokenCount = 0;
    memset(token, 0, sizeof(token));
}

// port listesini olusturur
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
    settings();

    while (str != NULL)
    {
        tokenCount++;
        token[index++] = str;
        str = strtok(NULL, " ");
    }
}

void printFiles()
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

void printstr(char *str)
{
    int i = 0;
    while (str[i] != '\0')
    {
        printf("%d->[%d]:%c\n", str[i], i, str[i]);
        i++;
    }
}

void printTokens()
{
    printf("\nPrint Tokens>>\n");
    for (int i = 0; i < tokenCount; i++)
    {
        printf("%d: %s\n", i, token[i]);
    }
}
