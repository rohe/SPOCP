/*
 * implementation of a Red-Black Binary Search Tree 
 */

#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#include <struct.h>
#include <wrappers.h>
#include <func.h>
#include <db0.h>

#define BLACK 0
#define RED   1

#define hp h->p
#define hpp h->p->p
#define hr h->r
#define hrr h->r->r
#define hl h->l
#define hll h->l->l

static rbnode_t *
rb_new(item_t item, Key v, rbnode_t * l, rbnode_t * r, rbnode_t * p, char red,
       int n)
{
	rbnode_t       *new;

	new = (rbnode_t *) Malloc(sizeof(rbnode_t));
	new->item = item;
	new->v = v;
	new->l = l;
	new->p = p;
	new->r = r;
	new->red = red;
	new->n = n;

	return new;
}

/*
 * ---------------------------------------------------------- 
 */

static void
rb_free(rbnode_t * rb, ffunc * ff)
{
	if (rb) {
		if (rb->item && ff)
			ff(rb->item);
		Free(rb);
	}
}

static void
rb_tree_free(rbnode_t * rb, ffunc * ff)
{
	if (rb) {
		rb_tree_free(rb->l, ff);
		rb_tree_free(rb->r, ff);
		rb_free(rb, ff);
	}
}

void
rbt_free(rbt_t * rbt)
{
	if (rbt) {
		rb_tree_free(rbt->head, rbt->ff);
		Free(rbt);
	}
}

/*
 * ---------------------------------------------------------- 
 */

static rbnode_t *
rb_dup(rbnode_t * old, dfunc * df)
{
	rbnode_t       *new = 0;

	if (old) {
		new = rb_new(0, old->v, 0, 0, 0, old->red, old->n);
		if (df)
			new->item = df(old->item, 0);
		else
			new->item = old->item;
	}

	return new;
}

static rbnode_t *
rb_tree_dup(rbnode_t * old, dfunc * df)
{
	rbnode_t       *new = 0;

	if (old) {
		new = rb_dup(old, df);

		if (old->l) {
			new->l = rb_dup(old->l, df);
			new->l->p = new;
		}

		if (old->r) {
			new->r = rb_dup(old->r, df);
			new->r->p = new;
		}
	}

	return new;
}

/*
 * ---------------------------------------------------------- * Is this node
 * the root node ---------------------------------------------------------- 
 */

static int
root(rbnode_t * r)
{
	if (r->p == 0)
		return 1;
	else
		return 0;
}

/*
 *      (p)                (p)
 *       |                  |
 *      (y)                (x)
 *      / \                / \
 *    (x)  'c    ===>    'a   (y)
 *    / \                     / \
 *  'a  'b                  'b  'c
 *
 * Returns a pointer to the resulting node x
 */

static rbnode_t *
rotR(rbnode_t * y)
{
	rbnode_t       *x = 0, *b, *p;

	if (y) {
		x = y->l;
		if (x) {
			p = y->p;

			x->p = p;	/* was y */

			/*
			 * make sure the parent points to the right node 
			 */
			if (p) {	/* x might have been root */
				if (p->l == y)
					p->l = x;
				else
					p->r = x;
			}

			y->p = x;

			b = x->r;

			if (b) {
				y->l = b;
				b->p = y;
			} else
				y->l = 0;

			x->r = y;
		}
	}

	return x;
}

/*
 *     (p)                    (p)
 *      |                      |
 *     (x)                    (y)
 *     / \                    / \
 *   'a   (y)      ===>     (x) 'c
 *        / \               / \
 *      'b  'c            'a  'b
 *
 * Returns a pointer to the resulting node y
 */

static rbnode_t *
rotL(rbnode_t * x)
{
	rbnode_t       *y = 0, *b, *p;

	if (x) {
		y = x->r;
		if (y) {
			p = x->p;

			y->p = p;	/* was x */

			/*
			 * make sure the parent points to the right node 
			 */
			if (p) {	/* x might have been root */
				if (p->l == x)
					p->l = y;
				else
					p->r = y;
			}

			x->p = y;

			b = y->l;

			if (b) {
				x->r = b;
				b->p = x;
			} else
				x->r = 0;

			y->l = x;
		}
	}

	return x;
}

/*
 * --------------------------------------------------------------- 
 */

