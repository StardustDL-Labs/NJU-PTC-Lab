#include <string.h>
#include <stdlib.h>
#include "symbol.h"
#include "type.h"
#include "assert.h"

static type *unit = NULL;
static type *any = NULL;
static type *never = NULL;
static type *metaint = NULL, *metafloat = NULL;

static type *new_type(TYPE_CLASS cls)
{
    type *result = new (type);
    result->cls = cls;
    result->hash = 0;
    return result;
}

type *new_type_array(type *base, int rank, int *lens)
{
    type *result = new_type(TC_ARRAY);
    result->base = base;
    result->rank = rank;
    result->lens = lens;
    return result;
}

type *new_type_func(int argc, type **args, type *ret)
{
    type *result = new_type(TC_FUNC);
    result->argc = argc;
    result->args = args;
    result->ret = ret;
    return result;
}

type *new_type_struct(int memc, symbol **mems)
{
    type *result = new_type(TC_STRUCT);
    result->memc = memc;
    result->mems = mems;
    return result;
}

type *new_type_unit()
{
    if (unit == NULL)
        unit = new_type(TC_UNIT);
    return unit;
}

type *new_type_any()
{
    if (any == NULL)
        any = new_type(TC_ANY);
    return any;
}

type *new_type_never()
{
    if (never == NULL)
        never = new_type(TC_NEVER);
    return never;
}

type *new_type_meta(METATYPE_type metatype)
{
    switch (metatype)
    {
    case MT_INT:
        if (metaint == NULL)
        {
            metaint = new_type(TC_META);
            metaint->metatype = MT_INT;
        }
        return metaint;
    case MT_FLOAT:
        if (metafloat == NULL)
        {
            metafloat = new_type(TC_META);
            metafloat->metatype = MT_FLOAT;
        }
        return metafloat;
    }
    assert(0);
}

bool type_full_eq(type *a, type *b, bool strict_arr)
{
    if (a == b)
        return true;
    if (a->cls == TC_ANY || b->cls == TC_ANY)
        return true;
    if (a->cls == TC_NEVER || b->cls == TC_NEVER)
        return true;
    if (a->cls != b->cls)
        return false;
    switch (a->cls)
    {
    case TC_META:
        return a->metatype == b->metatype;
    case TC_UNIT:
        return true;
    case TC_ARRAY:
        if (a->rank != b->rank)
            return false;
        if (strict_arr)
        {
            for (int i = 0; i < a->rank; i++)
                if (a->lens[i] != b->lens[i])
                    return false;
        }
        if (!type_full_eq(a->base, b->base, false))
            return false;
        return true;
    case TC_FUNC:
        if (a->argc != b->argc)
            return false;
        if (!type_full_eq(a->ret, b->ret, false))
            return false;
        for (int i = 0; i < a->argc; i++)
            if (!type_full_eq(a->args[i], b->args[i], false))
                return false;
        return true;
    case TC_STRUCT:
        if (a->memc != b->memc)
            return false;
        for (int i = 0; i < a->memc; i++)
        {
            // if (strcmp(a->mems[i]->name, b->mems[i]->name) != 0) return false;
            if (!type_full_eq(a->mems[i]->tp, b->mems[i]->tp, true))
                return false;
        }
        return true;
    }
    assert(0);
}

type *type_array_descending(type *t)
{
    assert(t->cls == TC_ARRAY);
    if (t->rank > 1)
    {
        type *result = new_type(TC_ARRAY);
        result->rank = t->rank - 1;
        result->lens = t->lens + 1;
        return result;
    }
    else
    {
        assert(t->rank == 1);
        return t->base;
    }
}

bool type_can_call(type *a)
{
    return a->cls == TC_FUNC;
}

bool type_can_callargs(type *a, int argc, type **args)
{
    if (!type_can_call(a))
        return false;
    if (argc != a->argc)
        return false;
    for (int i = 0; i < argc; i++)
    {
        if (!type_full_eq(args[i], a->args[i], false))
            return false;
    }
    return true;
}

bool type_can_index(type *a)
{
    return a->cls == TC_ARRAY;
}

bool type_can_member(type *a)
{
    return a->cls == TC_STRUCT;
}

symbol *type_can_membername(type *a, char *name)
{
    symbol *member = NULL;
    for (int i = 0; i < a->memc; i++)
    {
        symbol *sym = a->mems[i];
        if (strcmp(sym->name, name) == 0)
        {
            member = sym;
            break;
        }
    }
    return member;
}

bool type_can_logic(type *a)
{
    return a->cls == TC_META && a->metatype == MT_INT;
}

bool type_can_arithmetic(type *a)
{
    return a->cls == TC_META;
}

bool type_can_arithmetic2(type *a, type *b)
{
    return type_can_arithmetic(a) && type_can_arithmetic(b) && a->metatype == b->metatype;
}