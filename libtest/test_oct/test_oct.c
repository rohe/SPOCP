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
#include <wrappers.h>

#include <testRunner.h>

/*
 * test function 
 */
static unsigned int test_optimized_allocation(void);
static unsigned int test_oct_assign(void);
static unsigned int test_oct_find_balancing(void);
static unsigned int test_oct_resize(void);
static unsigned int test_oct_new(void);
static unsigned int test_octln(void);
static unsigned int test_octcln(void);
static unsigned int test_octdup(void);
static unsigned int test_octcpy(void);
static unsigned int test_octclr(void);
static unsigned int test_octcat(void);
static unsigned int test_oct2strcmp(void);
static unsigned int test_oct2strncmp(void);
static unsigned int test_octcmp(void);
static unsigned int test_octrcmp(void);
static unsigned int test_octncmp(void);
static unsigned int test_octmove(void);
static unsigned int test_octstr(void);
static unsigned int test_octchr(void);
static unsigned int test_octpbrk(void);
static unsigned int test_oct2strdup(void);
static unsigned int test_oct2strcpy(void);
static unsigned int test_oct_de_escape(void);
static unsigned int test_octtoi(void);
static unsigned int test_str2oct(void);
static unsigned int test_octset(void);
static unsigned int test_str_esc(void);

test_conf_t tc[] = {
    {"test_optimized_allocation", test_optimized_allocation},
    {"test_oct_assign", test_oct_assign},
    {"test_oct_find_balancing", test_oct_find_balancing},
    {"test_oct_new", test_oct_new},
    {"test_oct_resize", test_oct_resize},
    {"test_octln", test_octln},
    {"test_octcln", test_octcln},
    {"test_octdup", test_octdup},
    {"test_octcpy", test_octcpy},
    {"test_octclr", test_octclr},
    {"test_octcat", test_octcat},
    {"test_oct2strcmp", test_oct2strcmp},
    {"test_oct2strncmp", test_oct2strncmp},
    {"test_octcmp", test_octcmp},
    {"test_octrcmp", test_octrcmp},
    {"test_octncmp", test_octncmp},
    {"test_octmove", test_octmove},
    {"test_octstr", test_octstr},
    {"test_octchr", test_octchr},
    {"test_octpbrk", test_octpbrk},
    {"test_oct2strdup", test_oct2strdup},
    {"test_oct2strcpy", test_oct2strcpy},
    {"test_oct_de_escape", test_oct_de_escape},
    {"test_octtoi", test_octtoi},
    {"test_str2oct", test_str2oct},
    {"test_octset", test_octset},
    {"test_str_esc", test_str_esc},
    {"",NULL}
};

void _print_oct( octet_t *oct) 
{
    char *s;
    
    printf("len: %d\n", (int) oct->len);
    printf("size: %d\n", (int) oct->size);
    printf("base: %p\n", oct->base);
    printf("val: %p\n", oct->val);
    s = oct2strdup(oct, 0);
    printf("val: '%s'\n", s);
    Free(s);
}

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

static unsigned int test_optimized_allocation(void)
{
    octet_t octa, octb;
    char    *string = "Foo", *copy;
    char    *longstring = "Programming Language";
    int     size ;
    
    octa.size = 0;
    octa.base = octa.val = string;
    octa.len = strlen(string);
    
    copy = optimized_allocation(&octa, &size);
    TEST_ASSERT_EQUALS(size, 3);
    TEST_ASSERT( copy != 0 );

    octb.size = strlen(longstring);
    octb.base = octb.val = strdup(string);
    octb.len = octb.size;
    octb.val += 10;
    octb.len -= 10;
    /* remove some from the end */
    octb.len -= 2;
    
    copy = optimized_allocation(&octb, &size);
    /*
    printf("offset: %ld\n", octb.val - octb.base);
    printf("len: %d\n", (int) octb.len);
    printf("size: %d\n", size);
     */
    TEST_ASSERT_EQUALS(size, octb.size - 2);
    
    return 0;
}

