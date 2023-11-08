/*
Danny Argudo, Jason Swick
CSC345
Project #3: Virtual Memory Manager
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PAGESIZE 256
#define TLBENTRIES 16

int main(int argc, char* argv[])
{
    FILE* fp = fopen("addresses.txt", "rt");


    fclose(fp);
    return 0;
}