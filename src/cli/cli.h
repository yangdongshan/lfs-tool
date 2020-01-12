#ifndef CLI_H
#define CLH_H

typedef int (*cli_func_t)(int argc, char **argv);

typedef struct cli_cmd {
    const char *name;
    const char *desc;
    cli_func_t func;
} cli_cmd_t;

int cli_register_cmds(cli_cmd_t *cmds, int cnt);

int cli_unregister_cmds(cli_cmd_t *cmds, int cnt);


#endif // CLI_H