static unsigned int test_oct_assign(void)
{
    octet_t o;
    char    *string = "Foo";

    oct_assign(&o, string);
    TEST_ASSERT_EQUALS(o.size, 0);
    TEST_ASSERT_EQUALS(o.len, 3);
    TEST_ASSERT(memcmp(o.val, string, 3) == 0);
    
    return 0;
}

static unsigned int test_oct_find_balancing(void)
{
    octet_t o;
    char    *sym[2] = {"(4:fopp)","(()())"};
    char    *asym[2] = {"${foo}${bar}}","))[]}"};
    int     i;
    
    /* Symmetric */
    for (i=0; i<2; i++) {
        oct_assign(&o, sym[i]);
        TEST_ASSERT_EQUALS(oct_find_balancing(&o, '(', ')'), -1);
    }

    /* Asymmetric */
    for (i=0; i<2; i++) {
        oct_assign(&o, asym[i]);
        TEST_ASSERT_NOT_EQUALS(oct_find_balancing(&o, '{', '}'), -1);
    }
    
    return 0;
}

static unsigned int test_oct_new(void)
{
    octet_t *oct;
    char    *test = "Teststring";
    int     size;
    
    size = 16;
    oct = oct_new(size, test);
    TEST_ASSERT_EQUALS(oct->len, strlen(test));
    TEST_ASSERT_EQUALS(oct->size, size);
    TEST_ASSERT(strncmp(oct->val,test,strlen(test)) == 0);

    size = 0;
    oct = oct_new(size, test);
    TEST_ASSERT_EQUALS(oct->len, strlen(test));
    TEST_ASSERT_EQUALS(oct->size, strlen(test));
    TEST_ASSERT(strncmp(oct->val,test,strlen(test)) == 0);

    /* size less then string length */
    size = 8;
    oct = oct_new(size, test);
    TEST_ASSERT_EQUALS(oct->len, size);
    TEST_ASSERT_EQUALS(oct->size, size);
    TEST_ASSERT(strncmp(oct->val,test,size) == 0);    
    
    return 0;
}

static unsigned int test_oct_resize(void)
{
    octet_t o;
    octet_t *po;
    char    *text = "Test";

    oct_assign(&o, text);
    TEST_ASSERT_EQUALS(oct_resize(&o, 64), SPOCP_DENIED);
    
    text = "Dynamic text";
    po = oct_new(0, text);
    TEST_ASSERT_EQUALS(oct_resize(po, 64), SPOCP_SUCCESS);
    TEST_ASSERT(po->size >= 64);
    TEST_ASSERT_EQUALS(po->len, strlen(text));
    
    return 0;
}

static unsigned int test_octln(void)
{
    octet_t a, *po ;
    char    *text;
    
    text = "Dynamic text";
    po = oct_new(0, text);

    TEST_ASSERT_EQUALS(octln(&a, po), 1);
    
    TEST_ASSERT_EQUALS(a.len, po->len);
    /* printf("%d, %d\n", (int) a.size, (int) po->size);*/
    TEST_ASSERT_NOT_EQUALS(a.size, po->size);
    TEST_ASSERT_EQUALS(a.size, 0);

    TEST_ASSERT_EQUALS(octln(NULL,NULL), -1);
    TEST_ASSERT_EQUALS(octln(&a, NULL), -1);
    TEST_ASSERT_EQUALS(octln(NULL, po), -1);

    return 0;
}

static unsigned int test_octcln(void)
{
    octet_t *poa, *pob ;

    poa = octcln(0);
    TEST_ASSERT(poa == 0);
    
    poa = oct_new(0, "Dynamic text");
    pob = octcln(poa);
    TEST_ASSERT_EQUALS(poa->len, pob->len);
    TEST_ASSERT(memcmp(poa->val,pob->val,poa->len) == 0);
    TEST_ASSERT_EQUALS(pob->size, 0);
    TEST_ASSERT(poa != pob);
    
    return 0;
}

static unsigned int test_octdup(void)
{
    octet_t *poa,*pob ;
    
    TEST_ASSERT(octdup(NULL) == NULL);
    
    poa = oct_new(0, "Some text");
    pob = octdup(poa);
    TEST_ASSERT_EQUALS(poa->len, pob->len);
    TEST_ASSERT(memcmp(poa->val,pob->val,poa->len) == 0);
    /* printf("sizes: %d, %d\n", (int) poa->size, (int) pob->size); */
    TEST_ASSERT(pob->size >= poa->size);
    TEST_ASSERT(poa != pob);
    
    return 0;
}

