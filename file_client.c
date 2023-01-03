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

#define BUFF_LEN 64
#define MAX_TOKEN 32
#define CLIENT_MAX_INPUT 64
#define FIFO_LEN 1024

char commandInput[BUFF_LEN];
char *token[MAX_TOKEN];
char *responseArray[MAX_TOKEN];

char *named_pipe = "/tmp/file_manager_named_pipe";
char *customPipe;
char *clientID;
int tokenCount = 0;
int clientId;

void connectManager() // manager ile pipe uzerindenbaglanti kurar
{
    char buffer[35];
    char arr2[2];
    int fd = open(named_pipe, O_WRONLY);
    write(fd, "connection", 11);
    close(fd);

    int fd1 = open(named_pipe, O_RDONLY);
    read(fd1, buffer, 35);
    printf("Client Info: %s\n", buffer);
    close(fd1);
    printf("Baglandi\n");
    responseToToken(buffer);
}

int main()
{
    connectManager();

    while (1)
    {
        settings(commandInput, token); // arrayleri resetler
        printf("\n\nClient >> ");
        if (readCommandLine(commandInput, CLIENT_MAX_INPUT))
        {
            commandToToken();
            int choice = handleCommand();
            switch (choice)
            {
            case 1:
                sendExitMsg(); // client exit oldugu zaman pipe ile manager a mesaj gonderir
                break;
            case 2:
                executeCommand(); // read create delete komutlarini managera gonderir
                break;
            case 3:
                writeFile(); // write komutunu manager a gonderir ve cevap bekler
                break;
            }
        }

        else
        {
            printf("\nLutfen gecerli bir komut giriniz!\n");
            continue;
        }
    }
    printf("cikti\n");
    return 0;
}

void executeCommand()
{
    mkfifo(customPipe, 0666);
    char str[80];
    memset(str, 0, sizeof(str));
    strcat(str, token[0]);
    strcat(str, " ");
    strcat(str, token[1]);
    strcat(str, ".txt");
    int fd1 = open(customPipe, O_WRONLY);
    write(fd1, str, strlen(str) + 1);
    close(fd1);

    char serverResponse[FIFO_LEN]; // managerdan gelen donutu gosterir
    memset(serverResponse, 0, sizeof(serverResponse));
    int fd2 = open(customPipe, O_RDONLY);
    read(fd2, serverResponse, FIFO_LEN);
    printf("Server>>\n%s", serverResponse);
    close(fd2);
}

void writeFile()
{
    mkfifo(customPipe, 0666);
    char str[FIFO_LEN];
    memset(str, 0, sizeof(str));
    strcat(str, token[0]);
    strcat(str, " ");
    strcat(str, token[1]);
    strcat(str, ".txt");
    strcat(str, " ");
    strcat(str, token[2]);

    int fd1 = open(customPipe, O_WRONLY);
    write(fd1, str, strlen(str) + 1);
    close(fd1);

    char serverResponse[FIFO_LEN]; // managerdan gelen donutu gosterir
    memset(serverResponse, 0, sizeof(serverResponse));
    fd1 = open(customPipe, O_RDONLY);
    read(fd1, serverResponse, FIFO_LEN);
    printf("Server>>\n%s", serverResponse);
    close(fd1);
}

int readCommandLine(char *input, int maxLength)
{
    if (!fgets(input, maxLength, stdin))
        return 0;
    int cmdLength = commandLength(input);
    if (cmdLength == 0)
        return 0;
    if (cmdLength == 1 && (input[0] == '\n' || input[0] == '\r' || input[0] == '\t'))
        return 0;
    if (input[cmdLength - 1] == '\n')
    {
        input[cmdLength - 1] = '\0';
    }
    return 1;
}

int commandLength(char *input)
{
    return strlen(input);
}

int responseToToken(char *response)
{
    int index = 0;
    char *bufferArray = strdup(response);
    char *str = strtok(bufferArray, " ");

    while (str != NULL)
    {
        responseArray[index++] = str;
        str = strtok(NULL, " ");
    }

    char *buffer = strdup(responseArray[0]);
    customPipe = strdup(responseArray[1]);
}

int commandToToken()
{
    int index = 0;
    char *bufferArray = strdup(commandInput);
    char *str = strtok(bufferArray, " ");

    while (str != NULL)
    {
        tokenCount++;
        token[index++] = str;
        str = strtok(NULL, " ");
    }
}

int handleCommand()
{
    if (tokenCount == 1 && strcmp(token[0], "exit") == 0)
    {
        return 1;
    }

    else if (tokenCount == 2 && strcmp(token[0], "create") == 0)
    {
        if (token[1] == NULL)
        {
            return -1;
        }
        return 2;
    }
    else if (tokenCount == 2 && strcmp(token[0], "delete") == 0)
    {
        if (token[1] == NULL)
        {
            return -1;
        }
        return 2;
    }
    else if (tokenCount == 2 && strcmp(token[0], "read") == 0)
    {
        if (token[1] == NULL)
        {
            return -1;
        }
        return 2;
    }
    else if (tokenCount == 3 && strcmp(token[0], "write") == 0)
    {
        if (token[1] == NULL)
        {
            return -1;
        }
        return 3;
    }

    else
    {
        return -2;
    }
}

void handleErrors(int errorCode)
{
    if (errorCode == -1)
    {
        printf("\nDosya adini kontrol edin!\n");
    }
    if (errorCode == -2)
    {
        printf("\nLutfen paramatreleri kontrol edin!\n");
    }
}

int checkChar(char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] > '9' || str[i] < '0')
            return 0;
    }
    return 1;
}

void settings()
{
    tokenCount = 0;
    memset(commandInput, '\0', CLIENT_MAX_INPUT);
    memset(token, '\0', sizeof(token));
}

void print(char *input, int lineLength)
{
    int index = 0;

    while (input[index] != '0' && index <= lineLength)
    {
        printf("%c", input[index++]);
    }
    printf("\n%d --> ", lineLength);
}

void sendExitMsg()
{
    int fd1 = open(customPipe, O_WRONLY);
    write(fd1, "exit", 5);
    close(fd1);
    exit(1);
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
