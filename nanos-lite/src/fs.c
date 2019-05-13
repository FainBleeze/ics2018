#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;  //ramdisk中的偏移
  off_t open_offset;  //目前文件操作的位置 即读写指针
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

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);
extern void dispinfo_read(void *buf, off_t offset, size_t len);
extern void fb_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);

void init_fs() {
  // TODO: initialize the size of /dev/fb
  file_table[FD_FB].size = _screen.height * _screen.width * 4;
}

size_t fs_filesz(int fd) {
	return file_table[fd].size;
}

int fs_open(const char *pathname, int flags, int mode) {
	//可读写所有文件，故忽略flags mode
	Log("Pathname: %s", pathname);
	int i;
	for (i = 0; i < NR_FILES; i++) {
		//printf("file name: %s\n", file_table[i].name);
		if (strcmp(file_table[i].name, pathname) == 0) {
			return i;
		}
	}
	assert(0);
	return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
	ssize_t fs_size = fs_filesz(fd);
	//Log("in the read, fd = %d, file size = %d, len = %d, file open_offset = %d\n", fd, fs_size, len, file_table[fd].open_offset);
	switch(fd) {
		case FD_STDOUT:
		case FD_FB:
			Log("in the fs_read fd_fb\n");
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
			//偏移量不可以超过文件边界
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

ssize_t fs_write(int fd, const void *buf, size_t len) {
	ssize_t fs_size = fs_filesz(fd);
	switch(fd) {
		case FD_STDOUT:
		case FD_STDERR:
			// call _putc()
			// 串口已被抽象成stdout stderr
			for(int i = 0; i < len; i++) {
				_putc(((char*)buf)[i]);
			}
			break;
		case FD_FB:
			// write to frame buffer 显存
			// device.c:fb_write buff中len字节输出到屏幕上offest处
			fb_write(buf, file_table[fd].open_offset, len);
			file_table[fd].open_offset += len;
			break;
		default:
			// write to ramdisk
			if(file_table[fd].open_offset >= fs_size)
				return 0;	
			if(file_table[fd].open_offset + len > fs_size)
				len = fs_size - file_table[fd].open_offset;
			// 对文件的真正读写
			ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
			file_table[fd].open_offset += len;
			//Log("offset = %d", file_table[fd].open_offset);
			break;
	}
	return len;// 参见man 返回值
}

off_t fs_lseek(int fd, off_t offset, int whence) {
	off_t result = -1;
	// fs.h
	switch(whence) {
		case SEEK_SET:
			if (offset >= 0 && offset <= file_table[fd].size) {
				file_table[fd].open_offset = offset;
				//result = file_table[fd].open_offset = offset;
				result = file_table[fd].open_offset;
				//Log("!");
			}
			//Log("!");
			break;
		case SEEK_CUR:
			if ((offset + file_table[fd].open_offset >= 0) && (offset + file_table[fd].open_offset <= file_table[fd].size)) {
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

int fs_close(int fd) {
	return 0;
}
