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

char commandInput[BUFF_LEN];
char *token[MAX_TOKEN];
char *responseArray[MAX_TOKEN];

char *named_pipe = "/tmp/file_manager_named_pipe";
char *customPipe;
char *clientID;
int tokenCount = 0;
int clientId;

void connectManager()
{
    char buffer[35];
    char arr2[2];
    printf("bagalnti oncesi %s \n", named_pipe);
    int fd = open(named_pipe, O_WRONLY);
    write(fd, "baglanti", 9);
    close(fd);
    printf("bagalnti cikis\n");
    printf("bagalnti gi1\n");
    int fd1 = open(named_pipe, O_RDONLY);
    read(fd1, buffer, 35);
    printf("Client Info: %s\n", buffer);
    close(fd1);
    printf("bagalnti cikis1\n");

    responseToToken(buffer);
}

int main()
{

    connectManager();

    while (1)
    {
        settings(commandInput, token);
        printf("\nClient >> ");
        if (readCommandLine(commandInput, CLIENT_MAX_INPUT))
        {
            commandToToken();
            int choice = handleCommand();
            switch (choice)
            {
            case 1:
                sendExitMsg();
                break;
            case 2:
                executeCommand();
                break;
            case 3:
                writeFile();
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
    strcat(str, token[0]);
    strcat(str, " ");
    strcat(str, token[1]);
    strcat(str, ".txt");
    printf("custom pipe:%s  %s\n", customPipe, str);
    int fd1 = open(customPipe, O_WRONLY);
    write(fd1, str, strlen(str) + 1);
    close(fd1);
}

void writeFile()
{
    mkfifo(customPipe, 0666);
    char str[80];
    strcat(str, token[0]);
    strcat(str, " ");
    strcat(str, token[1]);
    strcat(str, ".txt");
    strcat(str, " ");
    strcat(str, token[2]);

    int fd1 = open(customPipe, O_WRONLY);
    write(fd1, str, strlen(str) + 1);
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
    // snprintf(buffer, 2, "%d", clientId);
    clientID = strdup(responseArray[0]);
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

void printTokens()
{
    for (int i = 0; i < tokenCount; i++)
    {
        printf("\n%d: %s\n", i, token[i]);
    }
}

void sendExitMsg()
{
    int fd1 = open(named_pipe, O_WRONLY);
    write(fd1, token[0], strlen(token[0]) + 1);
    close(fd1);
    exit(1);
}