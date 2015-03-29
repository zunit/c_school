#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;


// this transforms it into bits
void byte_to_bits(unsigned char *bytes){
	//all original code, no copy and paste from old assignments, no googling a solution :-)
	printf(" ");
	unsigned char value = *bytes;
	char temp;
	temp = value & 1 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 2 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 4 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 8 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 16 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 32 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 64 ? '1' : '0';
	printf ("%c", temp);
	temp = value & 128 ? '1' : '0';
	printf ("%c", temp);
}

// helper function for checking the flag type either file or dir
char typecheck(int flag){
		char type;
		if(flag & EXT2_S_IFREG){
			type = 'f';
		}
		else if (flag & EXT2_S_IFDIR){
			type = 'd';
		} else {
			type = 'q';
		}
		return type;
	}
	
// helper function for printing out data blocks
void printdatablocks(unsigned int *i_block){
	int i = 0;
	while (i_block[i]){
		printf("%u ", i_block[i]);
		i++;
	}
}

// this prints out all valid inodes
void print_valid_inodes(void *tablelocation, int i){
	struct ext2_inode *inode = (struct ext2_inode*)(tablelocation)+(i-1);
	// something valid at that inode block			
	int type = typecheck(inode->i_mode);  
	int size = inode->i_size;
	int links = inode->i_links_count;
	int iblocks = inode->i_blocks;
	printf("[%d] type: %c size: %d links: %d blocks: %d\n", i, type, size, links, iblocks);
	printf("[%d] blocks: ", i);
	printdatablocks(inode->i_block);
	printf("\n");
		
	
}



int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: readimg <image file name>\n");
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	perror("mmap");
	exit(1);
    }
	
	// super block 
	// second block 
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    
    struct ext2_group_desc *group_desc_table = (struct ext2_group_desc *)(disk + (1024 * 2));
    int inode_bitmap = group_desc_table->bg_inode_bitmap;
    int block_bitmap = group_desc_table->bg_block_bitmap;
    int inode_table = group_desc_table->bg_inode_table;
    printf("Block group: \n");
    printf("\t block bitmap: %d\n", block_bitmap);
    printf("\t inode bitmap: %d\n", inode_bitmap);
    printf("\t inode table: %d\n", inode_table);
    printf("\t free block: %d\n", group_desc_table->bg_free_blocks_count);
    printf("\t free inodes: %d\n", group_desc_table->bg_free_inodes_count);
    printf("\t used_dirs: %d\n", group_desc_table->bg_used_dirs_count); 
    printf("Block bitmap:");
    
    // starts at 3072 at c00  
    int start = EXT2_BLOCK_SIZE * block_bitmap;
    int total = sb->s_blocks_count / 8; // 8 bits in a byte. 16 groups 
    int i;
    for (i = 0; i < total; i++){
		byte_to_bits(disk+start+i);
	}
    printf("\n");
 
    printf("Inode bitmap:");
    // starts at 4096
    int total_inode = 4	;
    int start_inode = EXT2_BLOCK_SIZE *  inode_bitmap;
    int j;
    for (j = 0; j  < total_inode; j++){
		byte_to_bits(disk+start_inode+j);
	}
    printf("\n\n");
    printf("Inodes:\n");
    // find out where the table starts
    void *tablestart = disk + EXT2_BLOCK_SIZE*inode_table;
    // we are hard coding the root inode since it's always 2
    // we plus one to indicate the start of root inode and end of the first inode
    struct ext2_inode *rootinode = (struct ext2_inode*)(tablestart)+1;
    int type = typecheck(rootinode->i_mode);  
    int size = rootinode->i_size;
    int links = rootinode->i_links_count;
    int iblocks = rootinode->i_blocks;
    printf("[2] type: %c size: %d links: %d blocks: %d\n", type, size, links, iblocks);
    printf("[2] blocks:");
    printdatablocks(rootinode->i_block);
    printf("\n");
    
    // below are for inode block after 11
    
    // need another pointer that starts at the beginning of the inode_bitmap
    void *inode_bit_start = disk + start_inode;
    int inode_bits = *((int*)inode_bit_start);
    inode_bits = inode_bits >> 11; 
    
    int k;
    
    //we start at 11 because index 0 counts
    for (k = 11; k < 32; k++){
		if(inode_bits & 1){
			print_valid_inodes(tablestart, k+1);
		}
		inode_bits = inode_bits >> 1; 
	}
    
    return 0;
    
    
   
}
