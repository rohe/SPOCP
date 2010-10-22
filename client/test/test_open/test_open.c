#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <spocpcli.h>
#include <octet.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_open(void);
static unsigned int test_open_and_close(void);

test_conf_t tc[] = {
    {"test_open", test_open},
    {"test_open_and_close", test_open_and_close}, 
    {"",NULL}
};

char *LOGOUT = "8:6:LOGOUT";

char *com_path = "/usr/local/spocp/bin/spocd";
char *com_argv[] = {"spocd", "-f", "conf"};

void close_down_server()
{
    int     fd;
    char    pid[16], com[32];
    ssize_t nc;
    
    printf("== Closing down server\n");
    fd = open("spocd.pid",O_RDONLY);
    nc = read(fd, pid, 16);    
    if (nc > 0) {
        pid[nc] = '\0';
        sprintf(com,"kill %s",pid);
        system(com);
    }
    else {
        printf("== Problem reading spocd.pid (nc=%d)\n",(int) nc);
    }
    close(fd);
}

int main( int argc, char **argv )
{
    test_conf_t *ptc;
    int         i;
    
    i = fork();
    if (i == 0) { /* child process */
        /* start the spocp server */
        execve(com_path, com_argv, NULL);
        printf("Failed to execve: %d", errno);
        return 0;        
    }
    else {    
        sleep(2); /* wait for the server to start */

        printf("------------------------------------------\n");
        for( ptc = tc; ptc->func ; ptc++) {
            run_test(ptc);
        }
        printf("------------------------------------------\n");
        
        /* should kill the spocp server */
        close_down_server();
        
        return 0;
    }
}


static unsigned int test_open(void)
{
    SPOCP   *spocp = NULL;
    char    *server = "localhost:5678";
    int     timeout = 2;
    
    spocp = spocpc_open( 0, server, timeout ) ;
    TEST_ASSERT( spocp != NULL );
    
    spocpc_close(spocp);
    
    return 0;
}

static unsigned int test_open_and_close(void)
{
    SPOCP   *spocp = NULL;
    char    *server = "localhost:5678";
    int     timeout = 2;
    int     res;
    
    spocp = spocpc_open( 0, server, timeout ) ;
    TEST_ASSERT( spocp != NULL );
    
    res = spocpc_send_logout(spocp);
    printf("res: %d\n", res);
    TEST_ASSERT(res == SPOCPC_OK);
    
    spocpc_close(spocp);

    return 0;
}
