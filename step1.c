#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>


int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Usage %s <file1.txt> \n", argv[0]);
        return 1;
    }

    FILE *fp;
    fp = fopen(argv[1], "r");

    char buf[10000];

    while(fread(&buf, sizeof(buf), 1, fp))
    {
        //printf("Reading buffer of size 10000... \n");
    }

    fclose(fp);

    return 0;
}