static void
rb_fix_insert(rbnode_t * h)
{
	rbnode_t       *p, *gp, *u;	/* parent, grandparent and uncle */

	/*
	 * fix colors 
	 */

	if (!root(h)) {

		/*
		 * while current is not root and color of parent is not red 
		 */
		for (p = hp; p && !root(p) && p->red; p = hp) {

			gp = p->p;

			if (gp->l == p) {	/* if parent is left child */
				u = gp->r;	/* uncle is right child of
						 * grandparent */

				if (u && u->red) {	/* color of uncle is
							 * red, recolor */
					p->red = BLACK;
					u->red = BLACK;
					gp->red = RED;
					h = gp;	/* become grandparent */
				} else {
					if (p->r == h) {	/* current is
								 * right child 
								 */
						h = p;
						rotL(p);	/* make it a
								 * left child */
						p = hp;
					}
					p->red = BLACK;
					gp = p->p;	/* might have changed */
					gp->red = RED;
					rotR(gp);
				}
			} else {	/* parent is a right child */
				u = gp->l;

				if (u && u->red) {	/* color of uncle is
							 * red */
					p->red = BLACK;
					u->red = BLACK;
					gp->red = RED;
					h = gp;
				} else {
					if (p->l == h) {	/* current is
								 * left child */
						h = p;
						rotR(p);	/* make it a
								 * right child 
								 */
						p = hp;
					}
					p->red = BLACK;
					gp = p->p;
					gp->red = RED;
					rotL(gp);
				}
			}
		}
	}

	for (; !root(h); h = h->p);

	h->red = BLACK;		/* root should always be black */
}

static rbnode_t *
rb_insert(rbnode_t * h, item_t item, Key v, int sw, cmpfn * cf)
{
	int             r;

	/*
	 * empty tree, root is always black 
	 */
	if (h == 0)
		return rb_new(item, v, 0, 0, 0, BLACK, 1);

	if ((r = cf(v, h->v)) < 0) {
		if (hl)
			h = rb_insert(hl, item, v, 0, cf);
		else
			h = hl = rb_new(item, v, 0, 0, h, RED, 1);
	} else if (r > 0) {
		if (hr)
			h = rb_insert(hr, item, v, 1, cf);
		else
			h = hr = rb_new(item, v, 0, 0, h, RED, 1);
	} else {		/* already got this item in the tree */
		h->n++;
	}

	return h;
}

int
rbt_insert(rbt_t * bst, item_t item)
{
	rbnode_t       *top, *h;
	Key             v;

	if (bst == 0)
		return 0;

	top = bst->head;
	v = bst->kf(item);

	h = rb_insert(top, item, v, 0, bst->cf);

	/*
	 * unnecessary to fix if not a new node 
	 */
	if (h->n == 1)
		rb_fix_insert(h);

	while (h->p)
		h = h->p;

	/*
	 * if( bst->head == 0 ) 
	 */
	bst->head = h;

	return 1;
}

/*
 * --------------------------------------------------------------- 
 */

static rbnode_t *
rb_searchR(rbnode_t * h, Key v, cmpfn * cf)
{
	Key             t;
	int             r;

	if (h == 0)
		return 0;

	t = h->v;

	if ((r = cf(v, t)) == 0)
		return h;
	else if (r < 0)
		return rb_searchR(hl, v, cf);
	else
		return rb_searchR(hr, v, cf);
}

static rbnode_t *
rh_search(rbt_t * bst, Key v)
{
	return rb_searchR(bst->head, v, bst->cf);
}

item_t
rbt_search(rbt_t * bst, Key k)
{
	rbnode_t       *rb;

	rb = rh_search(bst, k);
	if (rb)
		return (rb->item);
	else
		return 0;
}

/*
 * --------------------------------------------------------------- 
 */

static void
rb_fix_remove(rbnode_t * h, rbnode_t * p)
{
	rbnode_t       *s;

	while (!root(h) && !h->red) {
		if (p->l == h) {	/* left child */
			s = p->r;	/* sibling is right child */
			if (s->red) {
				s->red = BLACK;
				p->red = RED;
				rotL(p);
				s = p->r;
			}

			if (s->l->red == BLACK && s->r->red == BLACK) {
				s->red = RED;
				h = p;
				p = hp;
			} else {
				if (s->r->red == BLACK) {
					s->l->red = BLACK;
					s->red = RED;
					rotR(s);
					s = p->r;
				}

				s->red = p->red;
				p->red = BLACK;
				s->r->red = BLACK;
				rotL(p);
				break;
			}
		} else {
			s = p->l;	/* sibling is left child */
			if (s->red) {
				s->red = BLACK;
				p->red = RED;
				rotR(p);
				s = p->l;
			}

			if (s->l->red == BLACK && s->r->red == BLACK) {
				s->red = RED;
				h = p;
				p = hp;
			} else {
				if (s->l->red == BLACK) {
					s->r->red = BLACK;
					s->red = RED;
					rotL(s);
					s = p->l;
				}

				s->red = p->red;
				p->red = BLACK;
				s->l->red = BLACK;
				rotR(p);
				break;
			}
		}
	}
	for (; !root(h); h = h->p);

	h->red = BLACK;		/* root should always be black */
}

