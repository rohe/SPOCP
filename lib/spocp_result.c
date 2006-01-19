#include <spocp.h>
#include <func.h>

struct code2txt {
	char			*str;
	spocp_result_t	r;
};

struct code2txt c2t[] =
 {
	{"The server is not ready yet but busy with the request", SPOCP_WORKING},
	{"The operation was a success",SPOCP_SUCCESS},
	{"Multiline response", SPOCP_MULTI},
	{"The request was denied, no error", SPOCP_DENIED},
	{"The server will close the connection",SPOCP_CLOSE},
	{"Something that was to be added already existed",SPOCP_EXISTS},
	{"Unspecified error in the Server when performing an operation",SPOCP_OPERATIONSERROR},
	{"The client actions did not follow the protocol specifications",SPOCP_PROTOCOLERROR},
	{"Unknown command",SPOCP_UNKNOWNCOMMAND},
	{"One or more arguments to an operation does not follow the prescribed syntax",SPOCP_SYNTAXERROR},
	{"Time limit exceeded",SPOCP_TIMELIMITEXCEEDED},
	{"Size limit exceeded",SPOCP_SIZELIMITEXCEEDED},
	{"Authentication information",SPOCP_AUTHDATA},
	{"Authentication failed",SPOCP_AUTHERR},
	{"Authentication in progress",SPOCP_AUTHINPROGRESS},
	{"The server is overloaded and would like you to come back later",SPOCP_BUSY},
	{"Occupied, resend the operation later",SPOCP_UNAVAILABLE},
	{"The server is unwilling to perform a operation",SPOCP_UNWILLING},
	{"Server has crashed",SPOCP_SERVER_DOWN},
	{"Local error within the Server",SPOCP_LOCAL_ERROR},
	{"Something did not happen fast enough for the server",SPOCP_TIMEOUT},
	{"The filters to a LIST command was faulty",SPOCP_FILTER_ERROR},
	{"The few or to many arguments to a protocol operation",SPOCP_PARAM_ERROR},
	{"The server is out of memory !!",SPOCP_NO_MEMORY},
	{"Command not supported",SPOCP_NOT_SUPPORTED},
	{"I/O buffer has reached the maximumsize",SPOCP_BUF_OVERFLOW},
	{"What has been received from the client sofar is not enough",SPOCP_MISSING_CHAR},
	{"Missing argument to a operation",SPOCP_MISSING_ARG},
	{"State violation",SPOCP_STATE_VIOLATION},
	{"It's OK to start the SSL negotiation",SPOCP_SSL_START},
	{"Something went wrong during the SSL negotiation",SPOCP_SSL_ERR},
	{"Could not verify a certificate",SPOCP_CERT_ERR},
	{"Something, unspecified what, went wrong",SPOCP_OTHER},
	{"While parsing the S-expression a unknown element was encountered",SPOCP_UNKNOWN_TYPE},
	{"A command or a subset of a command are unimplemented",SPOCP_UNIMPLEMENTED},
	{"No such information, usually a plugin error",SPOCP_INFO_UNAVAIL},
	{"SASL binding in progress",SPOCP_SASLBINDINPROGRESS},
	{"SSL connection alreay exists, so don't try to start another one",SPOCP_SSLCON_EXIST},
	{"Don't talk to me about this, talk to this other guy",SPOCP_REDIRECT}
};

char *spocp_result_str( spocp_result_t r)
{
	int	i;
	
	for (i = 0; i < sizeof(c2t)/sizeof(struct code2txt); i++)
		if (c2t[i].r == r)
			return c2t[i].str;
			
	return "Unkown code";
}