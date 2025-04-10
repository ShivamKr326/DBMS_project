/*
 * contrib/btree_gist/btree_float4.c
 */
#include "postgres.h"

#include "btree_gist.h"
#include "btree_utils_num.h"
#include "utils/float.h"
#include "utils/sortsupport.h"

typedef struct float4key
{
	float4		lower;
	float4		upper;
} float4KEY;

/* GiST support functions */
PG_FUNCTION_INFO_V1(gbt_float4_compress);
PG_FUNCTION_INFO_V1(gbt_float4_fetch);
PG_FUNCTION_INFO_V1(gbt_float4_union);
PG_FUNCTION_INFO_V1(gbt_float4_picksplit);
PG_FUNCTION_INFO_V1(gbt_float4_consistent);
PG_FUNCTION_INFO_V1(gbt_float4_distance);
PG_FUNCTION_INFO_V1(gbt_float4_penalty);
PG_FUNCTION_INFO_V1(gbt_float4_same);
PG_FUNCTION_INFO_V1(gbt_float4_sortsupport);

static bool
gbt_float4gt(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const float4 *) a) > *((const float4 *) b));
}
static bool
gbt_float4ge(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const float4 *) a) >= *((const float4 *) b));
}
static bool
gbt_float4eq(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const float4 *) a) == *((const float4 *) b));
}
static bool
gbt_float4le(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const float4 *) a) <= *((const float4 *) b));
}
static bool
gbt_float4lt(const void *a, const void *b, FmgrInfo *flinfo)
{
	return (*((const float4 *) a) < *((const float4 *) b));
}

static int
gbt_float4key_cmp(const void *a, const void *b, FmgrInfo *flinfo)
{
	float4KEY  *ia = (float4KEY *) (((const Nsrt *) a)->t);
	float4KEY  *ib = (float4KEY *) (((const Nsrt *) b)->t);

	if (ia->lower == ib->lower)
	{
		if (ia->upper == ib->upper)
			return 0;

		return (ia->upper > ib->upper) ? 1 : -1;
	}

	return (ia->lower > ib->lower) ? 1 : -1;
}

static float8
gbt_float4_dist(const void *a, const void *b, FmgrInfo *flinfo)
{
	return GET_FLOAT_DISTANCE(float4, a, b);
}


static const gbtree_ninfo tinfo =
{
	gbt_t_float4,
	sizeof(float4),
	8,							/* sizeof(gbtreekey8) */
	gbt_float4gt,
	gbt_float4ge,
	gbt_float4eq,
	gbt_float4le,
	gbt_float4lt,
	gbt_float4key_cmp,
	gbt_float4_dist
};


PG_FUNCTION_INFO_V1(float4_dist);
Datum
float4_dist(PG_FUNCTION_ARGS)
{
	float4		a = PG_GETARG_FLOAT4(0);
	float4		b = PG_GETARG_FLOAT4(1);
	float4		r;

	r = a - b;
	if (unlikely(isinf(r)) && !isinf(a) && !isinf(b))
		float_overflow_error();

	PG_RETURN_FLOAT4(fabsf(r));
}


/**************************************************
 * GiST support functions
 **************************************************/

Datum
gbt_float4_compress(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);

	PG_RETURN_POINTER(gbt_num_compress(entry, &tinfo));
}

Datum
gbt_float4_fetch(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);

	PG_RETURN_POINTER(gbt_num_fetch(entry, &tinfo));
}

Datum
gbt_float4_consistent(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	float4		query = PG_GETARG_FLOAT4(1);
	StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);

	/* Oid		subtype = PG_GETARG_OID(3); */
	bool	   *recheck = (bool *) PG_GETARG_POINTER(4);
	float4KEY  *kkk = (float4KEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	/* All cases served by this function are exact */
	*recheck = false;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_BOOL(gbt_num_consistent(&key, &query, &strategy,
									  GIST_LEAF(entry), &tinfo,
									  fcinfo->flinfo));
}

Datum
gbt_float4_distance(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	float4		query = PG_GETARG_FLOAT4(1);

	/* Oid		subtype = PG_GETARG_OID(3); */
	float4KEY  *kkk = (float4KEY *) DatumGetPointer(entry->key);
	GBT_NUMKEY_R key;

	key.lower = (GBT_NUMKEY *) &kkk->lower;
	key.upper = (GBT_NUMKEY *) &kkk->upper;

	PG_RETURN_FLOAT8(gbt_num_distance(&key, &query, GIST_LEAF(entry),
									  &tinfo, fcinfo->flinfo));
}

Datum
gbt_float4_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	void	   *out = palloc(sizeof(float4KEY));

	*(int *) PG_GETARG_POINTER(1) = sizeof(float4KEY);
	PG_RETURN_POINTER(gbt_num_union(out, entryvec, &tinfo, fcinfo->flinfo));
}

Datum
gbt_float4_penalty(PG_FUNCTION_ARGS)
{
	float4KEY  *origentry = (float4KEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(0))->key);
	float4KEY  *newentry = (float4KEY *) DatumGetPointer(((GISTENTRY *) PG_GETARG_POINTER(1))->key);
	float	   *result = (float *) PG_GETARG_POINTER(2);

	penalty_num(result, origentry->lower, origentry->upper, newentry->lower, newentry->upper);

	PG_RETURN_POINTER(result);
}

Datum
gbt_float4_picksplit(PG_FUNCTION_ARGS)
{
	PG_RETURN_POINTER(gbt_num_picksplit((GistEntryVector *) PG_GETARG_POINTER(0),
										(GIST_SPLITVEC *) PG_GETARG_POINTER(1),
										&tinfo, fcinfo->flinfo));
}

Datum
gbt_float4_same(PG_FUNCTION_ARGS)
{
	float4KEY  *b1 = (float4KEY *) PG_GETARG_POINTER(0);
	float4KEY  *b2 = (float4KEY *) PG_GETARG_POINTER(1);
	bool	   *result = (bool *) PG_GETARG_POINTER(2);

	*result = gbt_num_same((void *) b1, (void *) b2, &tinfo, fcinfo->flinfo);
	PG_RETURN_POINTER(result);
}

static int
gbt_float4_ssup_cmp(Datum x, Datum y, SortSupport ssup)
{
	float4KEY  *arg1 = (float4KEY *) DatumGetPointer(x);
	float4KEY  *arg2 = (float4KEY *) DatumGetPointer(y);

	/* for leaf items we expect lower == upper, so only compare lower */
	return float4_cmp_internal(arg1->lower, arg2->lower);
}

Datum
gbt_float4_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport) PG_GETARG_POINTER(0);

	ssup->comparator = gbt_float4_ssup_cmp;
	ssup->ssup_extra = NULL;

	PG_RETURN_VOID();
}
