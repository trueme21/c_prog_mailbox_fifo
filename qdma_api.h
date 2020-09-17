#ifndef QDMA_API_H__
#define QDMA_API_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

///// QDMA API /////
#define QUERY_START_POINT 0x800000000
#define PHYS_ADDR_MAIL 0xa0010000
#define PHYS_ADDR_ACK 0xa0010008
#define HOST_ACK_ADDR 0x10008  // 0xa0010008 in NDP

#define PHYS_ADDR_FWD_OUTPUT 0x80028C000
#define PHYS_ADDR_EMB_TABLE 0x8002CD000
#define SIZE_ALIGN 4096

#define is_aligned(POINTER) \
    (((unsigned long)(const void *)(POINTER)) % (SIZE_ALIGN) == 0)

int static compare (const void* first, const void* second)
{
    if (*(long*)first > *(long*)second)
        return 1;
    else if (*(long*)first < *(long*)second)
        return -1;
    else
        return 0;
}

int ack_polling(int fd, off_t mem_offset) {
    unsigned int *read_buf;
    read_buf = (unsigned int*) aligned_alloc(SIZE_ALIGN, 4);
    unsigned int read_buf_tmp;
    printf("start polling\n");
    for (;;) {
        sleep(1);
        lseek(fd, mem_offset, SEEK_SET);
        read(fd, read_buf, 4);
        read_buf_tmp = read_buf[0];
        if ((read_buf_tmp & 0x01) == 1) {
            break;
        }
    }
    free(read_buf);
    return (read_buf_tmp >> 16);  // return task id; always > 0
}

int ack_polling_bar(off_t host_mem_offset) {
    const char* read_fd_name = "/sys/bus/pci/devices/0000:84:00.0/resource2";
    int fd;
    uint32_t read_buf_tmp = 0;
    void* mem;
    if((fd = open(read_fd_name, O_RDWR|O_SYNC)) == -1){
        printf("Host: Can't open ACK fd\n");
        return -1;
    }

    mem = (void*) mmap((void *)0x0, 0x00040000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("Host: Can't map ACK memory");
        return -2;
    }

    printf("start polling (bar)\n");
    for(;;) {
        sleep(1);
        read_buf_tmp = *((uint32_t*)((off_t)mem + host_mem_offset));
        if (1 == (read_buf_tmp & 0x01)) {
            break;
        }
    }

    munmap(mem, 0x00040000);
    close(fd);

    return (read_buf_tmp >> 16);  // return task id; always > 0
}

template<typename T>
void read_block_data(int fd, off_t mem_offset, unsigned int size1, T* rdata) {
    lseek(fd, mem_offset, SEEK_SET);
    if(is_aligned(rdata))
        read(fd, rdata, size1 * sizeof(T));
    return;
}

template<typename T>
void write_block_bulk_data(int fd, off_t mem_offset, unsigned int size1, T* wdata) {
    lseek(fd, mem_offset, SEEK_SET);
    // if(is_aligned(wdata))
        write(fd, wdata, sizeof(T) * size1);
    return;
}

// For offset+indices
template<typename T>
void write_block_bulk_concat2(int fd, off_t mem_offset, 
                            unsigned int size1, T* wdata1, 
                            unsigned int size2, T* wdata2) {
    lseek(fd, mem_offset, SEEK_SET);
    // if(is_aligned(wdata1) && is_aligned(wdata2)) {
        write(fd, wdata1, sizeof(T) * size1);
        write(fd, wdata2, sizeof(T) * size2);
    // }
    return;
}

void write_block_data_msg4b(int fd, off_t mem_offset, unsigned int wdata) {
    lseek(fd, mem_offset, SEEK_SET);
    unsigned int *write_buf;
    write_buf = (unsigned int*) aligned_alloc(SIZE_ALIGN, 4);
    write_buf[0] = (unsigned int) wdata;
    write(fd, write_buf, 4);
    free(write_buf);
}
///////////////////

#endif
