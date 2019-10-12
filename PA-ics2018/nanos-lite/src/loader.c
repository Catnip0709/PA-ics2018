#include "common.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern size_t get_ramdisk_size();

extern ssize_t fs_read(int fd, void *buf, size_t len);
extern size_t fs_filesz(int fd);
extern int fs_open(const char *pathname, int flags, int mode);
extern int fs_close(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
 //TODO();
	//功能：将ramdisk中从0开始的所有内容放置到0x4000000,并返回
	/*size_t len = get_ramdisk_size();
	ramdisk_read(DEFAULT_ENTRY,0,len);
    return (uintptr_t)DEFAULT_ENTRY;*/
    int fd = fs_open(filename, 0, 0);  			//打开文件
	printf("fd = %d\n", fd);
	fs_read(fd, DEFAULT_ENTRY, fs_filesz(fd));  //读文件
	fs_close(fd);   							//关闭文件
	return (uintptr_t)DEFAULT_ENTRY;
}

