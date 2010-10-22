/*
 *  test_reply.c
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
static unsigned int test_reply_new(void);
static unsigned int test_reply_push(void);
static unsigned int test_reply_pop(void);
static unsigned int test_reply_add(void);

test_conf_t tc[] = {
    {"test_reply_new", test_reply_new},
    {"test_reply_push", test_reply_push},
    {"test_reply_pop", test_reply_pop},
    {"test_reply_add", test_reply_add},
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

static unsigned int test_reply_new(void)
{
    reply_t     *rp;
    work_info_t *wi = NULL;
    
    rp = reply_new(wi);
    TEST_ASSERT(rp != NULL);
    TEST_ASSERT(rp->wi == wi);
    
    wi = (work_info_t *)Calloc(1, sizeof(work_info_t));

    rp = reply_new(wi);
    TEST_ASSERT(rp != NULL);
    TEST_ASSERT(rp->wi == wi);
    
    return 0;
}

static unsigned int test_reply_push(void)
{
    reply_t     *rp, *head=NULL;
    work_info_t *wi = NULL;
    
    rp = reply_new(wi);
    head = reply_push(head, rp);
    TEST_ASSERT(head == rp);
    
    wi = (work_info_t *)Calloc(1, sizeof(work_info_t));    
    rp = reply_new(wi);
    head = reply_push(head, rp);
    TEST_ASSERT(head != rp);
    TEST_ASSERT(head->next == rp);
    
    return 0;
}

static unsigned int test_reply_pop(void)
{
    reply_t     *rp[2], *head=NULL, *rep;
    work_info_t *wi = NULL;
    
    rp[0] = reply_new(wi);
    head = reply_push(head, rp[0]);    
    wi = (work_info_t *)Calloc(1, sizeof(work_info_t));    
    rp[1] = reply_new(wi);
    head = reply_push(head, rp[1]);

    rep = reply_pop(&head);
    TEST_ASSERT(rep == rp[0])
    TEST_ASSERT(head == rp[1]);

    rep = reply_pop(&head);
    TEST_ASSERT(rep == rp[1])
    TEST_ASSERT(head == NULL);
    
    return 0;
}

static unsigned int test_reply_add(void)
{
    reply_t         *rp[2], *head=NULL, *rep;
    work_info_t     *wi[2];
    spocp_result_t  rc;
    octet_t         oct;    
    
    wi[0] = (work_info_t *)Calloc(1, sizeof(work_info_t));    
    wi[0]->buf = iobuf_new(1024);
    
    wi[1] = (work_info_t *)Calloc(1, sizeof(work_info_t));    
    wi[1]->buf = iobuf_new(1024);

    /* just to confuse :-) */
    rp[0] = reply_new(wi[1]);
    head = reply_push(head, rp[0]);   
    rp[1] = reply_new(wi[0]);
    head = reply_push(head, rp[1]);

    oct_assign(&oct, "Chrome");
    rc = iobuf_add_octet(wi[0]->buf, &oct);
    oct_assign(&oct, "DeLux");
    rc = iobuf_add_octet(wi[1]->buf, &oct);

    rep = reply_add(head, wi[0]);
    TEST_ASSERT(rep == rp[1]);
    TEST_ASSERT(strcmp(rep->buf->buf,"6:Chrome") == 0 );
    rep = reply_add(head, wi[1]);
    TEST_ASSERT(rep == rp[0]);
    TEST_ASSERT(strcmp(rep->buf->buf,"5:DeLux") == 0 );
    
    return 0;
}

