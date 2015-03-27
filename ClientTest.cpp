#include <stdio.h>

#include "Host.h"

using namespace Net;

int main(void)
{
    Host* clnt = new Host();
    printf("client created\n");
    getchar();

    clnt->connect("localhost",7000);
    printf("client connected\n");
    getchar();

    delete clnt;
    printf("client deleted\n");
    getchar();

    return 0;
}
