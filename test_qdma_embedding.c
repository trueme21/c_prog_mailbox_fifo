#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <x86intrin.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include "qdma_api.h"

#define DEBUG

#define QUERY1_OFFSET 0x0
#define QUERY2_OFFSET 0xa3000
#define QUERY3_OFFSET 0x146000
#define QUERY4_OFFSET 0x1e9000


/*
static inline bool is_aligned_uint(void *pointer) {
    return (((unsigned int)pointer & (SIZE_ALIGN - 1)) == 0);
}
*/
#define is_aligned_uint(POINTER) \
    (((unsigned long)(const void *)(POINTER)) % (SIZE_ALIGN) == 0)

int test_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    test_main(argc, argv);
    return 0;
}

int test_main(int argc, char *argv[]) {
    if (argc < 2){
        printf("Usage: %s I\n", argv[0]);
        printf("Usage: %s F\n", argv[0]);
        printf("Usage: %s B\n", argv[0]);
        return 0;
    }

    srand((unsigned int)time(NULL));
    
    char cmd = argv[1][0];
    off_t phys_addr_query, phys_addr_mail, phys_addr_ack;
    off_t grad_phys_addr;

    const char *fname_wq = "/dev/qdma84000-MM-0";
    const char *fname_rq = "/dev/qdma84000-MM-0";
    int fd_wq = open(fname_wq, O_WRONLY|O_TRUNC);
    if (fd_wq < 0) {
        printf("QDMA queue open error: %s, %s\n", fname_wq, strerror(errno));
        exit(1);
    }
	int fd_rq = open(fname_rq, O_RDONLY);
	if(fd_rq < 0)
	{
		printf("open error: %s, %s\n", fname_rq, strerror(errno));
		exit(1);
	}

    /////////////////////////////////////
    // Execute Function
    if (cmd == 'I') { // Initialization Emb Table
#ifdef DEBUG
        int emb_num = 10;
        int dimension = 16;
#else
        int emb_num = 1000000;        
        int dimension = 64;
#endif
        printf("emb_num = %d(%#x)\n", emb_num, emb_num);
        // write query
        int task_id = 1;
        long init_query_arr[1] ={1 + (task_id << 16) + (((long)emb_num) << 32) + (((long)dimension) << (32 + 21))};
        write_block_bulk_data<long>(fd_wq, QUERY_START_POINT + QUERY1_OFFSET, 1, init_query_arr);

        // write message on mailbox
        write_block_data_msg4b(fd_wq, PHYS_ADDR_MAIL, QUERY1_OFFSET);
        printf("Init Msg Recorded.\n");
        unsigned int received_task_id = ack_polling_bar(HOST_ACK_ADDR);
        printf("Received ACK from task %u: Init\n", received_task_id);
 
    }

    else if (cmd == 'F') {  // Forward
        // Task id = 1; First query
        int task_id = 1;
#ifdef DEBUG  // offset
        int emb_num = 10;
        int dimension = 16;
        unsigned int offset_size = 3;
        unsigned int indices_size = 7;
#else
        int emb_num = 1000000;        
        int dimension = 64;
        unsigned int offset_size = 1024;
        unsigned int indices_size = 1024*80;
#endif
        long* query1 = (long *) malloc(sizeof(long) * (indices_size + offset_size + 1));

        /////////////////////////////////////////////
        for(unsigned int i = 1; i < indices_size + 1; i++){
#ifdef DEBUG
            query1[i] = (long) (rand() % emb_num);  // indices
#else
            query1[i] = (long) (rand() % (RAND_MAX / emb_num + 1));  // indices
#endif
        }
        for(unsigned int i = indices_size + 1; i < indices_size + offset_size + 1; i++){
            query1[i] = (long) (rand() % indices_size);  // offset
        }
        query1[0] = (2 + (task_id << 16) + (((long)offset_size) << 32) + (((long)indices_size) << (32 + 11)));
        write_block_bulk_data<long>(fd_wq, QUERY_START_POINT + QUERY1_OFFSET, (indices_size + offset_size + 1), query1);
        
        // write message on mailbox
        write_block_data_msg4b(fd_wq, PHYS_ADDR_MAIL, QUERY1_OFFSET);
        printf("task id %d: Forward Msg Recorded.\n", task_id);

        /////////////////////////////////////////////
        task_id = 2;
        for(unsigned int i = 1; i < indices_size + 1; i++){
#ifdef DEBUG
            query1[i] = (long) (rand() % emb_num);  // indices
#else
            query1[i] = (long) (rand() % (RAND_MAX / emb_num + 1));  // indices
#endif
        }
        for(unsigned int i = indices_size + 1; i < indices_size + offset_size + 1; i++){
            query1[i] = (long) (rand() % indices_size);  // offset
        }
        query1[0] = (2 + (task_id << 16) + (((long)offset_size) << 32) + (((long)indices_size) << (32 + 11)));
        write_block_bulk_data<long>(fd_wq, QUERY_START_POINT + QUERY2_OFFSET, (indices_size + offset_size + 1), query1);
        
        // write message on mailbox
        write_block_data_msg4b(fd_wq, PHYS_ADDR_MAIL, QUERY2_OFFSET);
        printf("task id %d: Forward Msg Recorded.\n", task_id);
        /////////////////////////////////////////////
        task_id = 3;
        for(unsigned int i = 1; i < indices_size + 1; i++){
#ifdef DEBUG
            query1[i] = (long) (rand() % emb_num);  // indices
#else
            query1[i] = (long) (rand() % (RAND_MAX / emb_num + 1));  // indices
#endif
        }
        for(unsigned int i = indices_size + 1; i < indices_size + offset_size + 1; i++){
            query1[i] = (long) (rand() % indices_size);  // offset
        }
        query1[0] = (2 + (task_id << 16) + (((long)offset_size) << 32) + (((long)indices_size) << (32 + 11)));
        write_block_bulk_data<long>(fd_wq, QUERY_START_POINT + QUERY3_OFFSET, (indices_size + offset_size + 1), query1);
        
        // write message on mailbox
        write_block_data_msg4b(fd_wq, PHYS_ADDR_MAIL, QUERY3_OFFSET);
        printf("task id %d: Forward Msg Recorded.\n", task_id);
        /////////////////////////////////////////////
        task_id = 4;
        for(unsigned int i = 1; i < indices_size + 1; i++){
#ifdef DEBUG
            query1[i] = (long) (rand() % emb_num);  // indices
#else
            query1[i] = (long) (rand() % (RAND_MAX / emb_num + 1));  // indices
#endif
        }
        for(unsigned int i = indices_size + 1; i < indices_size + offset_size + 1; i++){
            query1[i] = (long) (rand() % indices_size);  // offset
        }
        query1[0] = (2 + (task_id << 16) + (((long)offset_size) << 32) + (((long)indices_size) << (32 + 11)));
        write_block_bulk_data<long>(fd_wq, QUERY_START_POINT + QUERY4_OFFSET, (indices_size + offset_size + 1), query1);
        
        // write message on mailbox
        write_block_data_msg4b(fd_wq, PHYS_ADDR_MAIL, QUERY4_OFFSET);
        printf("task id %d: Forward Msg Recorded.\n", task_id);
        
        
        unsigned int received_task_id = ack_polling_bar(HOST_ACK_ADDR);
        printf("Received ACK from task %u: Forward\n", received_task_id);
        received_task_id = ack_polling_bar(HOST_ACK_ADDR);
        printf("Received ACK from task %u: Forward\n", received_task_id);
        received_task_id = ack_polling_bar(HOST_ACK_ADDR);
        printf("Received ACK from task %u: Forward\n", received_task_id);
        received_task_id = ack_polling_bar(HOST_ACK_ADDR);
        printf("Received ACK from task %u: Forward\n", received_task_id);

        free(query1);
    }

    else if (cmd == 'B') {  // Backward
        // Task id = 1; First query
        int task_id = 1;
#ifdef DEBUG  // offset
        int emb_num = 10;
        int dimension = 16;
        unsigned int offset_size = 3;
        unsigned int indices_size = 7;
#else
        int emb_num = 1000000;        
        int dimension = 64;
        unsigned int offset_size = 1024;
        unsigned int indices_size = 1024*80;
#endif
        long* query1 = (long *) malloc(sizeof(long) * (indices_size + offset_size + 1));
        for(unsigned int i = 1; i < indices_size + 1; i++){
#ifdef DEBUG
            query1[i] = (long) (rand() % emb_num);  // indices
#else
            query1[i] = (long) (rand() % (RAND_MAX / emb_num + 1));  // indices
#endif
        }
        for(unsigned int i = indices_size + 1; i < indices_size + offset_size + 1; i++){
            query1[i] = (long) (rand() % indices_size);  // offset
        }
        query1[0] = (3 + (task_id << 16) + (((long)offset_size) << 32) + (((long)indices_size) << (32 + 11)));
        write_block_bulk_data<long>(fd_wq, QUERY_START_POINT + QUERY1_OFFSET, (indices_size + offset_size + 1), query1);

        // Generate grad; append to the query
        float* grads1 = (float *) malloc(sizeof(float) * (offset_size * dimension));
        for(unsigned int i = 0; i < offset_size * dimension; i++){
            grads1[i] = 2 * ((float)rand()/(float)(RAND_MAX)) - 1;  // ex. weight, grad
        }
        write_block_bulk_data<float>(fd_wq, QUERY_START_POINT + QUERY1_OFFSET + sizeof(long) * (indices_size + offset_size + 1), offset_size * dimension, grads1);

        // write message on mailbox
        write_block_data_msg4b(fd_wq, PHYS_ADDR_MAIL, QUERY1_OFFSET);
        printf("Backward Msg Recorded.\n");

        unsigned int received_task_id = ack_polling_bar(PHYS_ADDR_ACK);
        printf("Received ACK from task %u: Backward\n", received_task_id);

        free(grads1);
        free(query1);
    }

    close(fd_wq);
    close(fd_rq);
    return 0;
}

