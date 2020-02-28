#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/types.h> /* pid_t */
#include <unistd.h>    /* fork */
#include <errno.h>     /* errno */
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "lab7.h"

#define N 4
#define FALSE 0
#define TRUE 1

Machine machines[N];
int costs[N][N];
int sockfd;

pthread_t threads[2];
pthread_mutex_t lock;

void printCosts()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            printf("%d ", costs[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int minDist(int dist[], int nPrime[])
{
    int min = 10000, min_index = 0, i;
    for (i = 0; i < N; i++)
    {
        if (nPrime[i] == FALSE && dist[i] <= min)
        {
            min = dist[i];
            min_index = i;
        }
    }
    return min_index;
}

int printSolution(int dist[], int src)
{
    int i;
    printf("Source Node: %d \n", src);
    printf("Vertex \t\t Distance from Source\n");
    for (i = 0; i < N; i++)
        printf("%d \t\t %d\n", i, dist[i]);

    printf("\n");
}

void dijkstras(int src)
{
    printf("printing from dijkstras:\n");
    printCosts();
    int nPrime[N];
    int count = 0, i;
    int dist[N];
    for (count = 0; count < N; count++)
    {
        dist[count] = 1000;
        nPrime[count] = FALSE;
    }
    dist[src] = 0;
    for (count = 0; count < N - 1; count++)
    {
        int node = minDist(dist, nPrime);
        nPrime[node] = TRUE;

        for (i = 0; i < N; i++)
        {
            if (!nPrime[i] && costs[node][i] && dist[node] != 1000 && (dist[node] + costs[node][i]) < dist[i])
            {
                dist[i] = dist[node] + costs[node][i];
            }
        }
    }
    printSolution(dist, src);

    return;
}

void *receive_info_from_neighbors(void *arg)
{
    struct sockaddr_in *clienAddr = arg;
    socklen_t addrLen = sizeof(struct sockaddr);
    int i;
    while (1)
    {
        int updates[3];
        //receive updated cost table from neighbors and then update your own cost table
        for (i = 0; i < N; i++)
        {
            int nr = recvfrom(sockfd, &updates, sizeof(updates), 0, (struct sockaddr *)&clienAddr, &addrLen);

            pthread_mutex_lock(&lock);
            printf("[receive_info_from_neighbors]changing the costs: \n");
            printf("updates[0]] : %d [updates[1] : %d updates[2]] : %d \n",updates[0],updates[1],updates[2]);
            printCosts();
            costs[updates[0]][updates[1]] = updates[2];
            costs[updates[1]][updates[0]] = updates[2];

            pthread_mutex_unlock(&lock);
        }
    }
}

void *receive_info_from_user(void *arg)
{
    int changes = N;
    struct sockaddr_in *servAddr = arg;

    while (changes != 0)
    {
        int vertexOne;
        int vertexTwo;
        int updatedLinkCost;
        printf("Enter 2 vertices and the new link cost between them... \n");
        scanf("%d %d %d", &vertexOne, &vertexTwo, &updatedLinkCost);
        // scanf("%d", vertexTwo);
        // scanf("%d", updatedLinkCost);
        printf("vertexOne[0]] : %d [updatvertexOnees[1] : %d vertexOne[2]] : %d \n",vertexOne,vertexTwo,updatedLinkCost);
        pthread_mutex_lock(&lock);

        printf("[receive_info_from_user]changing the costs: \n");
        costs[vertexOne][vertexTwo] = updatedLinkCost;
        costs[vertexTwo][vertexOne] = updatedLinkCost;

        pthread_mutex_unlock(&lock);
        //update costs and notify neighbors you updated your cost table (neighbors are in machines)

        //notify neighbors that you updated your cost table
        int i;
        for (i = 0; i < N; i++)
        {
            int updates[3] = {vertexOne, vertexTwo, updatedLinkCost};
            sendto(sockfd, &updates, sizeof(updates), 0, (struct sockaddr *)&servAddr[i], sizeof(struct sockaddr));
        }

        changes--;
        sleep(3);
    }
}

void *run_link_state(void *arg)
{
    while (1)
    {
        int randSleep = (rand() % 11 + 10);
        int src;
        printf("printing from run_link_state:\n");
        printCosts();
        for (src = 0; src < N; src++)
        {
            dijkstras(src);
        }

        printCosts();
        sleep(randSleep);
    }
}

int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        fprintf(stderr, "Usage %s <costs.txt> <machines.txt> <Number of self machine> \n", argv[0]);
        return 1;
    }

    printf("Hello \n");
    int selfAddr = atoi(argv[3]);
    int i, num1, num2, num3, num4;
    FILE *costs_file;
    FILE *machines_file;
    int sockfds[N];
    char machineName[50];
    char machineIP[50];

    costs_file = fopen(argv[1], "r");
    machines_file = fopen(argv[2], "r");

    for (i = 0; i < N; i++)
    {
        //fscanf(costs_file, "%d %d %d %d", costs[i][0], costs[i][1], costs[i][2], costs[i][3]);
        //fscanf(machines_file, "%s", "%s", "%d", machines[i].name, machines[i].ip, machines[i].port);

        fscanf(costs_file, "%d %d %d %d", &num1, &num2, &num3, &num4);
        costs[i][0] = num1;
        costs[i][1] = num2;
        costs[i][2] = num3;
        costs[i][3] = num4;
        fscanf(machines_file, "%s %s %d", &machineName, &machineIP, &num1);
        strncpy(machines[i].name, machineName, 50);
        strncpy(machines[i].ip, machineIP, 50);
        machines[i].port = num1;
    }

    printCosts();

    //------------------------------------ Networking shit -------------------------------
    printf("Initializing networking shit... \n");
    struct sockaddr_in servAddr[N], clienAddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Failure to setup an endpoint socket");
        exit(1);
    }

    for (i = 0; i < N; i++)
    {
        servAddr[i].sin_family = AF_INET;
        servAddr[i].sin_port = htons(machines[i].port);
        servAddr[i].sin_addr.s_addr = INADDR_ANY;
    }

    if ((bind(sockfd, (struct sockaddr *)&servAddr[selfAddr - 1], sizeof(struct sockaddr))) < 0)
    {
        perror("Failure to bind server address to the endpoint socket");
        exit(1);
    }
    //------------------------------------------------------------------------------------

    printf("Initializing mutex lock... \n");

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return 1;
    }
    printf("printing from main:\n");
    printCosts();
    printf("Creating threads... \n");

    pthread_create(&threads[0], NULL, receive_info_from_neighbors, (void *)&servAddr[selfAddr - 1]);
    pthread_create(&threads[1], NULL, receive_info_from_user, (void *)servAddr);
    pthread_create(&threads[1], NULL, run_link_state, (void *)servAddr);

    for (i = 0; i < 2; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    return 0;
}
