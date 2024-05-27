#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include "ext2fs.h"
#include "identifier.h"

using namespace std;

void readSuperBlock(int fd, ext2_super_block &sb) {
    lseek(fd, EXT2_SUPER_BLOCK_POSITION, SEEK_SET);
    read(fd, &sb, sizeof(ext2_super_block));
}

void readBlockGroupDescriptorTable(int fd, const ext2_super_block &sb, vector<ext2_block_group_descriptor> &bgdt) {
    lseek(fd, EXT2_SUPER_BLOCK_POSITION + EXT2_SUPER_BLOCK_SIZE, SEEK_SET);
    bgdt.resize((sb.block_count + sb.blocks_per_group - 1) / sb.blocks_per_group);
    read(fd, bgdt.data(), bgdt.size() * sizeof(ext2_block_group_descriptor));
}

void readInodeTable(int fd, const ext2_block_group_descriptor &bgd, vector<ext2_inode> &inodes, uint32_t inodesPerGroup, uint32_t inodeSize) {
    lseek(fd, bgd.inode_table * EXT2_BLOCK_SIZE, SEEK_SET);
    inodes.resize(inodesPerGroup);
    read(fd, inodes.data(), inodesPerGroup * inodeSize);
}

void printDirectoryTree(int fd, const ext2_inode &inode, const string &path, int depth) {
    if (!S_ISDIR(inode.mode)) return;

    vector<char> block(EXT2_BLOCK_SIZE);
    for (int i = 0; i < EXT2_NUM_DIRECT_BLOCKS && inode.direct_blocks[i]; ++i) {
        lseek(fd, inode.direct_blocks[i] * EXT2_BLOCK_SIZE, SEEK_SET);
        read(fd, block.data(), EXT2_BLOCK_SIZE);
        
        ext2_dir_entry *entry = reinterpret_cast<ext2_dir_entry *>(block.data());
        int offset = 0;
        while (offset < EXT2_BLOCK_SIZE) {
            if (entry->inode) {
                string name(entry->name, entry->name_length);
                for (int j = 0; j < depth; ++j) cout << "-";
                cout << " " << name << (entry->file_type == EXT2_D_DTYPE ? "/" : "") << endl;
                if (entry->file_type == EXT2_D_DTYPE && name != "." && name != "..") {
                    ext2_inode child_inode;
                    lseek(fd, (entry->inode - 1) * sizeof(ext2_inode), SEEK_SET);  // inode numbers are 1-based
                    read(fd, &child_inode, sizeof(ext2_inode));
                    printDirectoryTree(fd, child_inode, path + "/" + name, depth + 1);
                }
            }
            offset += entry->length;
            entry = reinterpret_cast<ext2_dir_entry *>(block.data() + offset);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <ext2 image> <data identifier>" << endl;
        return 1;
    }

    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("Failed to open image");
        return 1;
    }

    uint8_t *data_identifier = parse_identifier(argc, argv);

    ext2_super_block sb;
    readSuperBlock(fd, sb);

    vector<ext2_block_group_descriptor> bgdt;
    readBlockGroupDescriptorTable(fd, sb, bgdt);

    uint32_t block_size = EXT2_UNLOG(sb.log_block_size);
    vector<ext2_inode> inodes(sb.inodes_per_group);

    for (uint32_t i = 0; i < bgdt.size(); i++) {
        readInodeTable(fd, bgdt[i], inodes, sb.inodes_per_group, EXT2_INODE_SIZE);
    }

    // Assuming root inode is always inode 2
    ext2_inode root_inode;
    lseek(fd, (EXT2_ROOT_INODE - 1) * EXT2_INODE_SIZE, SEEK_SET);
    read(fd, &root_inode, sizeof(ext2_inode));

    printDirectoryTree(fd, root_inode, "/", 1);

    free(data_identifier);
    close(fd);
    return 0;
}

// running the program:
// ./recext2fs ./example.img 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00