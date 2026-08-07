#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <susfs_defs.h>
#include <susfs_utils.h>
#include "file_spoof.h"

#define CMD_SUSFS_ADD_FILE_SPOOF 0x555d0

enum UID_SCHEME {
	UID_NON_APP_PROC = 0,
	UID_ROOT_PROC_EXCEPT_SU_PROC,
	UID_NON_SU_PROC,
	UID_UMOUNTED_APP_PROC,
	UID_UMOUNTED_PROC,
};

struct st_susfs_file_spoof {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	char                    spoofed_content[SUSFS_MAX_SPOOF_FILE_SIZE];
	unsigned int            spoofed_size;
	int                     uid_scheme;
	int                     err;
};

void file_spoof_print_help(void){
	log("    add_file_spoof </target/path> </fake/content/file> <uid_scheme>\n");
	log("      |--> Replace read/readv output of target path with the bytes from fake content file\n");
	log("      |--> Designed for small procfs/sysfs identity nodes such as /proc/cpuinfo, /proc/device-tree/model, /sys/devices/soc0/*\n");
	log("      |--> <uid_scheme>\n");
	log("             |--> 0: Effective for non-app processes (uid < 10000)\n");
	log("             |--> 1: Effective for non-su processes of which uid is 0 (All root process but not with su domain)\n");
	log("             |--> 2: Effective for non-su processes (Use it carefully!)\n");
	log("             |--> 3: Effective for processes that are marked umounted with uid >= 10000 (recommended for app/game spoofing)\n");
	log("             |--> 4: Effective for processes that are marked umounted (include most of the init spawned process, use it carefully!)\n");
	log("      * Important Notes *\n");
	log("      - target_path must exist, but no bind mount/open redirect is created\n");
	log("      - fake/content/file is read once by this tool; re-run the command after changing its content\n");
	log("      - Max fake content size: %d bytes\n", SUSFS_MAX_SPOOF_FILE_SIZE);
	log("\n");
}

static void print_help(void){
	print_help_banner();
	file_spoof_print_help();
}

static int read_fake_content(const char *path, char *buf, unsigned int *out_size) {
	FILE *fp;
	size_t read_size;

	fp = fopen(path, "rb");
	if (!fp) {
		log("[-] failed to open fake content file: %s, errno: %d\n", path, errno);
		return errno;
	}

	read_size = fread(buf, 1, SUSFS_MAX_SPOOF_FILE_SIZE, fp);
	if (ferror(fp)) {
		int err = errno ? errno : EIO;
		fclose(fp);
		log("[-] failed to read fake content file: %s, errno: %d\n", path, err);
		return err;
	}

	if (fgetc(fp) != EOF) {
		fclose(fp);
		log("[-] fake content file is larger than %d bytes: %s\n", SUSFS_MAX_SPOOF_FILE_SIZE, path);
		return E2BIG;
	}

	fclose(fp);
	*out_size = (unsigned int)read_size;
	return 0;
}

int add_file_spoof(int argc, char *argv[]) {
	struct st_susfs_file_spoof *info;
	char target_pathname[PATH_MAX];
	char fake_content_pathname[PATH_MAX];
	char *endptr;
	long uid_scheme;
	int err;

	if (argc != 5) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty'\n");
		return -EINVAL;
	}

	if (*argv[3] == '\0') {
		log("[-] argv[3] is empty'\n");
		return -EINVAL;
	}

	if (!realpath(argv[2], target_pathname)) {
		log("[-] failed to get realpath from target path: %s\n", argv[2]);
		return errno;
	}

	if (!realpath(argv[3], fake_content_pathname)) {
		log("[-] failed to get realpath from fake content file: %s\n", argv[3]);
		return errno;
	}

	uid_scheme = strtol(argv[4], &endptr, 10);
	if (*endptr != '\0') {
		print_help();
		return -EINVAL;
	}

	if (uid_scheme < UID_NON_APP_PROC || uid_scheme > UID_UMOUNTED_PROC) {
		print_help();
		return -EINVAL;
	}

	info = malloc(sizeof(struct st_susfs_file_spoof));
	if (!info) {
		perror("malloc");
		return -ENOMEM;
	}
	memset(info, 0, sizeof(struct st_susfs_file_spoof));

	err = read_fake_content(fake_content_pathname, info->spoofed_content, &info->spoofed_size);
	if (err) {
		free(info);
		return err;
	}

	info->uid_scheme = uid_scheme;
	strncpy(info->target_pathname, target_pathname, SUSFS_MAX_LEN_PATHNAME-1);
	info->err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_ADD_FILE_SPOOF, info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info->err, CMD_SUSFS_ADD_FILE_SPOOF);
	err = info->err;
	free(info);
	return err;
}
