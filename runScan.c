#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

int main(int argc, char **argv) {

    	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}

	// Creates output directory if non-existent, otherwise error
	if (opendir(argv[2])) {
		//ERROR - already exists
		printf("ERROR - Output directory already exists\n");
		exit(1);
	}
	mkdir(argv[2], 0700); // Creates the new output directory

	// Opens disk images
	int input_file;
	input_file = open(argv[1], O_RDONLY);

	ext2_read_init(input_file); // Prints info about the blocks and nodes in input_file
	printf("\n");

	struct ext2_super_block super;
	struct ext2_group_desc group;

	read_super_block(input_file, 0, &super); //prints uper block info
	printf("\n");
	read_group_desc(input_file, 0, &group); //prints group desc info
	printf("\n");
	
	printf("There are %u inodes in an inode table block and %u blocks in the idnode table\n", inodes_per_block, itable_blocks);
	//iterate the first inode block
	off_t start_inode_table = locate_inode_table(0, &group);
    for (unsigned int i = 0; i < inodes_per_group; i++) { //

            printf("inode %u: \n", i);
            struct ext2_inode *inode = malloc(sizeof(struct ext2_inode));
            read_inode(input_file, 0, start_inode_table, i, inode);
	    /* the maximum index of the i_block array should be computed from i_blocks / ((1024<<s_log_block_size)/512)
			 * or once simplified, i_blocks/(2<<s_log_block_size)
			 * https://www.nongnu.org/ext2-doc/ext2.html#i-blocks
			 */
			unsigned int i_blocks = inode->i_blocks/(2<<super.s_log_block_size);
            printf("number of blocks %u\n", i_blocks);
             printf("Is directory? %s \n Is Regular file? %s\n",
                S_ISDIR(inode->i_mode) ? "true" : "false",
                S_ISREG(inode->i_mode) ? "true" : "false");

			// Read the first block of every node - check is_jpg
			char buffer[block_size];
			lseek(input_file,BLOCK_OFFSET(inode->i_block[0]), SEEK_SET);
			read(input_file,buffer,block_size);

			int is_jpg = 0;
			if (buffer[0] == (char)0xff &&
				buffer[1] == (char)0xd8 &&
				buffer[2] == (char)0xff &&
				(buffer[3] == (char)0xe0 ||
				buffer[3] == (char)0xe1 ||
				buffer[3] == (char)0xe8)) {
				is_jpg = 1;
			}

			if (is_jpg == 1) {

				// sizing for multi-digit inode nums
				int num_digits = 1;
				int count = i;
				while (count /= 10) {
					num_digits++;
				}

				// Set up new file
				int filename_chars = (strlen(argv[2]) + strlen("/") + strlen("file-") + num_digits + strlen(".jpg") + 1);
				size_t name_len = filename_chars * sizeof(char);
				char* filename = malloc(name_len);
				snprintf(filename, name_len, "%s/file-%u.jpg", argv[2], i);
				int file_ptr = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);

				uint filesize = inode->i_size; // Get filesize from the inode
				char file_arr[filesize];

				uint block_id;
				uint bytes_read;
				for (bytes_read = 0, block_id = 0; bytes_read < filesize && bytes_read < block_size * EXT2_NDIR_BLOCKS; bytes_read = bytes_read + block_size) {
					uint i;
					for (i = 0; i < block_size && bytes_read + i < filesize; i++) {
						file_arr[bytes_read + i] = buffer[i];
					}
					if (bytes_read + i < filesize) {
						block_id++;
						lseek(input_file,BLOCK_OFFSET(inode->i_block[block_id]), SEEK_SET);
						read(input_file,buffer,block_size);
					}
				}

				//*
				if (bytes_read < filesize) {
					//printf("iNode: %d has single indirect pointers\n", i);

					uint ind_buffer[block_size / sizeof(uint)]; // TODO

					lseek(input_file,BLOCK_OFFSET(inode->i_block[EXT2_IND_BLOCK]), SEEK_SET);
					read(input_file,ind_buffer,block_size);

					lseek(input_file,BLOCK_OFFSET(ind_buffer[0]), SEEK_SET);
					read(input_file,buffer,block_size);

					for (block_id = 0; bytes_read < filesize && block_id < (block_size / sizeof(uint)); bytes_read = bytes_read + block_size) {

						uint i;
						for (i = 0; i < block_size && bytes_read + i < filesize; i++) {
							file_arr[bytes_read + i] = buffer[i];
						}
						if (bytes_read + i < filesize) {
							block_id++;
							lseek(input_file,BLOCK_OFFSET(ind_buffer[block_id]), SEEK_SET);
							read(input_file,buffer,block_size);
						}
					}
					//put single block handling code here
					//inode->i_block[EXT2_IND_BLOCK];
				} //*/

				if (bytes_read < filesize && 1 == 1) {
					//printf("iNode: %d has double indirect pointers\n", i);
					
					uint first_buffer[block_size / sizeof(uint)];
					uint second_buffer[block_size / sizeof(uint)];

					lseek(input_file,BLOCK_OFFSET(inode->i_block[EXT2_DIND_BLOCK]), SEEK_SET);
					read(input_file,first_buffer,block_size);

					uint first_layer_id;
					for (first_layer_id = 0; first_layer_id < (block_size / sizeof(int)) && bytes_read < filesize; first_layer_id++) {
						lseek(input_file,BLOCK_OFFSET(first_buffer[first_layer_id]), SEEK_SET);
						read(input_file,second_buffer,block_size);

						lseek(input_file,BLOCK_OFFSET(second_buffer[0]), SEEK_SET);
						read(input_file,buffer,block_size);
						for (block_id = 0; bytes_read < filesize && block_id < (block_size / sizeof(int)); bytes_read = bytes_read + block_size) {
							uint i;
							for (i = 0; i < block_size && bytes_read + i < filesize; i++) {
								file_arr[bytes_read + i] = buffer[i];
							}
							if (bytes_read + i < filesize) {
								block_id++;
								lseek(input_file,BLOCK_OFFSET(second_buffer[block_id]), SEEK_SET);
								read(input_file,buffer,block_size);
							}
						}
					}
				}
				write(file_ptr, file_arr, filesize); // Writes file to directory
				//printf("iNode %i is a jpg\n", i);
			}

			// print i_block numbers
			for(unsigned int i=0; i<EXT2_N_BLOCKS; i++)
			{       if (i < EXT2_NDIR_BLOCKS)                  				  /* direct blocks */
						printf("Block %2u : %u\n", i, inode->i_block[i]);
					else if (i == EXT2_IND_BLOCK)                             /* single indirect block */
							printf("Single   : %u\n", inode->i_block[i]);
					else if (i == EXT2_DIND_BLOCK)                            /* double indirect block */
							printf("Double   : %u\n", inode->i_block[i]);
					else if (i == EXT2_TIND_BLOCK)                            /* triple indirect block */
							printf("Triple   : %u\n", inode->i_block[i]);
			}
            free(inode);
        }
}