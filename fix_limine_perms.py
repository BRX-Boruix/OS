#!/usr/bin/env python3
"""
修复ELF文件中.limine_requests段的权限
找到包含.limine_requests的段并确保它是RW
"""

import sys
import struct

def fix_elf_permissions(elf_path):
    with open(elf_path, 'r+b') as f:
        # 读取ELF头
        elf_header = f.read(64)
        
        # 检查是否为ELF64
        if elf_header[4] != 2:
            print("Not an ELF64 file!")
            return False
        
        # 获取program header偏移和数量
        phoff = struct.unpack('<Q', elf_header[32:40])[0]
        phnum = struct.unpack('<H', elf_header[56:58])[0]
        
        # 获取section header偏移和数量
        shoff = struct.unpack('<Q', elf_header[40:48])[0]
        shnum = struct.unpack('<H', elf_header[60:62])[0]
        shstrndx = struct.unpack('<H', elf_header[62:64])[0]
        
        print(f"Program headers: {phnum} entries at offset {phoff}")
        print(f"Section headers: {shnum} entries at offset {shoff}")
        
        # 读取section header string table
        f.seek(shoff + shstrndx * 64)
        shstr_header = f.read(64)
        shstr_offset = struct.unpack('<Q', shstr_header[24:32])[0]
        shstr_size = struct.unpack('<Q', shstr_header[32:40])[0]
        
        f.seek(shstr_offset)
        shstrtab = f.read(shstr_size)
        
        # 找到.limine_requests section
        limine_section_addr = None
        for i in range(shnum):
            f.seek(shoff + i * 64)
            sh = f.read(64)
            sh_name_idx = struct.unpack('<I', sh[0:4])[0]
            sh_type = struct.unpack('<I', sh[4:8])[0]
            sh_addr = struct.unpack('<Q', sh[16:24])[0]
            
            # 获取section名字
            name_end = shstrtab.find(b'\x00', sh_name_idx)
            name = shstrtab[sh_name_idx:name_end].decode('ascii')
            
            if name == '.limine_requests':
                limine_section_addr = sh_addr
                print(f"Found .limine_requests section at {sh_addr:#x}")
                break
        
        if limine_section_addr is None:
            print("No .limine_requests section found!")
            return True  # 不是错误，可能没有这个段
        
        # 找到包含这个地址的PT_LOAD段
        for i in range(phnum):
            f.seek(phoff + i * 56)
            ph = f.read(56)
            
            p_type = struct.unpack('<I', ph[0:4])[0]
            p_flags = struct.unpack('<I', ph[4:8])[0]
            p_vaddr = struct.unpack('<Q', ph[16:24])[0]
            p_memsz = struct.unpack('<Q', ph[40:48])[0]
            
            if p_type == 1:  # PT_LOAD
                if p_vaddr <= limine_section_addr < p_vaddr + p_memsz:
                    print(f"Segment {i}: contains .limine_requests, flags={p_flags:#x}")
                    if p_flags & 0x2 == 0:  # 没有W标志
                        print(f"Patching segment {i} to add Write flag...")
                        new_flags = p_flags | 0x2
                        f.seek(phoff + i * 56 + 4)
                        f.write(struct.pack('<I', new_flags))
                        print(f"Changed flags from {p_flags:#x} to {new_flags:#x}")
                    else:
                        print("Already writable!")
                    return True
        
        print("Could not find PT_LOAD containing .limine_requests!")
        return False

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: fix_limine_perms.py <kernel.bin>")
        sys.exit(1)
    
    if fix_elf_permissions(sys.argv[1]):
        sys.exit(0)
    else:
        sys.exit(1)