static unsigned int test_octcpy(void)
{
    octet_t *poa, *pob;
    octet_t oct;
    
    memset(&oct, 0, sizeof(octet_t));
    
    poa = oct_new(0, "Some text");
    
    TEST_ASSERT(octcpy(poa, NULL) == SPOCP_DENIED);
    TEST_ASSERT(octcpy(&oct, poa) == SPOCP_SUCCESS);
    TEST_ASSERT(memcmp(poa->val,oct.val,poa->len) == 0);    
    
    pob = oct_new(0, "Another text");
    
    TEST_ASSERT_EQUALS(octcpy(pob, poa), SPOCP_SUCCESS);

    TEST_ASSERT_EQUALS(poa->len, pob->len);
    TEST_ASSERT(memcmp(poa->val,pob->val,poa->len) == 0);
    TEST_ASSERT_NOT_EQUALS(pob->size, (size_t) 0);
    
    return 0;
}

static unsigned int test_octclr(void)
{
    octet_t *poa ;
    char    *text = "Some text";
    
    poa = oct_new(0, text);

    TEST_ASSERT(memcmp(poa->val, text, strlen(text)) == 0);
    TEST_ASSERT_EQUALS(poa->len, strlen(text));
    
    octclr(poa);

    TEST_ASSERT_EQUALS(poa->size, 0);
    TEST_ASSERT_EQUALS(poa->len, 0);    
    TEST_ASSERT(poa->val == 0);    
    
    return 0;
}

static unsigned int test_octcat(void)
{
    octet_t poa ;
    char    *text = "Some text";
    size_t  n = strlen(text);
    
    poa.size = poa.len = 0;
    
    TEST_ASSERT_EQUALS(octcat(&poa, text, n), SPOCP_SUCCESS);
    TEST_ASSERT(memcmp(poa.val, text, n) == 0);
    TEST_ASSERT_EQUALS(poa.len, n);

    TEST_ASSERT_EQUALS(octcat(&poa, text, n), SPOCP_SUCCESS);
    TEST_ASSERT_EQUALS(poa.len, 2*n);
    TEST_ASSERT(memcmp(poa.val, text, n) == 0);
    TEST_ASSERT(memcmp(&poa.val[n], text, n) == 0);

    return 0;
}

static unsigned int test_oct2strcmp(void)
{
    octet_t *poa, ob ;
    char    *text = "Some text";
    char    *s = "";
    char    *extra = " extras";
    char    *num = "555";
    
    TEST_ASSERT_EQUALS(oct2strcmp(0, s), 0);

    ob.size = ob.len = 0;
    TEST_ASSERT_EQUALS(oct2strcmp(&ob, s), 0);
    
    poa = oct_new(0, text);    
    TEST_ASSERT_EQUALS(oct2strcmp(poa, text), 0);
    
    /* longer */
    octcat(poa, extra, strlen(extra));
    TEST_ASSERT_NOT_EQUALS(oct2strcmp(poa, text), 0);

    /* shorter */
    oct_free(poa);
    poa = oct_new(0, "Some");
    TEST_ASSERT_NOT_EQUALS(oct2strcmp(poa, text), 0);

    /* just different */
    oct_free(poa);
    poa = oct_new(0, "Any string");
    TEST_ASSERT_NOT_EQUALS(oct2strcmp(poa, text), 0);
    
    oct_free(poa);
    poa = oct_new(0, "444");
    TEST_ASSERT_EQUALS(oct2strcmp(poa, num), -1);
    return 0;
}

