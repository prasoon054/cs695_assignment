#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <num_pages> <stride>\n", argv[0]);
        return 1;
    }
    int num_pages = atoi(argv[1]);
    int stride = atoi(argv[2]);
    const ssize_t PAGE_SIZE = sysconf(_SC_PAGESIZE);
    if(num_pages <= 0 || stride <= 0){
        fprintf(stderr, "Invalid arguments: number of pages and stride size should be positive\n");
        return 1;
    }
    size_t reg_size = num_pages*PAGE_SIZE;
    // size_t reg_size = num_pages;
    size_t stride_size = stride;
    void *region = mmap(NULL, reg_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(region==MAP_FAILED){
        perror("mmap");
        return 1;
    }
    for(size_t ofst = 0; ofst < reg_size; ofst += stride_size){
        volatile char *ptr = (char*)region + ofst;
        *ptr = 42;
    }
    printf("PID of process created by test1.c: %d\n", getpid());
    printf("Done processing memory. Press ENTER to exit :)\n");
    getchar();
    munmap(region, reg_size);
    return 0;
}
