#include <stdio.h>
#include "cli.h"

char str_cwd[128] = "/";

int func_cwd(int argc, char **argv)
{
	printf("%s\r\n", str_cwd);
}

static cli_cmd_t cmd_cwd = {
	.name = "cwd",
	.desc = "print current work directory",
	.func = func_cwd,
};

#if 0
char* cwd_push(char *rel_path, int len)
{
	if (rel_path && (strlen(str_cwd) + len < 128)) {
		if (rel_path[0] == '/')
			strcat(str_cwd, rel_path);
		else {
			strcat(str_cwd, '/');
			strcat(str_cwd, rel_path);
		}
	}

	return str_cwd;
}

char *cwd_pop(char *rel_path, int len)
{
	if (rel_path && len < strlen(str_cwd)) {
		strstr(str_cwd, rel_path);
	}
}

#endif

void cmd_cwd_register(void)
{
	cli_register_cmds(&cmd_cwd, 1);
}



