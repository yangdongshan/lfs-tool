#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "cli.h"

#define CLI_CMD_MAX (50)
#define CMD_SIZE (128)
#define CMD_ARG_MAX (32)
#define CMD_ARG_LEN (32)

#define CLI_REMINDER "<cli>"

static cli_cmd_t g_cmds[CLI_CMD_MAX];

static pthread_mutex_t mutex;

static void register_cmds(void);

int cli_register_cmds(cli_cmd_t *cmds, int cnt)
{
    int done_cnt = 0;
    cli_cmd_t *cmd;

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < CLI_CMD_MAX; i++) {
       if (g_cmds[i].name == NULL) {
	   		printf("register cmd %s at g_cmds[%d]\r\n", cmds[done_cnt].name, i);
            g_cmds[i].name = cmds[done_cnt].name;
            g_cmds[i].desc = cmds[done_cnt].desc;
            g_cmds[i].func = cmds[done_cnt].func;
            if (++done_cnt >= cnt) break;
       }
    }
    pthread_mutex_unlock(&mutex);

    return done_cnt;
}

int cli_unregister_cmds(cli_cmd_t *cmds, int cnt)
{
    int done_cnt = 0;
    cli_cmd_t *cmd;

    cmd = g_cmds;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < CLI_CMD_MAX; i++) {
		for (int j = 0; j < cnt; j++) {
			if (cmd->name == cmds[j].name) {
				memset(cmd, 0, sizeof(*cmd));
			}
		}
    }
    pthread_mutex_unlock(&mutex);

    return cnt;
}


int cli_main(void *arg)
{

    pthread_mutex_init(&mutex, NULL);

    memset(g_cmds, 0, sizeof(g_cmds));

    register_cmds();

	printf("%s",CLI_REMINDER);

    char c;
    char cmd_buf[CMD_SIZE+1];
    int cmd_buf_index = 0;
    memset(cmd_buf, 0, sizeof(cmd_buf));
    while (scanf("%c", &c)) {
        if (c != '\n') {
            cmd_buf[cmd_buf_index++] = c;
            if (cmd_buf_index < CMD_SIZE) {
                continue;
            } else {
                printf("cmd_buf overflow\r\n");
            }
        }

        int argc = 0;
        char *argv[CLI_CMD_MAX];
        memset(argv, 0, sizeof(argv));
        char *p = cmd_buf;
        while (p && *p != '0') {
            if (*p == ' ') {
                p++;
                continue;
            } else if (*p == '0') {
                break;
            }

            argv[argc++] = p;
			p = strchr(p, ' ');
			if (p) {
				*p = 0;
				p++;
			}
        }


		if (argc == 0)
			continue;

		printf("cmd: argc %d\r\n",argc);
		for (int n = 0; n < argc; n++) {
			printf("cmd: argv[%d]: %s\r\n", n, argv[n]);
		}

        pthread_mutex_lock(&mutex);
        {
	        int i;
	        for (i = 0; i < CLI_CMD_MAX; i++) {
	            if (g_cmds[i].func == NULL)
	                continue;

	            if (strncmp(cmd_buf, g_cmds[i].name, strlen(g_cmds[i].name)) == 0) {
					printf("found registered cmd %s\r\n", g_cmds[i].name);
	                g_cmds[i].func(argc, argv);
	                break;
	            }
	        }
	        if (i == CLI_CMD_MAX) {
				printf(" unknown cmd\r\n");
	        }
        }
        pthread_mutex_unlock(&mutex);

        cmd_buf_index = 0;
        memset(cmd_buf, 0, sizeof(cmd_buf));
        printf("%s",CLI_REMINDER);
    }

	return 0;
}

extern void cmd_cwd_register(void);
extern void cmd_ls_register(void);


void register_cmds(void)
{
	cmd_cwd_register();
	cmd_ls_register();
}

