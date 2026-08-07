#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <errno.h>
#include <limits.h>
#include <susfs_defs.h>
#include <susfs_utils.h>
#include "sus_map.h"

#define CMD_SUSFS_ADD_SUS_MAP 0x60020
#define CMD_SUSFS_ADD_SUS_MAP_UID 0x60021

enum UID_SCHEME {
	UID_NON_APP_PROC = 0,
	UID_ROOT_PROC_EXCEPT_SU_PROC,
	UID_NON_SU_PROC,
	UID_UMOUNTED_APP_PROC,
	UID_UMOUNTED_PROC,
};

struct st_susfs_sus_map {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int                     err;
};

struct st_susfs_sus_map_uid {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int                     uid_scheme;
	int                     err;
};

void sus_map_print_help(void){
	log("    add_sus_map </path/to/actual/library>\n");
	log("      |--> added real file path which gets mmapped will be hidden from /proc/self/[maps|smaps|smaps_rollup|map_files|mem|pagemap]\n");
	log("      |--> e.g., add_sus_map '/data/adb/modules/my_module/zygisk/arm64-v8a.so'\n");
	log("      |--> legacy behavior: only effective for processes marked umounted with uid >= 10000\n");
	log("\n");
	log("    add_sus_map_uid </path/to/actual/library> <uid_scheme>\n");
	log("      |--> uid-aware sus_map. Use this when the target app is not marked umounted on the current kernel/manager setup\n");
	log("      |--> <uid_scheme>\n");
	log("             |--> 0: Effective for non-app processes (uid < 10000)\n");
	log("             |--> 1: Effective for non-su processes of which uid is 0 (All root process but not with su domain)\n");
	log("             |--> 2: Effective for non-su processes (use carefully; useful for ordinary app/game observation-surface hiding)\n");
	log("             |--> 3: Effective for processes that are marked umounted with uid >= 10000\n");
	log("             |--> 4: Effective for processes that are marked umounted\n");
	log("      |--> e.g., add_sus_map_uid '/data/adb/modules/my_module/zygisk/arm64-v8a.so' 2\n");
	log("      * Important Notes *\n");
	log("      - It does NOT support hiding for anon memory.\n");
	log("      - It does NOT hide any inline hooks or plt hooks cause by the injected library itself\n");
	log("      - It may not be able to evade detections by apps that implement a good injection detection\n");
	log("      - add_sus_map_uid only changes procfs observation surfaces, not actual execution state\n");
	log("\n");
}

static void print_help(void){
	print_help_banner();
	sus_map_print_help();
}

int add_sus_map(int argc, char *argv[]) {
	struct st_susfs_sus_map info = {0};

	if (argc != 3) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty'\n");
		return -EINVAL;
	}

	strncpy(info.target_pathname, argv[2], SUSFS_MAX_LEN_PATHNAME-1);
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_ADD_SUS_MAP, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_ADD_SUS_MAP);
	return info.err;
}

int add_sus_map_uid(int argc, char *argv[]) {
	struct st_susfs_sus_map_uid info = {0};
	char resolved_pathname[PATH_MAX];
	char *endptr;
	long uid_scheme;

	if (argc != 4) {
		print_help();
		return -EINVAL;
	}

	if (*argv[2] == '\0') {
		log("[-] argv[2] is empty'\n");
		return -EINVAL;
	}

	if (!realpath(argv[2], resolved_pathname)) {
		log("[-] failed to get realpath from target path: %s\n", argv[2]);
		return errno;
	}

	uid_scheme = strtol(argv[3], &endptr, 10);
	if (*endptr != '\0') {
		print_help();
		return -EINVAL;
	}

	if (uid_scheme < UID_NON_APP_PROC || uid_scheme > UID_UMOUNTED_PROC) {
		print_help();
		return -EINVAL;
	}

	strncpy(info.target_pathname, resolved_pathname, SUSFS_MAX_LEN_PATHNAME-1);
	info.uid_scheme = uid_scheme;
	info.err = ERR_CMD_NOT_SUPPORTED;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, CMD_SUSFS_ADD_SUS_MAP_UID, &info);
	PRT_MSG_IF_CMD_NOT_SUPPORTED(info.err, CMD_SUSFS_ADD_SUS_MAP_UID);
	return info.err;
}