static unsigned int test_oct2strncmp(void)
{
    octet_t *poa, ob ;
    char    *text = "Some text";
    char    *s = "";
    char    *extra = " extras";
    
    TEST_ASSERT_EQUALS(oct2strncmp(0, s, 1), 0);
    
    ob.size = ob.len = 0;
    TEST_ASSERT_EQUALS(oct2strncmp(&ob, s, 1), 0);
    
    poa = oct_new(0, text);    
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, 4), 0);
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, 6), 0);
    /* will not compare more than the length of the shortest */
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, 16), 0);
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, 128), 0);

    octcat(poa, extra, strlen(extra));
    /* Doesn't matter what size I give, it doesn't compare passed the
     length of the bytearrays */
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, strlen(text)), 0);
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, poa->len), 0);
    
    /* Doesn't matter which is the shorter */
    oct_free(poa);
    poa = oct_new(0, "Some");
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, 4), 0);
    oct_free(poa);
    poa = oct_new(0, "Some text which is extended");
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, 8), 0);
    TEST_ASSERT_EQUALS(oct2strncmp(poa, text, 16), 0);
    
    /* just different */
    oct_free(poa);
    poa = oct_new(0, "Any string");
    TEST_ASSERT_NOT_EQUALS(oct2strncmp(poa, text, 4), 0);
    
    return 0;
}

static unsigned int test_octcmp(void)
{
    octet_t *poa, *pob ;
    char    *text = "Some text";

    poa = 0;
    pob = 0;
    TEST_ASSERT_EQUALS(octcmp(poa, pob), 0);

    poa = oct_new(0, text);
    TEST_ASSERT_NOT_EQUALS(octcmp(poa, pob), 0);

    pob = oct_new(0, text);
    TEST_ASSERT_EQUALS(octcmp(poa, pob), 0);

    oct_free(poa);
    poa = oct_new(0, "Any string");
    TEST_ASSERT_NOT_EQUALS(octcmp(poa, pob), 1);

    oct_free(poa);
    poa = oct_new(0, "");
    TEST_ASSERT_NOT_EQUALS(octcmp(poa, pob), -1);
    
    return 0;
}

static unsigned int test_octrcmp(void)
{
    octet_t *poa, *pob ;
    char    *text = "Some text";
    
    poa = 0;
    pob = 0;
    TEST_ASSERT_EQUALS(octrcmp(poa, pob), 0);
    
    poa = oct_new(0, text);
    TEST_ASSERT_NOT_EQUALS(octrcmp(poa, pob), 0);
    
    pob = oct_new(0, text);
    TEST_ASSERT_EQUALS(octrcmp(poa, pob), 0);
    
    oct_free(poa);
    poa = oct_new(0, "Any string");
    TEST_ASSERT_NOT_EQUALS(octrcmp(poa, pob), 1);
    
    oct_free(poa);
    poa = oct_new(0, "");
    TEST_ASSERT_NOT_EQUALS(octrcmp(poa, pob), -1);
    
    return 0;
}

static unsigned int test_octncmp(void)
{
    octet_t *poa, *pob ;
    char    *text = "Some text";
    char    *extra = " extras";
    
    poa = 0;
    pob = 0;
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 1), 0);
    
    poa = oct_new(0, text);    
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 1), 1);
    
    pob = oct_new(0, text);    
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 4), 0);
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 6), 0);
    /* will not compare more than the length of the shortest */
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 16), 0);
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 128), 0);
    
    octcat(poa, extra, strlen(extra));
    /* Doesn't matter what size I give, it doesn't compare passed the
     length of the bytearrays */
    TEST_ASSERT_EQUALS(octncmp(poa, pob, pob->len), 0);
    TEST_ASSERT_EQUALS(octncmp(poa, pob, poa->len), 0);
    
    /* Doesn't matter which is the shorter */
    oct_free(poa);
    poa = oct_new(0, "Some");
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 4), 0);
    oct_free(poa);
    poa = oct_new(0, "Some text which is extended");
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 8), 0);
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 16), 0);
    
    /* just different */
    oct_free(poa);
    poa = oct_new(0, "Any string");
    TEST_ASSERT(octncmp(poa, pob, 4) < 0);
    TEST_ASSERT(octncmp(pob, poa, 4) > 0);
    
    oct_free(poa);
    poa = 0;
    TEST_ASSERT_EQUALS(octncmp(poa, pob, 1), -1);

    return 0;
}

