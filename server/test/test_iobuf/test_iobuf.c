/*
 *  test_iobuf.c
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 2/12/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <octet.h>
#include <wrappers.h>
#include <log.h>
#include <srv.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_iobuf_new(void);
static unsigned int test_iobuf_add(void);
static unsigned int test_iobuf_addn(void);
static unsigned int test_iobuf_add_octet(void);
static unsigned int test_iobuf_shift(void);
static unsigned int test_iobuf_flush(void);
static unsigned int test_iobuf_content(void);

test_conf_t tc[] = {
    {"test_iobuf_new", test_iobuf_new},
    {"test_iobuf_add", test_iobuf_add},
    {"test_iobuf_addn", test_iobuf_addn},
    {"test_iobuf_add_octet", test_iobuf_add_octet},
    {"test_iobuf_shift", test_iobuf_shift},
    {"test_iobuf_flush", test_iobuf_flush},
    {"test_iobuf_content", test_iobuf_content},
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

char *LONGSTR = "But when Newton couldn't mathematically balance the \"6 planets\" in stable orbits, he decides it must be God. He quits trying to understand and explore it after that, as do a great many intelligent people in history. The disturbing thing is that it means that that once \"God\" is accepted as an answer, they are either unable or unwilling to explore that subject further. God is the antithesis of discovery.";

static unsigned int test_iobuf_new(void)
{
    spocp_iobuf_t   *io;
    
    io = iobuf_new(1024);
    TEST_ASSERT(io != NULL);
    TEST_ASSERT(io->buf != NULL);
    TEST_ASSERT_EQUALS(io->left, 1023);
    TEST_ASSERT_EQUALS(io->bsize, 1024);
    TEST_ASSERT(*io->buf == '\0');
    
    return 0;
}

static unsigned int test_iobuf_add(void)
{
    spocp_iobuf_t   *io;
    spocp_result_t  rc;
    
    io = iobuf_new(128);
    rc = iobuf_add(io, "Chrome");
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 127-6);
    TEST_ASSERT( strcmp(io->buf,"Chrome") == 0 );

    rc = iobuf_add(io, "DeLux");
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 127-11);
    TEST_ASSERT( strcmp(io->buf,"ChromeDeLux") == 0 );

    rc = iobuf_add(io, "Super");
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 127-16);
    TEST_ASSERT( strcmp(io->buf,"ChromeDeLuxSuper") == 0 );

    rc = iobuf_add(io, "");
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 127-16);
    TEST_ASSERT( strcmp(io->buf,"ChromeDeLuxSuper") == 0 );

    rc = iobuf_add(io, LONGSTR);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT( io->bsize > 128);
    TEST_ASSERT( strcmp(io->buf+16,LONGSTR) == 0 );
    
    return 0;
}

static unsigned int test_iobuf_addn(void)
{
    spocp_iobuf_t   *io;
    spocp_result_t  rc;
    
    io = iobuf_new(1024);
    rc = iobuf_addn(io, "Chrome",5);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 1023-5);
    TEST_ASSERT( strcmp(io->buf,"Chrom") == 0 );
    
    rc = iobuf_addn(io, "DeLux",4);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 1023-9);
    TEST_ASSERT( strcmp(io->buf,"ChromDeLu") == 0 );
    
    rc = iobuf_addn(io, "Super",3);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 1023-12);
    TEST_ASSERT( strcmp(io->buf,"ChromDeLuSup") == 0 );
    
    rc = iobuf_addn(io, "",2);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 1023-12);
    TEST_ASSERT( strcmp(io->buf,"ChromDeLuSup") == 0 );
    
    return 0;
}

static unsigned int test_iobuf_add_octet(void)
{
    spocp_iobuf_t   *io;
    spocp_result_t  rc;
    octet_t         oct;
    
    io = iobuf_new(1024);
    oct_assign(&oct, "Chrome");
    rc = iobuf_add_octet(io, &oct);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 1023-8);
    TEST_ASSERT( strcmp(io->buf,"6:Chrome") == 0 );
    
    oct_assign(&oct, "DeLux");
    rc = iobuf_add_octet(io, &oct);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 1023-15);
    TEST_ASSERT( strcmp(io->buf,"6:Chrome5:DeLux") == 0 );
    
    oct_assign(&oct, "Super");
    rc = iobuf_add_octet(io, &oct);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 1023-22);
    TEST_ASSERT( strcmp(io->buf,"6:Chrome5:DeLux5:Super") == 0 );
    
    oct_assign(&oct, "");
    rc = iobuf_add_octet(io, &oct);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(io->left, 1023-22);
    TEST_ASSERT( strcmp(io->buf,"6:Chrome5:DeLux5:Super") == 0 );

    oct_assign(&oct, LONGSTR);
    rc = iobuf_add_octet(io, &oct);
    TEST_ASSERT_EQUALS(rc, SPOCP_SUCCESS);
    TEST_ASSERT( io->bsize > 128);
    TEST_ASSERT( strncmp(io->buf+22,"405:But when",12) == 0 );
    
    return 0;
}

static unsigned int test_iobuf_shift(void)
{
    spocp_iobuf_t   *io;
    spocp_result_t  rc;
    octet_t         oct;
    int             left;
    
    io = iobuf_new(1024);
    oct_assign(&oct, LONGSTR);
    rc = iobuf_add_octet(io, &oct);
    
    /* simulate that you have read 64 chars */
    left = io->left;
    
    io->r += 64;
    iobuf_shift(io);
    
    TEST_ASSERT( io->r == io->buf);
    TEST_ASSERT_EQUALS( io->left, left+64);

    io->r += 64;
    iobuf_shift(io);
    
    TEST_ASSERT( io->r == io->buf);
    /* Ooops encountered a space, hence the +1 */
    TEST_ASSERT_EQUALS( io->left, left+128+1);
    
    return 0;
}

static unsigned int test_iobuf_flush(void)
{
    spocp_iobuf_t   *io;
    spocp_result_t  rc;
    octet_t         oct;
    
    io = iobuf_new(1024);
    oct_assign(&oct, LONGSTR);
    rc = iobuf_add_octet(io, &oct);

    iobuf_flush(io);
    
    TEST_ASSERT( io->r == io->buf);
    TEST_ASSERT( io->w == io->buf);
    TEST_ASSERT_EQUALS( io->left, 1024-1);
    
    return 0;
}

static unsigned int test_iobuf_content(void)
{
    spocp_iobuf_t   *io;
    spocp_result_t  rc;
    octet_t         oct;
    
    io = iobuf_new(1024);
    oct_assign(&oct, LONGSTR);
    rc = iobuf_add_octet(io, &oct);
    
    TEST_ASSERT(iobuf_content(io));
    
    iobuf_flush(io);
    TEST_ASSERT(iobuf_content(io) == 0);

    TEST_ASSERT(iobuf_content(NULL) == -1);

    return 0;
}

