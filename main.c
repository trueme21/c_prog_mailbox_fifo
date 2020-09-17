#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "ndp_env.h"

#define NDP_COMPILE

#ifdef NDP_COMPILE
    #include "include/ndp_sls.h"
#endif

int main()
{
    size_t pagesize = sysconf(_SC_PAGE_SIZE);
    int mbox_fd = open("/dev/mem", O_RDWR | O_SYNC);
    int queries_fd;
    int output_fd = open("/dev/mem", O_RDWR | O_SYNC);
    int emb_fd;

    // Allocate CMD space
    // Truncate offset to a multiple of the page size, or mmap will fail.
    off_t mbox_page_base = (MBOX_START_ADDR / pagesize) * pagesize;
    printf("mbox_page_base = %08x\n", mbox_page_base);
    
    off_t mbox_page_offset = MBOX_START_ADDR - mbox_page_base;
    unsigned int *mbox_mem = (unsigned int*)mmap(NULL, mbox_page_offset + MBOX_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mbox_fd, mbox_page_base);
    if (mbox_mem == MAP_FAILED) {
        perror("Can't map CMD memory");
        return -2;
    }
    
    unsigned int mbox_mem_t1;

    // Allocate Output space 
    off_t output_page_base = (OUTPUT_START_ADDR / pagesize) * pagesize;
    off_t output_page_offset = OUTPUT_START_ADDR - output_page_base;
    float *output_mem = (float*)mmap(NULL, output_page_offset + OUTPUT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, output_fd, output_page_base);
    if (output_mem == MAP_FAILED) {
        perror("Can't map OUTPUT memory");
        return -2;
    }

    float* emb_mem = NULL;
    
    /*
     * mbox_mem[mbox_offset] => read mailbox (msg came from the host)
     * mbox_mem[ack_offset] => write mailbox to send ack to the host
     */
    unsigned int mbox_offset = mbox_page_offset / sizeof(int) + 2;
    unsigned int ack_offset = mbox_page_offset / sizeof(int);
    unsigned int output_offset = output_page_offset / sizeof(float);

    unsigned int embedding_num;
    unsigned int dimension;
    unsigned int emb_size;
    off_t emb_page_offset;
    unsigned int embed_offset;

    bool emb_initialized = false;
	

    printf("Allocation Completed.\n");
    for(;;) {
        sleep(1);
        // mbox_mem_t1: address offset
		// mbox_mem[mbox_offset + 2] => 0xa0020010
        if (MBOX_EMPTY != (mbox_mem[mbox_offset + 2] & 0x00000001)) {  // Now query always exists in this section
            // mmap query and parse
            mbox_mem_t1 = mbox_mem[mbox_offset];  // Can be First part of mailbox
            queries_fd = open("/dev/mem", O_RDWR | O_SYNC);
            off_t query_start_addr = QUERY_START_POINT + mbox_mem_t1;
            off_t queries_page_base = (query_start_addr / pagesize) * pagesize;
            off_t queries_page_offset = query_start_addr - queries_page_base;
            long *queries_mem = (long*)mmap(NULL, queries_page_offset + sizeof(long), PROT_READ | PROT_WRITE, MAP_SHARED, queries_fd, queries_page_base);  // single size long
            if (queries_mem == MAP_FAILED) {
                perror("Can't map QUERIES memory");
                return -2;
            }
            unsigned int queries_offset = queries_page_offset / sizeof(long);
            // Parse first two lines: 8 byte
            // check opcode
            long query_header = queries_mem[queries_offset];
			unsigned int opcode = query_header & 0x0000FFFF;
			unsigned int task_id = (query_header >> 16) & 0x0000FFFF;
            if ((!emb_initialized) && ((opcode == OP_INIT) || (opcode == OP_INIT_CPY))) {
				// Initialization(opcode 1), copy weight from host (opcode 5)
                emb_fd = open("/dev/mem", O_RDWR | O_SYNC);
                printf("EMB Initialization Start...\n");
                /*
                 *   31                                                       0
                 *   Reserved (16bit)              |             Opcode (16bit)
                 *   Reserved (3bit) | Dimension (8bit) | Embedding Num (21bit)
                 */
                embedding_num = (query_header >> 32) & 0x001FFFFF;
                dimension = (query_header >> (32 + 21)) & 0x000000FF;
                emb_size = (unsigned int) (embedding_num * dimension);

                // Allocate Embedding table space
                off_t emb_page_base = (EMB_START_ADDR / pagesize) * pagesize;
                emb_page_offset = EMB_START_ADDR - emb_page_base;
                embed_offset = emb_page_offset / sizeof(float);

                emb_mem = (float*)mmap(NULL, emb_page_offset + sizeof(float) * emb_size, PROT_READ | PROT_WRITE, MAP_SHARED, emb_fd, emb_page_base);
                if (emb_mem == MAP_FAILED) {
                    perror("Can't map EMB memory");
                    return -2;
                }
                // Initialization emb table with random
                //int rnd=open("/dev/urandom", O_RDONLY);
                //read(2 * ((float)rnd / (float)(RAND_MAX)) - 1, emb_mem, sizeof(float)*emb_size); // -1 ~ 1
                //close(rnd);
				if (opcode == OP_INIT) {
					printf("RAND Initialization...\n");
					for (int jj = 0 ; jj < emb_size ; ++jj) {
						emb_mem[embed_offset + jj] = (float)rand()/(float)(RAND_MAX);  // plug in embedding size
					}
				}
                emb_initialized = true;
                munmap(queries_mem, (queries_page_offset+8));
                close(queries_fd);

                mbox_mem[ack_offset] = (0x00000001) + (task_id << 16);
                printf("EMB Initialization Finished...\n");
            }
            else if (emb_initialized && (opcode == OP_FWD)) {  // Forward
                printf("Inference Start...\n");
                unsigned int offset_size = (query_header >> 32) & 0x000007FF;  // 11 bit
                unsigned int indices_size = (query_header >> (32 + 11)) & 0x0001FFFF;  // 17bit
                
                // Get payload 1
                int query_pl1_fd = open("/dev/mem", O_RDWR | O_SYNC);
                off_t query_pl1_page_base = ((query_start_addr + 8) / pagesize) * pagesize;
                off_t query_pl1_page_offset = (query_start_addr + 8) - query_pl1_page_base;
                unsigned int query_pl1_offset = query_pl1_page_offset / sizeof(long);
                long* query_pl1 = (long*)mmap(NULL, query_pl1_page_offset + sizeof(long) * (indices_size + offset_size), PROT_READ | PROT_WRITE, MAP_SHARED, query_pl1_fd, query_pl1_page_base);
                if (query_pl1 == MAP_FAILED) {
                    perror("Can't map PAYLOAD1 memory");
                    return -2;
                }

                // Don't have to parse dimension... already got from emb initialization process
#ifdef NDP_COMPILE
                // Payload: offset + indices
                embedding_forward_simd(emb_mem + embed_offset, 
                                    query_pl1 + query_pl1_offset + offset_size,  // indices, long
                                    query_pl1 + query_pl1_offset,  // offset start address
                                    0, 0, 0, dimension, offset_size,
                                    0, 0, indices_size, 
                                    output_mem + output_offset);
#endif
                munmap(queries_mem, (queries_page_offset+sizeof(long)));
                munmap(query_pl1, (query_pl1_page_offset+sizeof(long) * (indices_size + offset_size)));
                close(queries_fd);
                close(query_pl1_fd);
                
                mbox_mem[ack_offset] = (0x00000001) + (task_id << 16);
                printf("Inference Finished...\n");
            }

            else if (emb_initialized && (opcode == OP_BWD)) {  // Backward
                printf("Backward Start...\n");
                unsigned int task_id = (query_header >> 16) & 0x0000FFFF;
                unsigned int offset_size = (query_header >> 32) & 0x000007FF;  // 11 bit
                unsigned int indices_size = (query_header >> (32 + 11)) & 0x0001FFFF;  // 17bit

                // Get payload 1
                int query_pl1_fd = open("/dev/mem", O_RDWR | O_SYNC);
                off_t query_pl1_page_base = ((query_start_addr + sizeof(long)) / pagesize) * pagesize;
                off_t query_pl1_page_offset = (query_start_addr + sizeof(long)) - query_pl1_page_base;
                unsigned int query_pl1_offset = query_pl1_page_offset / sizeof(long);
                long* query_pl1 = (long*)mmap(NULL, query_pl1_page_offset + sizeof(long) * (indices_size + offset_size), PROT_READ | PROT_WRITE, MAP_SHARED, query_pl1_fd, query_pl1_page_base);
                if (query_pl1 == MAP_FAILED) {
                    perror("Can't map PAYLOAD1 memory");
                    return -2;
                }

                // Get grads
                int grad_fd = open("/dev/mem", O_RDWR | O_SYNC);
                off_t grad_start_addr = query_start_addr + sizeof(long) + sizeof(long) * (indices_size + offset_size);
                off_t grad_page_base = (grad_start_addr / pagesize) * pagesize;
                off_t grad_page_offset = grad_start_addr - grad_page_base;
                float* grad_mem = (float*)mmap(NULL, grad_page_offset + sizeof(float) * (offset_size * dimension), PROT_READ | PROT_WRITE, MAP_SHARED, grad_fd, grad_page_base);
                unsigned int grad_offset = grad_page_offset / sizeof(float);
                if (grad_mem == MAP_FAILED) {
                    perror("Can't map GRAD (Payload 2) memory");
                    return -2;
                }

				float lr = 0.01;  // TODO: Parameterize
#ifdef NDP_COMPILE
                grad_coalesce_hash(grad_mem + grad_offset,
                                    query_pl1 + query_pl1_offset +  offset_size, // indices
                                    query_pl1 + query_pl1_offset, // offset
                                    indices_size, offset_size, 
                                    emb_mem + embed_offset, 
                                    dimension, lr);
#endif
                munmap(queries_mem, (queries_page_offset+sizeof(long)));
                munmap(query_pl1, (query_pl1_page_offset+sizeof(long) * (indices_size + offset_size)));
                munmap(grad_mem, (grad_page_offset+sizeof(float) * (offset_size * dimension)));
                close(queries_fd);
                close(query_pl1_fd);
                close(grad_fd);

                mbox_mem[ack_offset] = (0x00000001) + (task_id << 16);
                printf("Backward Finished...\n");
            }

            else if (opcode == OP_KILL) {  // kill switch
                printf("TERMINATION!\n");
                if (emb_initialized) {
                    munmap(emb_mem, (emb_page_offset+emb_size));
                    close(emb_fd);
                }
                munmap(mbox_mem, (mbox_page_offset+MBOX_SIZE));
                munmap(output_mem, (output_page_offset+OUTPUT_SIZE));
                close(mbox_fd);
                close(output_fd);
                return -1;
            }
        }  // if first msg ; mbox not empty
    }
    return 0;
}

