#include "computer_vfs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static computer_sb_t *computer_sb = NULL;

/**
 * 初始化 computer:/ 文件系统
 */
int computer_vfs_init(void) {
    // 分配超级块
    computer_sb = malloc(sizeof(computer_sb_t));
    if (!computer_sb) {
        return -1;
    }
    
    // 初始化超级块
    computer_sb->magic = COMPUTER_FS_MAGIC;
    computer_sb->version = 1;
    computer_sb->next_ino = 1;
    
    // 创建根目录
    computer_sb->root_inode = computer_create_inode(COMPUTER_FILE_TYPE_DIR, "");
    if (!computer_sb->root_inode) {
        free(computer_sb);
        computer_sb = NULL;
        return -1;
    }
    
    printf("Computer:/ 文件系统初始化成功\n");
    return 0;
}

/**
 * 清理文件系统资源
 */
void computer_vfs_cleanup(void) {
    if (computer_sb) {
        // TODO: 递归释放所有 inode
        free(computer_sb);
        computer_sb = NULL;
    }
    printf("Computer:/ 文件系统已清理\n");
}

/**
 * 创建新的 inode
 */
computer_inode_t* computer_create_inode(computer_file_type_t type, const char *name) {
    computer_inode_t *inode = malloc(sizeof(computer_inode_t));
    if (!inode) {
        return NULL;
    }
    
    // 初始化 inode
    memset(inode, 0, sizeof(computer_inode_t));
    inode->ino = computer_sb ? computer_sb->next_ino++ : 1;
    inode->type = type;
    inode->mode = (type == COMPUTER_FILE_TYPE_DIR) ? 0755 : 0644;
    inode->size = 0;
    
    // 设置时间戳 (简化实现)
    inode->atime = inode->mtime = inode->ctime = 0; // TODO: 获取实际时间
    
    return inode;
}

/**
 * 向父目录添加子节点
 */
int computer_add_child(computer_inode_t *parent, computer_inode_t *child) {
    if (!parent || !child || parent->type != COMPUTER_FILE_TYPE_DIR) {
        return -1;
    }
    
    child->parent = parent;
    
    // 添加到子节点链表
    if (!parent->child) {
        parent->child = child;
    } else {
        computer_inode_t *sibling = parent->child;
        while (sibling->next) {
            sibling = sibling->next;
        }
        sibling->next = child;
    }
    
    return 0;
}

/**
 * 在父目录中查找子节点
 */
computer_inode_t* computer_lookup(computer_inode_t *parent, const char *name) {
    if (!parent || !name || parent->type != COMPUTER_FILE_TYPE_DIR) {
        return NULL;
    }
    
    // TODO: 实现名称比较逻辑
    // 这里需要存储文件名信息，当前简化实现
    
    return NULL;
}
