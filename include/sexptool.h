typedef char   *(sexpargfunc) (void * comarg);

typedef struct {
	char           *var;
	sexpargfunc    *af;
	char            type;
	char            dynamic;
} sexparg_t;

sexparg_t	** parse_format(const char *, const sexparg_t [], int );
char		* sexp_constr( void *, sexparg_t ** );
void 		sexparg_free( sexparg_t **sa ) ;
