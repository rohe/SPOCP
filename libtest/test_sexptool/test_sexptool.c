/* test program for lib/sexp.c */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <result.h>
#include <sexptool.h>
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */

static char *get_uid(void *arg);
static char *get_host(void *arg);
static char *get_time(void *arg);

static unsigned int test_parse_format(void);
static unsigned int test_sexp_constr(void);

test_conf_t tc[] = {
    {"test_parse_format", test_parse_format},
    {"test_sexp_constr", test_sexp_constr},
    {"",NULL}
};

/* ================================================================== */

const sexparg_t transf[] = {
	{"uid", get_uid, 'a', FALSE},
	{"host", get_host, 'a', FALSE},
	{"time", get_time, 'a', FALSE},    
} ;

int ntransf = (sizeof(transf) / sizeof(transf[0]));

static char    *
get_uid(void *arg)
{
    char  uid[32];
    sprintf(uid,"%d",(int) getuid());
	return strdup(uid);
}

static char    *
get_host(void *arg)
{
    int     namelen = 256;
    char    name[namelen];
    
    gethostname(name, namelen);
    
	return strdup(name);
}

static char    *
get_time(void *arg)
{
	char            date[24];
	time_t          t;
	struct tm       tm;
    
	time(&t);
	localtime_r(&t, &tm);
	strftime(date, 24, "%Y-%m-%dT%H:%M:%S", &tm);
    
	return strdup(date);
}

char *srvquery = "(6:server(4:host%{host})(3:uid%{uid})(4:time%{time}))";

/* Expected result splitting the string above */
sexparg_t sares[] = {
    {"(6:server(4:host", 0, 'l', 1 },
    {NULL, get_host, 'a', 0 },
    {")(3:uid", 0, 'l', 1},
    {NULL, get_uid, 'a', 0 },
    {")(4:time", 0, 'l', 1},
    {NULL, get_time, 'a', 0 },
    {"))", 0, 'l', 0},
    {NULL, 0, 0, 0}
} ;


/* ================================================================== */

int main( int argc, char **argv )
{
    test_conf_t *ptc;
    
    printf("------------------------------------------\n");
    for( ptc = tc; ptc->func ; ptc++) {
        run_test(ptc);
    }
    printf("------------------------------------------\n");
    return 0;
}

static unsigned int test_parse_format(void)
{
    sexparg_t	**srvq;
    int         n;

    srvq = parse_format(srvquery, transf, ntransf);
    for (n = 0; srvq[n]; n++) ;
    TEST_ASSERT_EQUALS(n, 7);
    for (n = 0; n < 7; n++) {
        TEST_ASSERT( srvq[n]->af == sares[n].af);
        TEST_ASSERT_EQUALS( srvq[n]->type, sares[n].type);
        TEST_ASSERT_EQUALS( srvq[n]->dynamic ,sares[n].dynamic);
        if (srvq[n]->type == 'l') {
            TEST_ASSERT( strcmp(srvq[n]->var,sares[n].var) == 0);
        }
        else
            TEST_ASSERT( srvq[n]->var == sares[n].var);
    }
    return 0;
}

static unsigned int test_sexp_constr(void)
{
    sexparg_t	**srvq;
    char        *res;
    
    srvq = parse_format(srvquery, transf, ntransf);
    res = sexp_constr(NULL, srvq);
    TEST_ASSERT( res != NULL );
    TEST_ASSERT( strlen(res) > 0 );
    /* printf("res: %s\n", res); */
    return 0;
}