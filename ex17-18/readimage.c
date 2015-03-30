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

// helper for printing out the array 
void print_inode_array(int *inode_array, int size){
	int i;
	for (i = 0; i < size; i++){
		printf("%d ", inode_array[i]);
	}
}

void print_directory_entry(struct ext2_dir_entry_2 *dir_entry){
	printf("DIR BLOCK NUM: %d (for inode %d)\n", 9, dir_entry->inode);
	printf("Inode: %d rec_len: %hu name_len: %d type=%c name=%s\n", dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, dir_entry->file_type, dir_entry->name);
	//printf("%s\n", );
}

// helper for returning the size of data_block
int size_data_block(void *tablelocation, int i){
	struct ext2_inode *inode = (struct ext2_inode*)(tablelocation)+(i-1);
	int k = 0;
	int count = 0;
	while(inode->i_block[k]){
		//count++;
		k++;
	}
	return k;
}

// helper for return a list of data block 
int * create_data_block(void *tablelocation, int i, int length_block){
	struct ext2_inode *inode = (struct ext2_inode*)(tablelocation)+(i-1);
	int k = 0;
	int *array =(int*)malloc(length_block*sizeof(int));
	int block = 0;
	while(inode->i_block[k]){
		array[block] = inode->i_block[k];
		k++;
	}
	return array;
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
    int counter = 0; // this will record how many inode are occupied 

    //we start at 11 because index 0 counts
    for (k = 11; k < 32; k++){
		if(inode_bits & 1){
			print_valid_inodes(tablestart, k+1);
			counter += 1;
		}
		inode_bits = inode_bits >> 1; 
	}
    
    // this is extra 2 for the putting the root and lost+found
    counter += 2;

    printf("counter length: %d\n", counter);
    // the reason we plus 1 here is because of the root
    int occupied_inode[counter];

    // need another pointer that starts at the beginning of the inode_bitmap
    void *inode_bit_array_form = disk + start_inode;
    int ibits_array = *((int*)inode_bit_array_form);
    ibits_array = ibits_array >> 11; 
 
    occupied_inode[0] = 2; // filling in the root inode
    occupied_inode[1] = 11; // filling in the lost+found inode
    int inode_count = 2;
    for (k = 11; k < 32; k++){
		if(ibits_array & 1){
			// this stores the inode that need to be examined
			occupied_inode[inode_count] = k+1;
			inode_count += 1;
		}
		ibits_array = ibits_array >> 1; 
	}

	print_inode_array(occupied_inode, counter);
	printf("\n");
	printf("Directory Blocks:\n");
	
	// loop through the 
	// root starts at 9
	
	int end = 1024;
	int current = 0;

	// root can always be hard coded 
	printf("DIR BLOCK NUM: %d (for inode %d)\n", 9, 2);
	while (current < end){
		struct ext2_dir_entry_2 *root_dir_entry = (struct ext2_dir_entry_2*)(disk + (1024 * 9) + current);
		int inode_num = root_dir_entry->inode;
		unsigned short dir_len = root_dir_entry->rec_len;
		int name_length = root_dir_entry->name_len;
		char file_flag = root_dir_entry->file_type;

		printf("Inode: %d rec_len: %hu name_len: %d type=%c name=%s\n", 
			inode_num, dir_len, name_length, file_flag, root_dir_entry->name);
		current += (int)dir_len;
	}
	/*
	 * go through the bit again and get all the iblocks[i]
	 **/
	void *inode_bit_start_block = disk + start_inode;
    int inode_bits_block = *((int*)inode_bit_start_block);
    inode_bits_block = inode_bits_block >> 11; 
    
    int l;
    int length_block;

    //int list_data_block[length_block]; // this is the list of data blocks
    //we start at 11 because index 0 counts
    for (l = 11; l < 32; l++){
		if(inode_bits_block & 1){
			length_block = size_data_block(tablestart, l+1);
			int *list_data_block;
			// this creates the data_block 
			list_data_block = create_data_block(tablestart, l+1, length_block);
			int m;
			for (m = 0; m<length_block; m++){ // this prints out the data block that we are going into
				printf("printing: %d ", list_data_block[m]);
				// function for print out each content
			} 
			printf("\n");
		}
		inode_bits_block = inode_bits_block >> 1; 
	}
    return 0;
    
    
   
}
