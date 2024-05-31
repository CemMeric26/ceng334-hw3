#include <iostream>
#include <vector>
#include <cstdio>
#include <cstring>
#include "ext2fs.h"
#include "ext2fs_print.h"
#include "identifier.h"

using namespace std;

bool checkDataIdentifier(const char* block, const uint8_t* data_identifier, size_t identifier_length) {
    return memcmp(block, data_identifier, identifier_length) == 0;
}

void printDirectoryTree(FILE *file, const ext2_inode &inode, const string &path, int depth, const uint8_t* data_identifier, const vector<ext2_block_group_descriptor> &bgdt, const uint32_t block_size, const ext2_super_block &sb) {
    if (!S_ISDIR(inode.mode)) {
        // cerr << "Not a directory: " << path << endl;
        return;
    }

    vector<char> block(block_size);

    for (int i = 0; i < EXT2_NUM_DIRECT_BLOCKS; ++i) {
        if (inode.direct_blocks[i] == 0) {
            // cout << "Skipping block " << i << " for path " << path << endl;
            continue; 
        }

        // cout << "Reading directory block " << inode.direct_blocks[i] << " for path " << path << endl;
        fseek(file, inode.direct_blocks[i] * block_size, SEEK_SET);
        size_t bytesRead = fread(block.data(), 1, block_size, file);
        if (bytesRead != block_size) {
            // cerr << "Error reading block " << inode.direct_blocks[i] << " at path " << path << endl;
            return;
        }

        int offset = 0;
        while (offset < block_size) {
            ext2_dir_entry *entry = reinterpret_cast<ext2_dir_entry *>(block.data() + offset);
            if (entry->inode) {
                string name(entry->name, entry->name_length);
                for (int j = 0; j < depth; ++j) cout << "-";
                cout << " " << name << (entry->file_type == EXT2_D_DTYPE ? "/" : "") << endl;
                if (entry->file_type == EXT2_D_DTYPE && name != "." && name != "..") {
                    ext2_inode child_inode;
                    uint32_t group = (entry->inode - 1) / sb.inodes_per_group;
                    uint32_t index = (entry->inode - 1) % sb.inodes_per_group;
                    fseek(file, (bgdt[group].inode_table * block_size) + (index * sb.inode_size), SEEK_SET);
                    size_t inodeRead = fread(&child_inode, 1, sizeof(ext2_inode), file);
                    if (inodeRead != sizeof(ext2_inode)) {
                        // cerr << "Error reading inode " << entry->inode << " at path " << path << "/" << name << endl;
                        return;
                    }
                    printDirectoryTree(file, child_inode, path + "/" + name, depth + 1, data_identifier, bgdt, block_size, sb);
                }
            }
            offset += entry->length;
            if (entry->length == 0) break; 
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <ext2 image> <data identifier>" << endl;
        return 1;
    }

    FILE *file = fopen(argv[1], "rb+");
    if (file == NULL) {
        perror("Failed to open image");
        return 1;
    }

    uint8_t *data_identifier = parse_identifier(argc, argv);

    ext2_super_block sb;
    fseek(file, EXT2_SUPER_BLOCK_POSITION, SEEK_SET);
    fread(&sb, sizeof(ext2_super_block), 1, file);
    // cout << "superblock read, Block size: " << EXT2_UNLOG(sb.log_block_size) << " bytes" << endl;

    uint32_t block_size = EXT2_UNLOG(sb.log_block_size);
    size_t num_block_groups = (sb.block_count + sb.blocks_per_group - 1) / sb.blocks_per_group;
    vector<ext2_block_group_descriptor> bgdt(num_block_groups);
    fseek(file, EXT2_SUPER_BLOCK_POSITION + EXT2_SUPER_BLOCK_SIZE, SEEK_SET);
    fread(bgdt.data(), sizeof(ext2_block_group_descriptor), num_block_groups, file);
    // cout << "block group descriptor table read" << endl;

    vector<ext2_inode> inodes(sb.inodes_per_group);

    for (uint32_t i = 0; i < num_block_groups; i++) {
        fseek(file, bgdt[i].inode_table * block_size, SEEK_SET);
        fread(inodes.data(), sizeof(ext2_inode), sb.inodes_per_group, file);
    }
    // cout << "inode tables read" << endl;


    ext2_inode root_inode;
    fseek(file, (bgdt[0].inode_table * block_size) + ((EXT2_ROOT_INODE - 1) * sb.inode_size), SEEK_SET);
    fread(&root_inode, sizeof(ext2_inode), 1, file);

    /* cout << "Root inode read" << endl;
    cout << "Root inode mode: " << hex << root_inode.mode << dec << endl; // Print mode in hex
    cout << "Root inode size: " << root_inode.size << endl;
    cout << "Root inode direct blocks: "; */
    /* for (int i = 0; i < EXT2_NUM_DIRECT_BLOCKS; ++i) {
        cout << root_inode.direct_blocks[i] << " ";
    }
    cout << endl; */

    printDirectoryTree(file, root_inode, "/", 1, data_identifier, bgdt, block_size, sb);

    delete[] data_identifier;
    fclose(file);
    return 0;
}
