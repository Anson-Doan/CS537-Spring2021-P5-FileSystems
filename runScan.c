#include <stdio.h>
#include "ext2_fs.h"
#include "read_ext2.h"
#include <sys/types.h>
#include <dirent.h>

int main(int argc, char **argv) {

    	if (argc != 3) {
		printf("expected usage: ./runscan inputfile outputfile\n");
		exit(0);
	}

	// // Creates output directory if non-existent, otherwise error TODO: Uncomment
	// if (opendir(argv[2])) {
	// 	//ERROR - already exists
	// 	printf("ERROR - Output directory already exists\n");
	// 	exit(1);
	// }
	// mkdir(argv[2], 0700); // Creates the new output directory

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
    for (unsigned int i = 0; i < 17; i++) { //inodes_per_group


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

				char file_arr[inode->i_size];

				// Get filesize from the inode
				// Set up a counter i = 0 that represents the current byte you're reading
				// Set up a pointer b that points to the current block
				// Set up a pointer j = 0 that points to your place in block b
				// while i < filesize:
				// copy b[j] into file_arr[i]
				// increment i and j by 1
				// if j > blocksize:
				// set j = 0
				// set b = b->next

				//copy array to new directory

				printf("iNode %i is a jpg\n", i);
			}


			// print i_block numbers
			for(unsigned int i=0; i<EXT2_N_BLOCKS; i++)
			{       if (i < EXT2_NDIR_BLOCKS) {                  
							/* direct blocks */
						printf("Block %2u : %u\n", i, inode->i_block[i]);
						}

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