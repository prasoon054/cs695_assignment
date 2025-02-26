#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

void set_thp(const char* state){
    FILE *fp = fopen("/sys/kernel/mm/transparent_hugepage/enabled", "w");
    if(!fp){
        perror("Failed to modify the THP state");
        exit(1);
    }
    fprintf(fp, "%s\n", state);
    fclose(fp);
}

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <size_in_MB> <thp_state: enable|disable|leave>\n", argv[0]);
        return 1;
    }
    ssize_t size_in_mb = atoi(argv[1]);
    const char *thp_state = argv[2];
    if(strcmp(thp_state, "enable")==0){
        set_thp("always");
    }
    else if(strcmp(thp_state, "disable")==0){
        set_thp("never");
    }
    ssize_t size_in_bytes = size_in_mb << 20;
    void *region = mmap(NULL, size_in_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // madvise(region, size_in_bytes, MADV_HUGEPAGE);
    if(region==MAP_FAILED){
        perror("mmap");
        return 1;
    }
    if(madvise(region, size_in_bytes, MADV_HUGEPAGE) != 0){
        perror("madvise");
        munmap(region, size_in_bytes);
        return 1;
    }
    memset(region, 42, size_in_bytes);
    printf("Memory allocated. PID of current process: %d\n", getpid());
    printf("Press ENTER to release the memory\n");
    getchar();
    munmap(region, size_in_bytes);
    return 0;
}