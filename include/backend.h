#ifndef __BACKEND_H
#define __BACKEND_H

#include <config.h>

/*
 * be_dbm.c 
 */

#ifdef HAVE_LIBGDBM
spocp_result_t  gdbm_test(octet_t * arg, becpool_t * b, octet_t * blob);
#endif

/*
 * be_flatfile.c 
 */

spocp_result_t  flatfile_test(octet_t * arg, becpool_t * b, octet_t * blob);

/*
 * be_lastlogin.c 
 */

spocp_result_t  lastlogin_test(octet_t * arg, becpool_t * b, octet_t * blob);

/*
 * be_time.c 
 */

spocp_result_t  time_test(octet_t * arg, becpool_t * b, octet_t * blob);

/*
 * be_ldapset.c 
 */

#ifdef HAVE_LIBLDAP
spocp_result_t  ldapset_test(octet_t * arg, becpool_t * b, octet_t * blob);
#endif

/*
 * ipnum.c 
 */

spocp_result_t  ipnum_test(octet_t * arg, becpool_t * b, octet_t * blob);

/*
 * mailmatch.c 
 */

spocp_result_t  addrmatch_test(octet_t * arg, becpool_t * b, octet_t * blob);

/*
 * rbl.c 
 */

spocp_result_t  rbl_test(octet_t * arg, becpool_t * b, octet_t * blob);

/*
 * strmatch.c 
 */

spocp_result_t  strmatch_test(octet_t * arg, becpool_t * b, octet_t * blob);
spocp_result_t  strcasematch_test(octet_t * arg, becpool_t * b,
				  octet_t * blob);

/*
 * cert.c 
 */

#ifdef HAVE_SSL

spocp_result_t  cert_test(octet_t * arg, becpool_t * b, octet_t * blob);
spocp_result_t  cert_init(confgetfn * cgf, void *conf, becpool_t * bcp);

#endif

/*
 * sql.c 
 */

#ifdef HAVE_LIBODBC

spocp_result_t  sql_test(octet_t * arg, becpool_t * b, octet_t * blob);

#endif

/*
 * localgroup 
 */

spocp_result_t  localgroup_test(octet_t * arg, becpool_t * bcp,
				octet_t * blob);
spocp_result_t  localgroup_init(confgetfn * cgf, void *conf, becpool_t * bcp);

/*
 * system 
 */

spocp_result_t  system_test(octet_t * arg, becpool_t * bcp, octet_t * blob);

/*
 * regexp 
 */

spocp_result_t  regexp_test(octet_t * arg, becpool_t * bcp, octet_t * blob);

/*
 * difftime 
 */

spocp_result_t  difftime_test(octet_t * arg, becpool_t * bcp, octet_t * blob);

/*
 * motd 
 */

spocp_result_t  motd_test(octet_t * arg, becpool_t * bcp, octet_t * blob);

/*
 * First argument is urn name second argument the function to call for this
 * backend third argument is where cachetime will be placed has to be 0 (zero) 
 */

backend_t       spocp_be[] = {
#ifdef HAVE_LIBGDBM
	{"gdbm", gdbm_test, 0, 0},
#endif
	{"flatfile", flatfile_test, 0, 0},
	{"lastlogin", lastlogin_test, 0, 0},
	{"time", time_test, 0, 0},
#ifdef HAVE_LIBLDAP
	{"ldapset", ldapset_test, 0, 0},
#endif
	{"ipnum", ipnum_test, 0, 0},
	{"mail", addrmatch_test, 0, 0},
	{"rbl", rbl_test, 0, 0},
	{"strmatch", strmatch_test, 0, 0},
	{"strcasematch", strcasematch_test, 0, 0},
#ifdef HAVE_LIBSSL
	{"cert", cert_test, cert_init, 0},
#endif
#ifdef HAVE_LIBODBC
	{"sql", sql_test, 0, 0},
#endif
	{"difftime", difftime_test, 0, 0},
	{"localgroup", localgroup_test, localgroup_init, 0},
	{"system", system_test, 0, 0},
	{"regexp", regexp_test, 0, 0},
	{"motd", motd_test, 0, 0},
	{0, 0, 0}
};

#endif
