/*
 *  check_oct.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 12/3/09.
 *  Copyright 2009 Umeå Universitet. All rights reserved.
 *
 */

/* test program for lib/oct.c */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <octet.h>
#include <proto.h>
#include <spocp.h>
#include <wrappers.h>
#include <log.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_chunk_new(void);
static unsigned int test_charbuf_new(void);
static unsigned int test_get_chunk(void);
static unsigned int test_get_sexp(void);
static unsigned int test_chunk2sexp(void);

test_conf_t tc[] = {
    {"test_chunk_new", test_chunk_new},
    {"test_charbuf_new", test_charbuf_new},
    {"test_get_chunk", test_get_chunk},
    {"test_get_sexp", test_get_sexp},
    {"test_chunk2sexp", test_chunk2sexp},
    {"",NULL}
};

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

static unsigned int test_chunk_new(void)
{
    octet_t         octa;
    spocp_chunk_t   *scp;

    oct_assign(&octa, "Willow");
    scp = chunk_new(&octa);
    TEST_ASSERT(octcmp(scp->val, &octa) == 0);
    TEST_ASSERT(scp->next == 0);
    TEST_ASSERT(scp->prev == 0);
                
    return 0;
}

/*char            *b64 = "bGVhc3VyZS4=";*/

static unsigned int test_charbuf_new(void)
{
    spocp_charbuf_t *scp;
    FILE            *fp;
    
    fp = fopen("file.1", "r");
    scp = charbuf_new(fp, 64);

    TEST_ASSERT(scp->fp == fp);
    TEST_ASSERT_EQUALS(scp->size, 64);
    TEST_ASSERT(scp->str != 0);
    TEST_ASSERT(scp->start == scp->str);
    
    fclose(fp);
    
    return 0;
}

char *chnk[] = {
    "(","test","(","host","(","*","suffix",
    ".catalogix.se",")",")","(","uid",")",")",NULL
};

static unsigned int test_get_chunk(void)
{
    spocp_charbuf_t *charbuf;
    FILE            *fp;
    spocp_chunk_t   *chunk;
    int             i;

    fp = fopen("file.1", "r");
    charbuf = charbuf_new(fp, 64);

    for (i = 0; chnk[i]; i++) {
        chunk = get_chunk( charbuf );    
        TEST_ASSERT( oct2strcmp(chunk->val, chnk[i]) == 0 );
    }

    fclose(fp);
                    
    return 0;
}

char *ch3[] = { "(", "ref", "su-employee", ")", NULL };

static unsigned int test_get_sexp(void)
{
    spocp_charbuf_t *charbuf;
    FILE            *fp;
    spocp_chunk_t   *chunk = NULL, *cp;
    int             i;
    
    fp = fopen("file.1", "r");
    charbuf = charbuf_new(fp, 64);
    
    chunk = get_chunk(charbuf);
    cp = get_sexp( charbuf, chunk);
    cp = chunk;
    for (i = 0; chnk[i]; i++, cp = cp->next) {
        TEST_ASSERT( oct2strcmp(cp->val, chnk[i]) == 0 );
    }
    
    fclose(fp);
    
    fp = fopen("file.2", "r");
    charbuf = charbuf_new(fp, 64);
    /* first is a s-expression */
    chunk = get_chunk(charbuf);
    cp = get_sexp( charbuf, chunk);
    /* second is '=>' */
    chunk = get_chunk(charbuf);
    TEST_ASSERT( oct2strcmp(chunk->val, "=>") == 0 ) ;
    chunk = get_chunk(charbuf);
    cp = get_sexp( charbuf, chunk);
    for (i = 0, cp = chunk; ch3[i]; i++, cp = cp->next) {
        TEST_ASSERT( oct2strcmp(cp->val, ch3[i]) == 0 );
    }
    
    return 0;
}

char *file4str[] = { 
    "(6:base648:leasure.)", 
    "(3:hex4:<NO>)",
    "(5:quote6:bedoba4:bupp)",
    "/file",
    "(7:combine8:leasure.4:<NO>6:bed ba)",
    NULL,
};

static unsigned int test_chunk2sexp(void)
{
    octet_t         *op;
    spocp_charbuf_t *charbuf;
    FILE            *fp;
    spocp_chunk_t   *chunk = NULL, *cp;
    int             n = 0;
    
    fp = fopen("file.1", "r");
    charbuf = charbuf_new(fp, 64);    
    chunk = get_chunk(charbuf);
    cp = get_sexp( charbuf, chunk);
    
    op = chunk2sexp( chunk );
    /*oct_print(0, "-->", op); */
    TEST_ASSERT(oct2strcmp(op, 
            "(4:test(4:host(1:*6:suffix13:.catalogix.se))(3:uid))") == 0);
    fclose(fp);

    fp = fopen("file.2", "r");
    charbuf = charbuf_new(fp, 64);    
    chunk = get_chunk(charbuf);
    cp = get_sexp( charbuf, chunk);
    
    op = chunk2sexp( chunk );
    /*oct_print(0, "-->", op); */
    TEST_ASSERT(oct2strcmp(op, 
            "(4:http(8:protocol)(5:vhost22:public.it.secure.su.se)(8:resource10:SUintranet)(8:authtype7:SUKATID)(6:client(8:hostname)(2:ip))(4:user(9:principal(3:uid)(5:realm5:SU.SE))))") == 0);
    oct_free(op);
    chunk = get_chunk(charbuf);
    chunk = get_chunk(charbuf);
    cp = get_sexp( charbuf, chunk);
    op = chunk2sexp( chunk );
    TEST_ASSERT(oct2strcmp(op, "(3:ref11:su-employee)") == 0 );
    fclose(fp);

    fp = fopen("file.3", "r");
    charbuf = charbuf_new(fp, 64);    
    chunk = get_chunk(charbuf);
    cp = get_sexp( charbuf, chunk);
    
    op = chunk2sexp( chunk );
    TEST_ASSERT(oct2strcmp(op, 
        "(9:suservice(3:uid)(7:service(1:*3:set3:ppp25:urn:x-su:service:type:ppp))(2:op8:activate)(4:time))") == 0);
    oct_free(op);
    chunk = get_chunk(charbuf);
    TEST_ASSERT( oct2strcmp(chunk->val, "=>") == 0 ) ;
    chunk = get_chunk(charbuf);
    cp = get_sexp( charbuf, chunk);
    op = chunk2sexp( chunk );
    TEST_ASSERT(oct2strcmp(op, "(3:ref10:su-student)") == 0 );
    fclose(fp);

    fp = fopen("file.4", "r");
    charbuf = charbuf_new(fp, 64);    
    chunk = get_chunk(charbuf);
    while (chunk) {
        if ( oct2strcmp(chunk->val, "(") == 0 ) {
            cp = get_sexp( charbuf, chunk);
            op = chunk2sexp( chunk );
            /* oct_print(0, "-->", op); */
            TEST_ASSERT(oct2strcmp(op, file4str[n]) == 0);
        }
        else {
            TEST_ASSERT(oct2strcmp(chunk->val, file4str[n]) == 0);
        }
        chunk = get_chunk(charbuf);
        n++;
    }
    fclose(fp);
    
    return 0;
}

