//
//    Copyright (C) 2017 LGPL
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Lesser General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
//    USA

/*
 *---------------------------------------------------------------------------
 *| VERSION	| AUTHOR		| DATE			| NOTE							|
 *---------------------------------------------------------------------------
 *| 01		| JiangBo              | 2017-08-29	       |								|
 *---------------------------------------------------------------------------
 */

#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fwk/cli/cli.h>
#include <fwk/task/task.h>

int efwk_cli_idle_timeout(struct cli_def *p_cli_def);
int efwk_cli_thread_create(void);

struct cli_def * efwk_cli_init(void)
{
    struct cli_def *cli = cli_init();

    cli_set_banner(cli, "Connect to server"); // greeting info when user connects
    cli_set_hostname(cli, "Cli"); // cli hostname
    cli_telnet_protocol(cli, 1);

    // disable some default command
    cli_unregister_command(cli,"logout");
    cli_unregister_command(cli,"quit");
    //cli_unregister_command(cli,"history");
    cli_unregister_command(cli,"enable");
    cli_unregister_command(cli,"disable");
    cli_unregister_command(cli,"configure");

    // authentication with add username / password combinations via cli_allow_user() function
    // if not set, means no authentication required.
    cli_allow_user(cli,"cli","cli");

    // set idle timeout
    cli_set_idle_timeout_callback(cli, 180, efwk_cli_idle_timeout);

    return cli;
}

typedef struct {
    struct cli_def* cli;
    int socket;
}cli_arg_wrap_t;

void *efwk_cli_loop_wrapper(void *arg)
{
    cli_arg_wrap_t* pArg = (cli_arg_wrap_t*)arg;
    struct cli_def *cli = pArg->cli;
    int socket = pArg->socket;
    cli_loop(cli, socket);
    close(socket);
    cli_done(cli);
    fwk_terminateTask();
    return NULL;
}

int efwk_cli_idle_timeout(__attribute__((unused)) struct cli_def *p_cli_def)
{
    cli_print(p_cli_def, "Idle timeout");
    return CLI_QUIT;
}

struct cli_def * efwk_cli_session_init(void)
{
    int i = 0, rc = 0;
    struct cli_def * myCli;
    myCli = efwk_cli_init();
    while (gCliUserCmd[i].name) {
        rc = efwk_cli_register_command(myCli, gCliUserCmd[i].name, gCliUserCmd[i].func, gCliUserCmd[i].tips);
        if (rc == CLI_ERROR) {
            printf("%s fail to register %i cmd, name=%s\n", __func__, i, gCliUserCmd[i].name);
            break;
        }
        ++i;
    }
    return myCli;
}

void *efwk_cli_thread_func(__attribute__((unused)) void *arg)
{
    int s, x;
    struct sockaddr_in addr;
    int on = 1;
    (void)(arg);

    signal(SIGCHLD, SIG_IGN);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return NULL;
    }
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(EFWK_CLI_PORT);
    if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return NULL;
    }

    if (listen(s, 50) < 0) {
        perror("listen");
        return NULL;
    }

    printf("CLI listening on port %d\n", EFWK_CLI_PORT);
    while ((x = accept(s, NULL, 0))) {
        socklen_t len = sizeof(addr);
        if (getpeername(x, (struct sockaddr *) &addr, &len) >= 0)
            printf("CLI accepted connection %i from %s\n", x, inet_ntoa(addr.sin_addr));

        cli_arg_wrap_t args;
        args.cli = efwk_cli_session_init();
        args.socket = x;
        fwk_taskID_t tid;
        fwk_taskRes_t resource = {1024*1024, 0, 0, 0};
        int rc = fwk_createDisposableTask("cliClt", &tid, efwk_cli_loop_wrapper, &args, 1, resource, 1);
        printf("%s to create CLI Client, socket %i, tid: %p\n",  rc ? "Fail" : "Success", x, tid);
    }

    return NULL;
}


int efwk_cli_start(size_t stackSize)
{
    fwk_taskID_t tid;
    fwk_taskRes_t resource = {stackSize, 0, 0, 0}; //default stackSize 1024*1024
    int rc = fwk_createDisposableTask("cliSvr", &tid, efwk_cli_thread_func, NULL, 1, resource, 1);
    printf("%s to create CLI Server, tid: %p\n", rc ? "Fail" : "Success", tid);
    return rc;
}

int efwk_cli_register_command(struct cli_def * cli, const char *command,
    int (*callback)(struct cli_def *cli, const char *, char **, int),
    const char *help)
{
    struct cli_command *c = NULL;

    c = cli_register_command(cli, NULL, command, callback, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, help);
    if(c == NULL)
        return CLI_ERROR;
    else
        return CLI_OK;
}
