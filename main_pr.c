/*
Danny Argudo, Jason Swick
CSC345
Project #3: Virtual Memory Manager
main_pr
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//define constants
#define PAGESIZE 256
#define BUF_SIZE 256
//Assuming smaller physical address space
#define PHYSICALMEMORYSIZE 128
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

//Page table struct
struct pageTable
{
    int frame;
    bool valid;
};

//FIFO queue struct
struct fifoQueue
{
    int head;
    int tail;
    int* qArray;
};

//make a new FIFO queue
struct fifoQueue* newQueue(int MAX)
{
    struct fifoQueue* queue = (struct fifoQueue*)malloc(sizeof(struct fifoQueue));
    if(queue == NULL)
    {
        perror("Error allocating memory for queue");
        exit(EXIT_FAILURE);
    }
    queue->head = -1;
    queue->tail = -1;
    queue->qArray = (int*)malloc(MAX * sizeof(int));
    if(queue->qArray == NULL)
    {
        perror("Error allocating memory for queue array");
        free(queue);
        exit(EXIT_FAILURE);
    }
    return queue;
}

//check if queue is full
bool qFull(struct fifoQueue* queue, int MAX)
{
    return((queue->tail+1) % MAX == queue->head);
}

//check if queue is empty
bool qEmpty(struct fifoQueue* queue)
{
    return(queue->head == -1);
}

//Enqueue an element in the Queue
void enqueue(struct fifoQueue* queue, int element, int MAX)
{
    if(qFull(queue, MAX))
    {
        printf("Queue is full. Element %d not enqueed. \n", element);
        return;
    }
    if(qEmpty(queue))
        queue->head = 0;
    queue->tail = (queue->tail + 1)% MAX;
    queue->qArray[queue->tail] = element;   
    //printf("Enqueued element: %d\n", element); 
}

//dequeue an element in the queue
int dequeue(struct fifoQueue* queue, int MAX)
{
    if(qEmpty(queue))
    {    
        printf("error: Queue is empty\n");
        return -1;
    }

    int element = queue ->qArray[queue->head];
    if(queue->head == queue->tail)
    {
        //printf("Dequeued last element: %d\n", element);  
        queue->head = -1;
        queue->tail = -1;
    }
    else
    {    
        //printf("Dequeued element: %d\n", element);  
        queue->head = (queue->head+1) % MAX;
    }  
    return element;        
}


//Demand paging function
//Is called in find_page function when a page fault occurs
//reads in a 256-byte page from BACKING_STORE.bin and stores it in available page frame
int demand_paging(int pageNum, char *physMem, int *curFrame) {
    char buffer[BUF_SIZE] = {0};

    FILE *bin = fopen("BACKING_STORE.bin", "rb");

    fseek(bin, pageNum * PHYSICALMEMORYSIZE, SEEK_SET);
    fread(buffer, sizeof(char), PHYSICALMEMORYSIZE, bin);
    fclose(bin);
    if((*curFrame) < PHYSICALMEMORYSIZE)
    {
        for (int i = 0; i < PHYSICALMEMORYSIZE; i++) {
            physMem[(*curFrame) * PHYSICALMEMORYSIZE + i] = buffer[i];
        }

        (*curFrame)++;

        return (*curFrame) - 1;
    }
    else
    {
        //printf("Error: Attempting to exceed physical memory bounds\n");
        return -1;
    }
}


int find_page(int logicalAddr, struct pageTable* PT, struct TLB *tlb, char *physMem, int *curFrame, int *numPageFaults, int *TLBhits, struct fifoQueue* pageQ, int MAX, FILE *outputFile1, FILE *outputFile2, FILE *outputFile3)
{
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

    if(!TLBhit)
    {
        if(!PT[pageNum].valid)
        {
            if(qFull(pageQ, MAX))
            {
                int removedP = dequeue(pageQ, MAX);
                if(removedP != -1)
                {
                    PT[removedP].valid = false;
                }
            }
            frame = demand_paging(pageNum, physMem, curFrame);
            PT[pageNum].frame = frame;
            PT[pageNum].valid = true;
            enqueue(pageQ, pageNum, MAX);
            (*numPageFaults)++;
        }
        else
        {
            frame = PT[pageNum].frame;
        }

        int tlbIndex = tlb->ind % TLBENTRIES;
        tlb->TLBpage[tlbIndex] = pageNum;
        tlb->TLBframe[tlbIndex] = frame;
        tlb->ind++;
    }

    //calculate physical address and value
    int physicalAddr = ((unsigned char)frame*PHYSICALMEMORYSIZE) + offset;
    //printf("Before acccesing physMem\n");
    if(frame >= 0 && frame < PHYSICALMEMORYSIZE && physMem != NULL)
    {
        value = physMem[frame * PHYSICALMEMORYSIZE + (int)offset];
        //printf("After accesing physMem\n");
    }
    else{
        //printf("Error:Invalid frame or physMem is null\n");
        //exit(EXIT_FAILURE);
    }

    //write to output file
    fprintf(outputFile1, "%d\n", logicalAddr);
    fprintf(outputFile2, "%d\n", physicalAddr);
    fprintf(outputFile3, "%d\n", value);

    return 0;
}

int main(int argc, char* argv[]) {
    int val;

    float pageFaultRate, TLBHitRate;

    struct pageTable PageTable[PAGESIZE];
    for(int i=0; i< PAGESIZE; i++)
    {
        PageTable[i].valid = false;
    }

    struct TLB tlb;
    memset(tlb.TLBpage, -1, sizeof(tlb.TLBpage));
    memset(tlb.TLBframe, -1, sizeof(tlb.TLBframe));
    tlb.ind = 0;

    char* PhyMem = (char*)malloc(PHYSICALMEMORYSIZE * PHYSICALMEMORYSIZE);

    FILE* fd = fopen(argv[1], "r");
    if(fd == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    FILE* outputFile1 = fopen(OUT_FILE_1, "w");
    FILE* outputFile2 = fopen(OUT_FILE_2, "w");
    FILE* outputFile3 = fopen(OUT_FILE_3, "w");

    if(outputFile1 == NULL || outputFile2 == NULL || outputFile3 == NULL)
    {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    int openFrame = 0;
    int numPageFaults = 0;
    int TLBhits = 0;
    int numAddresses = 0;
    struct fifoQueue* pQueue = newQueue(PHYSICALMEMORYSIZE);

    //read file, call find_page for each address
    while (fscanf(fd, "%d", &val) == 1) {
        //printf("Read value: %d\n", val);
        find_page(val, PageTable, &tlb, (char*)PhyMem, &openFrame, &numPageFaults, &TLBhits, pQueue, PHYSICALMEMORYSIZE, outputFile1, outputFile2, outputFile3);
        numAddresses++;
    }

    printf("Total addresses read: %d\n", numAddresses);
    printf("Number of page faults: %d\n", numPageFaults);
    pageFaultRate = (float)numPageFaults / (float)numAddresses;
    TLBHitRate = (float)TLBhits / (float)numAddresses;
    printf("Page Fault Rate = %.4f\nTLB hit rate = %.4f\n", pageFaultRate, TLBHitRate);

    fclose(fd);
    fclose(outputFile1);
    fclose(outputFile2);
    fclose(outputFile3);
    free(PhyMem);

    return 0;
}
