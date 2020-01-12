#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "cli.h"
#include "vfs_lfs.h"

extern char str_cwd[];

static void ls_help(void)
{
	printf("\n\r\tUsage: ls [-lah] [path]\r\n");
}


static void print_file(char *name, int flags, struct stat *s)
{
	printf("%-12s %8d Bytes\r\n", name, s->st_size);
}

static int func_ls(int argc, char **argv)
{
	printf("enter %s\r\n", __func__);
	printf("%s\r\n", str_cwd);

	int opt = 0;
	int opt_cnt = 0;
	int list_verbose = 0;
	int list_all = 0;
	int human = 0;

	printf("func_ls: argc %d\r\n", argc);
	for (int i = 0; i < argc; i++) {
		printf("argv[%d]: %s\r\n", i, argv[i]);
	}

#if 0
    while ((opt = getopt(argc, argv, "lah?")) != -1) {
        switch (opt) {
            case 'l':
                list_verbose = 1;
				opt_cnt++;
                break;
            case 'a':
                list_all = 1;
				opt_cnt++;
				break;
			case 'h':
				human = 1;
				opt_cnt++;
				break;
			case '?':
				ls_help();
				return -1;
        	}
	}
#endif

	struct stat s;
	int ret;
	char *path = str_cwd;
	if (ret = vfs_stat(NULL, path, &s)) {
		printf("stat %s error, errno %d\r\n", path, ret);
	}

	if (!(S_ISDIR(s.st_mode))) {
		print_file(path, 0, &s);
	} else {
		void *dir;
		struct vfs_dirent *dirent = NULL;
		dir = vfs_opendir(NULL, path);
		if (!dir) {
			printf("opendir %s failed\r\n", path);
			return -1;
		}

    	while ((dirent = vfs_readdir(NULL, dir)) != NULL) {
			if (dirent->type == VFS_TYPE_END) {
				break;
			}
			char temp_path[128];
			int len = strlen(path);
			memset(temp_path, 0, sizeof(temp_path));
			strncpy(temp_path, path, len);
			strcat(temp_path, dirent->name);
			if (ret = vfs_stat(NULL, temp_path, &s)) {
				printf("stat %s error, errno %d\r\n", temp_path, ret);
			} else {
				print_file(dirent->name, 0, &s);
			}
		}
	}

	return 0;
}

static cli_cmd_t cmd_ls = {
	.name = "ls",
	.desc = "ls [-lah] [path]",
	.func = func_ls,
};

void cmd_ls_register(void)
{
	cli_register_cmds(&cmd_ls, 1);
}




