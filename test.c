#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_PRINT 32

int test_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	test_main(argc, argv);

	return 0;
}

int test_main(int argc, char *argv[])
{
	char cmd;
	off_t phys_addr;
	size_t size;
	size_t len;
	off_t data;
	int state;

	if (argc < 4){
		printf("Usage: %s <phys_addr> R <size>\n", argv[0]);
		printf("Usage: %s <phys_addr> W <4B data>\n", argv[0]);
		printf("Usage: %s <phys_addr> P <4B data> <size>\n", argv[0]);
		printf("Usage: %s <phys_addr> I <size>\n", argv[0]);
		printf("Usage: %s <phys_addr> S <data>\n", argv[0]);
		return 0;
	}
	
	phys_addr = strtoul(argv[1], NULL, 0);

	cmd = argv[2][0];
	if(cmd == 'r'){
		cmd = 'R';	// Read
	}
	else if(cmd == 'w'){
		cmd = 'W';	// 4B Write
	}
	else if(cmd == 'p'){
		cmd = 'P';	// 4B Pattern Write
	}
	else if(cmd == 'i'){
		cmd = 'I';	// Incremental Write
	}
	else if(cmd == 's'){
		cmd = 'S';	// Scenario Test
	}

	if(cmd == 'R'){
		size = strtoul(argv[3], NULL, 0);
		len = size / sizeof(int);
	}
	else if(cmd == 'W'){
		size = 4;
		len = 1;
		data = strtoul(argv[3], NULL, 0);
	}
	else if(cmd == 'P'){
		data = strtoul(argv[3], NULL, 0);
		size = strtoul(argv[4], NULL, 0);
		len = size / 4;
	}
	else if(cmd == 'I'){
		size = strtoul(argv[3], NULL, 0);
		len = size / sizeof(int);
	}
	else if(cmd == 'S'){
		size = 4;
		len = 1;
		data = strtoul(argv[3], NULL, 0);
	}


	// Truncate offset to a multiple of the page size, or mmap will fail.
	size_t pagesize = sysconf(_SC_PAGE_SIZE);
	off_t page_base = (phys_addr / pagesize) * pagesize;
	off_t page_offset = phys_addr - page_base;

	//
	printf(" $ phys_addr = %#lx\n", phys_addr);
	printf(" $ size = %ld B\n", (long)size);
	printf(" $ len = %ld\n", (long)len);
	printf(" $ pagesize = %ld B\n", (long)pagesize);
	printf(" $ page_base = %#lx\n", page_base);
	printf(" $ page_offset = %#lx\n\n", page_offset);
	//


	//
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	unsigned int *mem = mmap(NULL, page_offset + size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, page_base);
	if (mem == MAP_FAILED) {
		perror("Can't map memory");
		return -1;
	}

	size_t i;
	//int i;
	int j;
	int offset = page_offset / sizeof(int);

	if(cmd == 'W'){
		mem[offset] = data;
	}
	else if(cmd == 'P'){
		for(i=0; i<len; i++){
			mem[offset+i] = data;
		}
	}
	else if(cmd == 'I'){
		for(i=0; i<len; i++){
			if(i%2 == 0){
				mem[offset+i] = i >> 1;
			}
			else{
				mem[offset+i] = 0;
			}				
		}
	}		
	else if(cmd == 'S'){
		state = 0;
		printf("state = %d\n", state);
		//while(1){
		for(j=0; j<16; j++){
			printf("j=%d\n", j);
			if(mem[offset] == data){
				state = 1;
				break;
			}
			sleep(1);
		}

		// SLS operation start!
		state = 3;
		printf("state = %d\n", state);
		sleep(1);
		
		// SLS done!
		state = 2;
		printf("state = %d\n", state);
	}

	off_t tmp;
	if(len <= MAX_PRINT){
		for (i=0, tmp=phys_addr; i<len; ++i, tmp+=sizeof(int)){
			printf("mem[%#lx]: %08x\n", tmp, mem[offset+i]);
		}
	}
	else{
		for (i=0, tmp=phys_addr; i<MAX_PRINT; ++i, tmp+=sizeof(int)){
			printf("mem[%#lx]: %08x\n", tmp, mem[offset+i]);
		}
		printf("...\n");
		
		i = len-2;
		tmp = phys_addr + (i * sizeof(int));
		printf("mem[%#lx]: %08x\n", tmp, mem[offset+i]);
		i = len-1;
		tmp = phys_addr + (i * sizeof(int));
		printf("mem[%#lx]: %08x\n", tmp, mem[offset+i]);
	}
	printf("\n");


	close(fd);
	munmap(mem, (page_offset+size));

	return 0;
}

