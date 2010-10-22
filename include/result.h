/*
 *  result.h
 *  xcode_spocp
 *
 *  Created by Roland Hedberg on 1/3/10.
 *  Copyright 2010 Umeå Universitet. All rights reserved.
 *
 */

#ifndef __RESULT_H
#define __RESULT_H

/*!
 * system health levels 
 */
/*! system is unusable */
#define	SPOCP_EMERG	0	
/*! action must be taken immediately by the administrator */
#define	SPOCP_ALERT	1
/*! critical conditions */
#define	SPOCP_CRIT	2	
/*! error conditions */
#define	SPOCP_ERR	3	
/*! warning conditions */
#define	SPOCP_WARNING	4	
/*! normal but significant condition */
#define	SPOCP_NOTICE	5	
/*! informational */
#define	SPOCP_INFO	6	
/*! debug-level messages */
#define	SPOCP_DEBUG	7	
/*! mask off the level value */
#define	SPOCP_LEVELMASK 7	

/*!
 * System debug levels
 */
/*! Parsing the S-expression */
#define	SPOCP_DPARSE	0x10
/*! Storing the rule in the database */
#define	SPOCP_DSTORE	0x20
/*! Matching a request against the database */
#define	SPOCP_DMATCH	0x40
/*! Boundary conditions */
#define	SPOCP_DBCOND	0x80
/*! Internal server things */
#define	SPOCP_DSRV	0x100


#ifndef FALSE
/*! Basic TRUE/FALSE stuff, should be defined elsewhere */
#define FALSE		0
#endif
#ifndef TRUE
/*! Basic TRUE/FALSE stuff, should be defined elsewhere */
#define TRUE		1
#endif

/*
 * ---------------------------------------- 
 */

/*!
 * result codes 
 */
typedef enum {
	/*! The server is not ready yet but busy with the request */
	SPOCP_WORKING,
	/*! The operation was a success */
	SPOCP_SUCCESS,
	/*! Multiline response */
	SPOCP_MULTI,
	/*! The request was denied, no error */
	SPOCP_DENIED,
	/*! The server will close the connection */
	SPOCP_CLOSE,
	/*! Something that was to be added already existed */
	SPOCP_EXISTS,
	/*! Unspecified error in the Server when performing an operation */
	SPOCP_OPERATIONSERROR,
	/*! The client actions did not follow the protocol specifications */
	SPOCP_PROTOCOLERROR,
	/*! Unknown command */
	SPOCP_UNKNOWNCOMMAND,
	/*! One or more arguments to an operation does not follow the prescribed syntax */
	SPOCP_SYNTAXERROR,
	/*! Time limit exceeded */
	SPOCP_TIMELIMITEXCEEDED
	/*! Size limit exceeded */,
	SPOCP_SIZELIMITEXCEEDED,
	/*! Authentication information */
	SPOCP_AUTHDATA,
	/*! Authentication failed */
	SPOCP_AUTHERR,
	/*! Authentication in progress */
	SPOCP_AUTHINPROGRESS,
	/*! The server is overloaded and would like you to come back later */
	SPOCP_BUSY,
	/*! Occupied, resend the operation later */
	SPOCP_UNAVAILABLE,
	/*! The server is unwilling to perform a operation */
	SPOCP_UNWILLING,
	/*! Server has crashed */
	SPOCP_SERVER_DOWN,
	/*! Local error within the Server */
	SPOCP_LOCAL_ERROR,
	/*! Something did not happen fast enough for the server */
	SPOCP_TIMEOUT,
	/*! The filters to a LIST command was faulty */
	SPOCP_FILTER_ERROR,
	/*! The few or to many arguments to a protocol operation */
	SPOCP_PARAM_ERROR,
	/*! The server is out of memory !! */
	SPOCP_NO_MEMORY,
	/*! Command not supported */
	SPOCP_NOT_SUPPORTED,
	/*! I/O buffer has reached the maximumsize */
	SPOCP_BUF_OVERFLOW,
	/*! What has been received from the client sofar is not enough */
	SPOCP_MISSING_CHAR,
	/*! Missing argument to a operation */
	SPOCP_MISSING_ARG,
	/*! State violation */
	SPOCP_STATE_VIOLATION,
	/*! It's OK to start the SSL negotiation */
	SPOCP_SSL_START,
	/*! Something went wrong during the SSL negotiation */
	SPOCP_SSL_ERR,
	/*! Could not verify a certificate */
	SPOCP_CERT_ERR,
	/*! Something, unspecified what, went wrong */
	SPOCP_OTHER,
	/*! While parsing the S-expression a unknown element was encountered */
	SPOCP_UNKNOWN_TYPE,
	/*! A command or a subset of a command are unimplemented */
	SPOCP_UNIMPLEMENTED,
	/*! No such information, usually a plugin error */
	SPOCP_INFO_UNAVAIL,
	/*! SASL binding in progress */
	SPOCP_SASLBINDINPROGRESS,
	/*! SSL connection alreay exists, so don't try to start another one */
	SPOCP_SSLCON_EXIST,
	/*! Don't talk to me about this, talk to this other guy */
	SPOCP_REDIRECT,
    /*! The server can for some reason not do what it was asked to do */
    SPOCP_CAN_NOT_PERFORM
    
	/*
     * SPOCP_ALIASPROBLEM , SPOCP_ALIASDEREFERENCINGPROBLEM , 
     */
} spocp_result_t;

/*! The maximum I/O buffer size */
#define SPOCP_MAXBUF	262144	/* quite arbitrary */

char *spocp_result_str( spocp_result_t r);

#endif /* _RESULT_H */