static unsigned int test_octmove(void)
{
    octet_t *poa, *pob ;
    char    *text = "Some text";
    char    *extra = " extras";
    size_t  size, len;

    poa = oct_new(0, text);
    pob = oct_new(0, extra);
    
    size = pob->size;
    len = pob->len;
    
    octmove(poa, pob);
    TEST_ASSERT_EQUALS(poa->len, len);
    TEST_ASSERT_EQUALS(poa->size, size);
    TEST_ASSERT( memcmp(poa->val, extra, len) == 0);
    TEST_ASSERT_EQUALS(pob->len, 0);
    TEST_ASSERT_EQUALS(pob->size, 0);
    TEST_ASSERT(pob->val == 0);

    return 0;
}

static unsigned int test_octstr(void)
{
    octet_t oct;
    
    oct.val = "The C Programming Language";
    oct.len = strlen(oct.val);
    oct.size = 0;
    
    /* printf("where: %d", octstr(&oct,"Prog")); */
    TEST_ASSERT_EQUALS(octstr(&oct,"Prog"),6);
    TEST_ASSERT_EQUALS(octstr(&oct,"Lang"),18);
    TEST_ASSERT_EQUALS(octstr(&oct,"Brian"),-1);
    TEST_ASSERT_EQUALS(octstr(&oct,"chie"),-1);
    TEST_ASSERT_EQUALS(octstr(&oct,"The C Programming Language by BK&DR"),-1);
    
    return 0;
}

static unsigned int test_octchr(void)
{
    octet_t oct;
    
    oct.val = "The C Programming Language";
    oct.len = strlen(oct.val);
    oct.size = 0;
    
    TEST_ASSERT_EQUALS(octchr(&oct,'P'), 6);
    TEST_ASSERT_EQUALS(octchr(&oct,'L'), 18);
    TEST_ASSERT_EQUALS(octchr(&oct,'B'),-1);
    TEST_ASSERT_EQUALS(octchr(&oct,'x'),-1);
    /* printf("where: %d", octchr(&oct,'g'));*/
    TEST_ASSERT_EQUALS(octchr(&oct,'g'), 9);
    
    return 0;
}

static unsigned int test_octpbrk(void)
{
    octet_t oct, *acc;
    
    oct.val = "The C Programming Language";
    oct.len = strlen(oct.val);
    oct.size = 0;
    
    acc = oct_new(0, "abc");
    TEST_ASSERT_EQUALS(octpbrk(&oct,acc), 11);
    oct_free(acc);
    acc = oct_new(0, "def");
    TEST_ASSERT_EQUALS(octpbrk(&oct,acc), 2);
    oct_free(acc);
    acc = oct_new(0, "fed");
    TEST_ASSERT_EQUALS(octpbrk(&oct,acc), 2);
    oct_free(acc);
    acc = oct_new(0, "gam");
    TEST_ASSERT_EQUALS(octpbrk(&oct,acc), 9);
    
    return 0;
}

static unsigned int test_oct2strdup(void)
{
    octet_t *acc;

    acc = oct_new(0, "Apelsiner");
    TEST_ASSERT(strcmp(oct2strdup(acc, 0), "Apelsiner") == 0);
    oct_free(acc);
    acc = oct_new(0, "Päron");
    TEST_ASSERT(strcmp(oct2strdup(acc, '%'), "P%c3%a4ron") == 0);
    /* printf(">>: %s\n",oct2strdup(acc, '\\')); */
    TEST_ASSERT(strcmp(oct2strdup(acc, '\\'), "P\\c3\\a4ron") == 0);
    
    return 0;
}