/*
 * successor is next higher or next lower stored object 
 */
static rbnode_t *
rbt_successor(rbnode_t * h)
{
	rbnode_t       *s;

	if (h->r) {		/* next higher */

		s = h->r;
		if (s->l == 0)
			return s;

		/*
		 * the leftmost of the nodes to the right 
		 */
		for (; s->l; s = s->l);

		return s->l;
	} else if (h->l) {	/* next lower */
		s = h->l;
		if (s->r == 0)
			return s;

		/*
		 * the rightmost of the nodes to the left 
		 */
		for (; s->r; s = s->r);

		return s->r;
	} else
		return 0;
}

static void
replace_content(rbt_t * rh, rbnode_t * a, rbnode_t * b)
{
	rh->ff(a->item);
	if (b) {
		a->item = b->item;
		b->item = 0;
		a->v = b->v;
	} else {
		a->item = 0;
		a->v = 0;
	}
}

int
rbt_remove(rbt_t * rh, Key v)
{
	rbnode_t       *h = 0, *s;
	int             c;

	if (rh == 0)
		return 0;

	h = rh_search(rh, v);

	if (h == 0)
		return 0;

	/*
	 * only one item in the tree ?? 
	 */
	if (rh->head == h && hr == 0 && hl == 0) {
		rb_free(h, rh->ff);
		rh->head = 0;

		return 1;
	}

	if (h->n > 1) {
		h->n--;
		return 1;
	}

	c = h->red;		/* color of node */

	if (hl == 0 || hr == 0) {
		;		/* no children */
	} else {		/* have to pick a successor ( a leaf node ) */

		s = rbt_successor(h);

		if (s) {	/* shouldn't happen, since there has to be a
				 * child at this point */
			/*
			 * replace the content i h with that in s 
			 */
			replace_content(rh, h, s);

			h = s;
		}
	}

	rb_fix_remove(h, hp);

	rb_free(h, rh->ff);

	if (hp) {
		if (hp->l == h)
			hp->l = 0;
		else
			hp->r = 0;
	} else {
		;
	}

	return 1;
}

static void
rb_print(rbnode_t * h, char *path, pfunc * pf)
{
	int             l = strlen(path);
	char           *sp, *tmp;

	tmp = pf(h->item);
	printf("(%d)%s:%s\n", h->red, path, tmp);
	Free(tmp);

	if (hl) {
		path[l] = 'l';
		rb_print(hl, path, pf);
	}
	if (hr) {
		for (sp = &path[l]; *sp; sp++)
			*sp = '\0';
		path[l] = 'r';
		rb_print(hr, path, pf);
	}
}

void
rbt_print(rbt_t * rh)
{
	char            path[129];

	memset(path, 0, 129);

	rb_print(rh->head, path, rh->pf);

}

/*
 * --------------------------------------------------------- 
 */

rbt_t          *
rbt_init(cmpfn * cf, ffunc * ff, kfunc * kf, dfunc * df, pfunc * pf)
{
	rbt_t          *bst;

	bst = (rbt_t *) Malloc(sizeof(rbt_t));

	bst->cf = cf;
	bst->ff = ff;
	bst->kf = kf;
	bst->pf = pf;
	bst->df = df;
	bst->head = 0;

	return bst;
}

rbt_t          *
rbt_dup(rbt_t * old, int dyn)
{
	rbt_t          *new = 0;

	if (old) {
		new = rbt_init(old->cf, old->ff, old->kf, old->df, old->pf);

		if (dyn)
			new->head = rb_tree_dup(old->head, old->df);
		else
			new->head = rb_tree_dup(old->head, 0);
	}

	return new;
}

/*
 * --------------------------------------------------------- 
 */

static int
rbnode_count(rbnode_t * r)
{
	int             n;

	if (r == 0)
		return 0;

	n = 1;

	n += rbnode_count(r->l);
	n += rbnode_count(r->r);

	return n;
}

int
rbt_items(rbt_t * rh)
{
	if (rh == 0)
		return 0;

	return rbnode_count(rh->head);
}

static varr_t  *
rbnode_get_item(rbnode_t * r, varr_t * dest)
{
	if (r == 0)
		return dest;

	if (r->l)
		dest = rbnode_get_item(r->l, dest);

	dest = varr_add(dest, (void *) r->item);

	if (r->r)
		dest = rbnode_get_item(r->r, dest);

	return dest;
}

varr_t         *
rbt2varr(rbt_t * rh)
{
	return rbnode_get_item(rh->head, 0);
}
