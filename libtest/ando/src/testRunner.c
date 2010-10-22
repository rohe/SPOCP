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
#include <stdio.h>
#include "testRunner.h"

static unsigned int counter;

unsigned int testRunner(TEST_FUNCTION testFunction) {
    int err;
    counter = 0;
    err = (testFunction)();
    if (err) {
        printf("NG (now %d ok...)\n",counter);
        return 0xffffffffu;
    }
    printf("\n");
    return 0;
}

int run_test(test_conf_t *tc)
{
    printf("%s: ", tc->name);
    if (testRunner(tc->func) != 0) {
        printf("Failed\n");
        return -1;
    }
    
    return 0;
}

void doTestCountUp(void) {
    printf(".");
    counter++;
}
