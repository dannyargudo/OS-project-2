#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//define constants
#define PAGESIZE 256
#define BUF_SIZE 256
#define PHYSICALMEMORYSIZE 256
#define TLBENTRIES 16

//Output file names
#define OUT_FILE_1 "out1.txt"
#define OUT_FILE_2 "out2.txt"
#define OUT_FILE_3 "out3.txt"

//define TLB struct
struct TLB {
    unsigned char TLBpage[16];
    unsigned char TLBframe[16];
    int ind;
};

//Demand paging function
//Is called in find_page function when a page fault occurs
//reads in a 256-byte page from BACKING_STORE.bin and stores it in available page frame
int demand_paging(int pageNum, char *physMem, int *curFrame) {
    char buffer[BUF_SIZE] = {0};

    FILE *bin = fopen("BACKING_STORE.bin", "rb");

    fseek(bin, pageNum * PHYSICALMEMORYSIZE, SEEK_SET);
    fread(buffer, sizeof(char), PHYSICALMEMORYSIZE, bin);
    fclose(bin);

    for (int i = 0; i < PHYSICALMEMORYSIZE; i++) {
        physMem[(*curFrame) * PHYSICALMEMORYSIZE + i] = buffer[i];
    }

    (*curFrame)++;

    return (*curFrame) - 1;
}

//function to perform address translation
int find_page(int logicalAddr, char *PT, struct TLB *tlb, char *physMem, int *curFrame, int *numPageFaults, int *TLBhits, FILE *outputFile1, FILE *outputFile2, FILE *outputFile3) {
   
   //8 bit mask
    unsigned char mask = 0xFF;
    unsigned char offset;
    unsigned char pageNum;
    bool TLBhit = false;
    int frame = 0;
    int value;

    //mask the rightmost 16 bits
    //get 8 bit page num
    //get 8 bit offset
    pageNum = (logicalAddr >> 8) & mask;    

    offset = logicalAddr & mask;
    
    //Check if in TLB
    for (int i = 0; i < TLBENTRIES; i++){
        if(tlb->TLBpage[i] == pageNum){
            frame = tlb->TLBframe[i];
            TLBhit = true;
            (*TLBhits)++;
            break;
        }
            
    }

    //Check if in PageTable
    if (!TLBhit){
        //if not in either, then call demand_paging
        if (PT[pageNum] == -1){
            int newFrame = demand_paging(pageNum, physMem, curFrame);
            PT[pageNum] = newFrame;
            (*numPageFaults)++;
        }
        //when updating TLB, use FIFO
        frame = PT[pageNum];
        tlb->TLBpage[tlb->ind] = pageNum;
        tlb->TLBframe[tlb->ind] = PT[pageNum];
        tlb->ind = (tlb->ind + 1)%TLBENTRIES;
        
    }
    //calculate physical address and value
    int physicalAddr = ((unsigned char)frame*PHYSICALMEMORYSIZE) + offset;
    value = *(physMem + physicalAddr);

    //write to output file
    fprintf(outputFile1, "%d\n", logicalAddr);
    fprintf(outputFile2, "%d\n", physicalAddr);
    fprintf(outputFile3, "%d\n", value);

    return 0;
}

//main function
int main(int argc, char* argv[]) {
    int val;

    float pageFaultRate, TLBHitRate;

    unsigned char PageTable[PAGESIZE];
    memset(PageTable, -1, sizeof(PageTable));

    struct TLB tlb;
    memset(tlb.TLBpage, -1, sizeof(tlb.TLBpage));
    memset(tlb.TLBframe, -1, sizeof(tlb.TLBframe));
    tlb.ind = 0;

    char PhyMem[PHYSICALMEMORYSIZE][PHYSICALMEMORYSIZE];

    FILE* fd = fopen(argv[1], "r");

    FILE* outputFile1 = fopen(OUT_FILE_1, "w");
    FILE* outputFile2 = fopen(OUT_FILE_2, "w");
    FILE* outputFile3 = fopen(OUT_FILE_3, "w");

    int openFrame = 0;
    int numPageFaults = 0;
    int TLBhits = 0;
    int numAddresses = 0;

    //read file, call find_page for each address
    while (fscanf(fd, "%d", &val) == 1) {
        find_page(val, PageTable, &tlb, (char*)PhyMem, &openFrame, &numPageFaults, &TLBhits, outputFile1, outputFile2, outputFile3);
        numAddresses++;
    }

    pageFaultRate = (float)numPageFaults / (float)numAddresses;
    TLBHitRate = (float)TLBhits / (float)numAddresses;
    printf("Page Fault Rate = %.4f\nTLB hit rate = %.4f\n", pageFaultRate, TLBHitRate);

    fclose(fd);
    fclose(outputFile1);
    fclose(outputFile2);
    fclose(outputFile3);

    return 0;
}






