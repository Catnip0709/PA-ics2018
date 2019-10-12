#include "fs.h"

extern size_t fs_filesz(int fd);

//真正的文件读写
extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);

extern void dispinfo_read(void *buf, off_t offset, size_t len);
extern void fb_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);

typedef struct {
  char *name;         //文件名
  size_t size;        //文件大小
  off_t disk_offset;  //文件在ramdisk中的偏移
  off_t open_offset;  //文件被打开后的读写指针(新添加的成员)
} Finfo;



//文件描述符
enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
  file_table[FD_FB].size = _screen.height * _screen.width * 4;
}

int fs_open(const char *pathname, int flags, int mode) {
  Log("pathname %s\n", pathname);
  for (int i = 0; i < NR_FILES; i++) {
    //printf("file name: %s\n", file_table[i].name);
    if (strcmp(file_table[i].name, pathname) == 0) 
      //找到了目标文件
      return i; //返回文件描述符
  }

  //没找到目标文件
  assert(0);
  return -1;
}


ssize_t fs_read(int fd, void *buf, size_t len) {
  ssize_t fs_size = fs_filesz(fd);
  //Log("in the read, fd = %d, file size = %d, len = %d, file open_offset = %d\n", fd, fs_size, len, file_table[fd].open_offset);
  
  switch(fd) {
    case FD_STDOUT:
    case FD_FB:
      break;
    case FD_EVENTS:
      len = events_read((void *)buf, len);
      break;
    case FD_DISPINFO:
      if (file_table[fd].open_offset >= file_table[fd].size)
        return 0;
      if (file_table[fd].open_offset + len > file_table[fd].size)
        len = file_table[fd].size - file_table[fd].open_offset;
      dispinfo_read(buf, file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;  
      break;
    default:
      if(file_table[fd].open_offset >= fs_size || len == 0)
        return 0;
      if(file_table[fd].open_offset + len > fs_size)
        len = fs_size - file_table[fd].open_offset;
      ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      break;
  }
  return len;
}

int fs_close(int fd) {
  return 0;
}

size_t fs_filesz(int fd) {
  return file_table[fd].size;
}

ssize_t fs_write(int fd, const void *buf, size_t len) {
  ssize_t fs_size = fs_filesz(fd);
  switch(fd) {
    case FD_STDOUT:
    case FD_STDERR:
      for(int i = 0; i < len; i++) {
          _putc(((char*)buf)[i]);
      }
      break;
    case FD_FB:
      fb_write(buf, file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      break;
    default:
      if(file_table[fd].open_offset >= fs_size)
        return 0; 
      if(file_table[fd].open_offset + len > fs_size)
        len = fs_size - file_table[fd].open_offset;
      ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      break;
  }
  return len;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  off_t result = -1;

  switch(whence) {
    case SEEK_SET:
      if (offset >= 0 && offset <= file_table[fd].size) {
        file_table[fd].open_offset = offset;
        result = file_table[fd].open_offset = offset;
      }
      break;
    case SEEK_CUR:
      if ((offset + file_table[fd].open_offset >= 0) && 
          (offset + file_table[fd].open_offset <= file_table[fd].size)) {
        file_table[fd].open_offset += offset;
        result = file_table[fd].open_offset;
      }
      break;
    case SEEK_END:
      file_table[fd].open_offset = file_table[fd].size + offset;
      result = file_table[fd].open_offset;
      break;
  }
  
  return result;
}
