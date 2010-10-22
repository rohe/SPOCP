/*
 * "CUnit for Mr.Ando" is CppUnit-x based C langage testing framework
 * for Mr.Ando. It provide the C source code for unit testing. 
 * Copyright (C) 2004-2005 Toshikazu Ando.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _TEST_RUNNER_H_
#define _TEST_RUNNER_H_
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Assert single */
#define TEST_ASSERT(_a) \
    {\
        int assertErrorA = (_a);\
        if (! assertErrorA) {\
            fprintf(stdout,"%s:%d: error: TestCaseError(0x%08x)\n",\
                __FILE__,__LINE__,(assertErrorA));\
            return 0xffffffffu;\
        } else {\
            doTestCountUp();\
        }\
    }

#define TEST_ASSERT_FALSE(_a) \
    {\
        int assertErrorA = (_a);\
        if (assertErrorA) {\
            fprintf(stdout,"%s:%d: error: TestCaseError(0x%08x)\n",\
                __FILE__,__LINE__,(assertErrorA));\
            return 0xffffffffu;\
        } else {\
            doTestCountUp();\
        }\
    }
    
/** Assert equals */
#define TEST_ASSERT_EQUALS(_a,_b) \
    {\
        int assertErrorA = (_a);\
        int assertErrorB = (_b);\
        if ((assertErrorA) != (assertErrorB)) {\
            fprintf(stdout,"%s:%d: error: TestCaseError<0x%08x><0x%08x>\n",\
                __FILE__,__LINE__,(assertErrorA),(assertErrorB));\
            return 0xffffffffu;\
        } else {\
            doTestCountUp();\
        }\
    }

/** Assert equals */
#define TEST_ASSERT_NOT_EQUALS(_a,_b) \
    {\
        int assertErrorA = (_a);\
        int assertErrorB = (_b);\
        if ((assertErrorA) == (assertErrorB)) {\
            fprintf(stdout,"%s:%d: error: TestCaseError<0x%08x><0x%08x>\n",\
                __FILE__,__LINE__,(assertErrorA),(assertErrorB));\
            return 0xffffffffu;\
        } else {\
            doTestCountUp();\
        }\
    }

/** File pointer define */
typedef unsigned int (*TEST_FUNCTION)(void);

typedef struct test_conf {
    char            *name;
    TEST_FUNCTION   func;
} test_conf_t;


/** Test runner. */
unsigned int testRunner(TEST_FUNCTION testFunction);

int run_test(test_conf_t *tc);

/** Test counter */
void doTestCountUp(void);
    
#ifdef __cplusplus
}
#endif
#endif 
