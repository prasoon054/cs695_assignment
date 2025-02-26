#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define DEVICE_FILE "/dev/ioctl_device"
#define IOCTL_MAGIC 'X'
#define IOCTL_VA_TO_PA _IOWR(IOCTL_MAGIC, 1, unsigned long)
#define IOCTL_WRITE_PA _IOW(IOCTL_MAGIC, 2, unsigned long)

struct ioctl_data{
    unsigned long vaddr;
    unsigned int value;
};

int main(){
    int fd = open(DEVICE_FILE, O_RDWR);
    if(fd<0){
        perror("Failed to open device file");
        return 1;
    }
    size_t count = 2;
    unsigned int *val = malloc(count);
    unsigned long *pa = malloc(count);
    unsigned long *va = malloc(count);
    for(size_t i=0; i<count; i++){
        val[i] = 104 + i;
        printf("VA: %p Value: %d\n", &val[i], val[i]);
        va[i] = (unsigned long)&val[i];
        pa[i] = (unsigned long)&val[i];
    }
    struct ioctl_data data;
    printf("VA to PA translation\n");
    for(size_t i=0; i<count; i++){
        // data.vaddr = (unsigned long)&val[i];
        ioctl(fd, IOCTL_VA_TO_PA, &pa[i]);
        printf("VA: %p\tPA: %p\n", &val[i], pa[i]);
    }
    for(size_t i=0; i<count; i++){
        printf("PA: %p\tValue: %d\n", pa[i], val[i]);
    }
    for(size_t i=0; i<count; i++){
        data.vaddr = pa[i];
        data.value = 53 + i;
        ioctl(fd, IOCTL_WRITE_PA, &data);
    }
    for(size_t i=0; i<count; i++){
        printf("VA: %p\tPA: %p\tUpdated Value: %d\n", va[i], pa[i], val[i]);
    }
    free(val);
    close(fd);
    return 0;
}