static unsigned int test_oct2strcpy(void)
{
    octet_t *acc;
    char    str[64];
    int     n;
    
    memset(str, 0, 64*sizeof(char));

    acc = oct_new(0, "Apelsiner");
    n = oct2strcpy(acc, str, 63, 0);
    TEST_ASSERT_EQUALS(n, 9);
    TEST_ASSERT(strcmp(str, "Apelsiner") == 0);

    /* too small */
    n = oct2strcpy(acc, str, 4, 0);
    /* printf(">>: '%s' [%d]\n",str,n); */
    TEST_ASSERT_EQUALS(n, -1);

    oct_free(acc);

    acc = oct_new(0, "Päron");
    n = oct2strcpy(acc, str, 63, '%');
    /* printf(">>: '%s' [%d]\n",str,n); */
    TEST_ASSERT(strcmp(str, "P%c3%a4ron") == 0);

    /* When escpade len == 10 */
    n = oct2strcpy(acc, str, 12, '%');
    TEST_ASSERT_EQUALS(n, 10);
    /* doesn't fit when escaped */
    n = oct2strcpy(acc, str, 9, '%');
    TEST_ASSERT_EQUALS(n, -1);

    oct2strcpy(acc, str, 63, '\\');
    TEST_ASSERT(strcmp(str, "P\\c3\\a4ron") == 0);
    
    return 0;
}

static unsigned int test_oct_de_escape(void)
{
    octet_t *acc, *esc;
    char    str[64];
    int     n;
    
    memset(str, 0, 64*sizeof(char));
    
    acc = oct_new(0, "Päron");
    n = oct2strcpy(acc, str, 63, '%');
    esc = oct_new(0, str);
    n = oct_de_escape(esc, '%');
    /* printf(">>: '%s' [%d]\n",esc->val,n); */ 
    TEST_ASSERT(memcmp(esc->val, "Päron", esc->len) == 0);

    oct_free(esc);
    esc = oct_new(0, str);
    /* wrong escape char, nothing happens */
    n = oct_de_escape(esc, '\\');
    /* printf(">>: '%s' [%d]\n",esc->val,n); */
    TEST_ASSERT(memcmp(esc->val, "P%c3%a4ron", esc->len) == 0);
    
    return 0;
}

static unsigned int test_octtoi(void)
{
    octet_t *oct;
    int     n;
    
    oct = oct_new(0, "2009");
    n = octtoi(oct);
    TEST_ASSERT_EQUALS(n, 2009);
    
    /*printf("INT_MAX: %d\n", __INT_MAX__);*/
    oct_free(oct);
    /* Exceeds INT_MAX */
    oct = oct_new(0, "2147483652");
    /*printf("strcmp -> %d", oct2strcmp(oct, "2147483647"));*/
    n = octtoi(oct);
    /*printf(">>: %s/%d (%d)\n", oct->val,n, oct->len);*/
    TEST_ASSERT_EQUALS(n, -1);
    
    oct_free(oct);
    oct_new(0, "-1234");
    TEST_ASSERT_EQUALS(octtoi(oct), -1);

    oct_free(oct);
    oct_new(0, "1234x");
    TEST_ASSERT_EQUALS(octtoi(oct), -1);
    
    return 0;
}

static unsigned int test_str2oct(void)
{
    octet_t *oct;
    char    *str = "Brian"; 
    
    oct = str2oct(str, 0);
    
    TEST_ASSERT_EQUALS(oct->size, 0);
    TEST_ASSERT_EQUALS(oct->len, 5);
    TEST_ASSERT( memcmp( oct->val, str, strlen(str)) == 0);

    oct = str2oct(NULL, 0);
    
    TEST_ASSERT_EQUALS(oct->size, 0);
    TEST_ASSERT_EQUALS(oct->len, 0);
    TEST_ASSERT( oct->val == 0);    
    
    return 0;
}

static unsigned int test_octset(void)
{
    octet_t *oct;
    char    *str = "Brian"; 
    
    oct = str2oct(str, 0);
    
    TEST_ASSERT_EQUALS(oct->size, 0);
    TEST_ASSERT_EQUALS(oct->len, 5);
    TEST_ASSERT( memcmp( oct->val, str, strlen(str)) == 0);

    return 0;
}

static unsigned int test_str_esc(void)
{
    char    *a, *b;

    a = "Päron";
    b = str_esc(a, 0);
    /*printf(">> %s\n", b );*/
    TEST_ASSERT(strcmp(b, "P\\c3\\a4ron") == 0);

    b = str_esc(a, 3);
    /* printf(">> %s\n", b ); */
    TEST_ASSERT(strcmp(b, "P\\c3\\a4") == 0);
    
    return 0;
}
