#include <varr.h>
#include <wrappers.h>

/*
 * ---------------------------------------------------------------------- 
 */

varr_t         *
varr_new(size_t n)
{
	varr_t         *j;

	j = (varr_t *) Calloc(1, sizeof(varr_t));
	j->size = n;
	j->arr = (void **) Calloc(n, sizeof(void *));

	return j;
}

varr_t         *
varr_add(varr_t * va, void *v)
{
	void          **arr;

	if (va == 0) {
		va = varr_new(4);
	} else {
		if (va->n == va->size) {
			va->size *= 2;
			arr = Realloc(va->arr, va->size * sizeof(void *));
			va->arr = arr;
		}
	}

	va->arr[va->n++] = v;

	return va;
}

void
varr_free(varr_t * va)
{
	if (va) {
		free(va->arr);
		free(va);
	}
}

int
varr_find(varr_t * va, void *v)
{
	void          **arr;
	unsigned int    i;

	if (va == 0 || va->n == 0)
		return -1;

	for (arr = va->arr, i = 0; i < va->n; i++, arr++)
		if (*arr == v)
			return (int) i;

	return -1;
}

void           *
varr_next(varr_t * va, void *v)
{
	void          **arr;
	unsigned int    i, n;

	if (va == 0 || va->n == 0 || v == 0)
		return 0;

	for (arr = va->arr, i = 0, n = va->n - 1; i < n; i++, arr++)
		if (*arr == v)
			return *++arr;

	return 0;
}

void           *
varr_first(varr_t * va)
{
	if (va == 0 || va->n == 0)
		return 0;

	return va->arr[0];
}

void           *
varr_pop(varr_t * va)
{
	if (va == 0 || va->n == 0)
		return 0;

	va->n--;

	return va->arr[va->n];
}

void           *
varr_fifo_pop(varr_t * va)
{
	int             i;
	void           *vp;

	if (va == 0 || va->n == 0)
		return 0;

	vp = va->arr[0];

	va->n--;

	for (i = 0; i < (int) va->n; i++)
		va->arr[i] = va->arr[i + 1];

	return vp;
}

varr_t         *
varr_dup(varr_t * old, dfunc * df)
{
	varr_t         *new;
	unsigned int    i;

	if (old == 0)
		return 0;

	new = varr_new(old->size);

	for (i = 0; i < old->n; i++) {
		if (df == 0)
			new->arr[i] = old->arr[i];
		else
			new->arr[i] = df(old->arr[i], 0);
	}

	return new;
}

varr_t         *
varr_or(varr_t * a, varr_t * b, int noninv)
{
	unsigned int    i;
	void           *v;

	if (a == 0) {
		if (noninv)
			return varr_dup(b, 0);
		else
			return b;
	}

	if (b == 0)
		return a;

	if (noninv) {
		for (i = 0; i < b->n; i++) {
			varr_add(a, b->arr[(int) i]);
		}
	} else {
		while (b->n) {
			v = varr_pop(b);
			if (varr_find(a, v) < 0)
				varr_add(a, v);
		}
		varr_free(b);
	}

	return a;
}

void           *
varr_common(varr_t * a, varr_t * b)
{
	unsigned int    i;

	if (a == 0 || b == 0)
		return 0;

	for (i = 0; i < a->n; i++)
		if (varr_find(b, a->arr[(int) i]) >= 0)
			return a->arr[(int) i];

	return 0;
}

void           *
varr_rm(varr_t * va, void *v)
{
	unsigned int    i;
	void           *vp = 0;

	for (i = 0; i < va->n; i++) {
		if (va->arr[i] == v) {
			vp = v;
			va->n--;

			for (; i < va->n; i++)
				va->arr[i] = va->arr[(int) i + 1];
			break;
		}
	}

	return vp;
}

void           *
varr_nth(varr_t * va, int n)
{
	if (va == 0 || n < 0 || n > (int) va->n)
		return 0;

	return va->arr[n];
}

unsigned int
varr_len(varr_t * va)
{
	return va->n;
}

/*
 * ====================================================================== 
 */
