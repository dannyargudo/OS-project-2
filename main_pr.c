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
#define PHYSICALMEMORYSIZE 32
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
   queue->head = -1;
   queue->tail = -1;
   queue->qArray = (int*)malloc(MAX * sizeof(int));
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
       return;
   if(qEmpty(queue))
       queue->head = 0;
   queue->tail = (queue->tail + 1)% MAX;
   queue->qArray[queue->tail] = element;   
}


//dequeue an element in the queue
int dequeue(struct fifoQueue* queue, int MAX)
{
   if(qEmpty(queue))
       return -1;
   int element = queue ->qArray[queue->head];
   if(queue->head == queue->tail)
   {
       queue->head = -1;
       queue->tail = -1;
   }
   else
       queue->head = (queue->head+1) % MAX;
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


   for (int i = 0; i < PHYSICALMEMORYSIZE; i++) {
       physMem[(*curFrame) * PHYSICALMEMORYSIZE + i] = buffer[i];
   }


   (*curFrame)++;


   return (*curFrame) - 1;
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
               PT[removedP].valid = false;
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


       tlb->TLBpage[tlb->ind] = pageNum;
       tlb->TLBframe[tlb->ind] = frame;
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


   char PhyMem[PHYSICALMEMORYSIZE][PHYSICALMEMORYSIZE];


   FILE* fd = fopen(argv[1], "r");


   FILE* outputFile1 = fopen(OUT_FILE_1, "w");
   FILE* outputFile2 = fopen(OUT_FILE_2, "w");
   FILE* outputFile3 = fopen(OUT_FILE_3, "w");


   int openFrame = 0;
   int numPageFaults = 0;
   int TLBhits = 0;
   int numAddresses = 0;
   struct fifoQueue* pQueue = newQueue(PHYSICALMEMORYSIZE);


   //read file, call find_page for each address
   while (fscanf(fd, "%d", &val) == 1) {
       find_page(val, PageTable, &tlb, (char*)PhyMem, &openFrame, &numPageFaults, &TLBhits, pQueue, PHYSICALMEMORYSIZE, outputFile1, outputFile2, outputFile3);
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
