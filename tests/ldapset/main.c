#include "ldapset.h"
#include "dmalloc.h"

#define AVLUS 1

char *test = "{\\0$cn & it-sysadm}/uniqueMember & {\\0$jabberID & ${0}}" ;

int main(int argc,char **argv)
{
    int i;
    spocp_result_t rc = SPOCP_SUCCESS ;
    scnode_t *scp;
    char text[512];
    octet_t oct;
    
    for( i = 0; i < 10000; i++) {
        oct_assign(&oct,test);
        scp = scnode_get(&oct,&rc) ;
        printf("%p\n", scp);
        memset(text,0,512);
        scnode_print(scp,text);
        printf("%s\n",text);
        scnode_free(scp);
    }
}

