#ifndef COMPUTER_VFS_H
#define COMPUTER_VFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

// 文件系统类型定义
#define COMPUTER_FS_MAGIC 0x434F4D50  // "COMP"

// 文件类型
typedef enum {
    COMPUTER_FILE_TYPE_DIR = 1,
    COMPUTER_FILE_TYPE_REGULAR,
    COMPUTER_FILE_TYPE_STATUS,     // 只读状态文件
    COMPUTER_FILE_TYPE_CONTROL,    // 可写控制文件
    COMPUTER_FILE_TYPE_SYMLINK
} computer_file_type_t;

// 文件节点结构
typedef struct computer_inode {
    uint32_t ino;                  // inode 号
    computer_file_type_t type;     // 文件类型
    uint32_t mode;                 // 权限模式
    uint32_t size;                 // 文件大小
    uint32_t atime, mtime, ctime;  // 时间戳
    
    // 设备相关信息
    void *device_data;             // 指向设备数据的指针
    int (*read_func)(struct computer_inode *inode, char *buffer, size_t size, off_t offset);
    int (*write_func)(struct computer_inode *inode, const char *buffer, size_t size, off_t offset);
    
    struct computer_inode *parent; // 父目录
    struct computer_inode *next;   // 兄弟节点
    struct computer_inode *child;  // 子节点
} computer_inode_t;

// 文件系统超级块
typedef struct computer_sb {
    uint32_t magic;                // 魔数
    uint32_t version;              // 版本号
    computer_inode_t *root_inode;  // 根 inode
    uint32_t next_ino;             // 下一个可用的 inode 号
} computer_sb_t;

// 函数声明
int computer_vfs_init(void);
void computer_vfs_cleanup(void);
computer_inode_t* computer_create_inode(computer_file_type_t type, const char *name);
int computer_add_child(computer_inode_t *parent, computer_inode_t *child);
computer_inode_t* computer_lookup(computer_inode_t *parent, const char *name);

#endif // COMPUTER_VFS_H
