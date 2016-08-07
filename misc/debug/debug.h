#ifndef _DEBUG_H_
#define _DEBUG_H_
/*
 * Debug name
 */
char *debug_name[37] = {
	NULL,
	"EPERM: Operation not permitted",
	"ENOENT: No such file or directory",
	"ESRCH: No such process",
	"EINTR: Interrupt system call",
	"EIO: I/O error",
	"ENXIO: No such device or address",
	"E2BIG: Argument list too long",
	"ENOEXEC: Exec format error",
	"EBADF: Bad file number",
	"ECHILD: No child processes",
	"EAGAIN: Try again",
	"ENOMEM: Out of memory",
	"EACCES: Permission denied",
	"EFAULT: Bad address",
	"ENOTBLK: Block device required",
	"EBUSY: Device or resource busy",
	"EEXIST: File exists",
	"EXDEV: Cross-device link",
	"ENODEV: No such device",
	"ENOTDIR: Not a directory",
	"EISDIR: Is a directory",
	"EINVAL: Invalid argument",
	"ENFILE: File table overflow",
	"EMFILE: Too many open files",
	"ENOTTY: Not a tyoewriter",
	"ETXTBSY: Text file busy",
	"EFBIG: File too large",
	"ENOSPC: No space left on device",
	"ESPIPE: lllegal seek",
	"EROFS: Read-only file system",
	"EMLINK: Too many links",
	"EPIPE: Broken pipe",
	"EDOM: Math argument out of domain of func",
	"ERANGE: Math result not representable",
	NULL
};
/*
 * Debug number
 */
static void buddy_debug_number(unsigned long num)
{
	printk(KERN_INFO "DEBUG: %s\n",debug_name[num]);
}

#endif
