#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"
#include "ast.h"
#include "symbol.h"
#include "type.h"
#include "common.h"

static void semantics_error(int type, int lineno, char *format, ...);
static void semantics_log(int lineno, char *format, ...);

#pragma region error functions

static void error_var_nodef(int lineno, char *name)
{
    semantics_error(1, lineno, "No def var: %s", name);
}
static void error_func_nodef(int lineno, char *name)
{
    semantics_error(2, lineno, "No def func: %s", name);
}
static void error_var_redef(int lineno, char *name)
{
    semantics_error(3, lineno, "Re def var: %s", name);
}
static void error_func_redef(int lineno, char *name)
{
    semantics_error(4, lineno, "Re def func: %s", name);
}
static void error_assign_type(int lineno)
{
    semantics_error(5, lineno, "assign type not match");
}
static void error_assign_rval(int lineno)
{
    semantics_error(6, lineno, "assign to rval");
}
static void error_op_type(int lineno)
{
    semantics_error(7, lineno, "op type not match");
}
static void error_return_type(int lineno)
{
    semantics_error(8, lineno, "return type not match");
}
static void error_call_type(int lineno)
{
    semantics_error(9, lineno, "func call arg type not match");
}
static void error_index(int lineno)
{
    semantics_error(10, lineno, "not indexable");
}
static void error_call(int lineno)
{
    semantics_error(11, lineno, "not callable");
}
static void error_index_arg(int lineno)
{
    semantics_error(12, lineno, "not integer in index");
}
static void error_member(int lineno)
{
    semantics_error(13, lineno, "not memberable");
}
static void error_member_nodef(int lineno, char *name)
{
    semantics_error(14, lineno, "no member: %s", name);
}
static void error_member_def(int lineno, char *name)
{
    semantics_error(15, lineno, "invalid member def");
}
static void error_struct_redef(int lineno, char *name)
{
    semantics_error(16, lineno, "struct redef");
}
static void error_struct_nodef(int lineno, char *name)
{
    semantics_error(17, lineno, "struct nodef");
}
static void error_func_decnodef(int lineno, char *name)
{
    semantics_error(18, lineno, "func dec but no def");
}
static void error_func_decconflict(int lineno, char *name)
{
    semantics_error(19, lineno, "func dec conflict");
}
#pragma endregion

#pragma region structs

typedef struct
{
    symbol_table *syms;
    type *declare_type;
    type *ret_type;
    bool in_struct;
    bool in_vardec;
} env;

typedef struct
{
    type *tp;
    int value;
} SES_INT;

typedef struct
{
    type *tp;
} SES_FLOAT;

typedef struct
{
    char *name;
    symbol *sym;
} SES_ID;

typedef struct
{
    type *tp;
} SES_TYPE;

typedef struct
{
    type *tp;
    char *struct_name;
} SES_Specifier;

typedef struct
{
    symbol *sym;
    int lineno;
    env *ev;
} SES_FunDec;

typedef struct
{
    char *name;
} SES_Tag;

typedef struct __SES_Exp
{
    type *tp;
    int lineno;
    struct __SES_Exp *next;
} SES_Exp;

typedef struct __int_list
{
    int data;
    struct __int_list *next;
} int_list;

typedef struct __SES_VarDec
{
    symbol *sym;
    bool hasinit;
    int lineno;
    int_list *lens;
    struct __SES_VarDec *next;
} SES_VarDec;

static bool is_struct_specifier(SES_Specifier *sp)
{
    return sp->struct_name != NULL;
}
static bool is_struct_specifier_dec(SES_Specifier *sp)
{
    return is_struct_specifier(sp) && sp->tp == NULL;
}
static bool is_struct_specifier_def(SES_Specifier *sp)
{
    return is_struct_specifier(sp) && sp->tp != NULL;
}
static bool resolve_struct_specifier_dec(SES_Specifier *sp, env *ev)
{
    assert(is_struct_specifier(sp));
    if (is_struct_specifier_dec(sp))
    {
        symbol *existsym = st_find(ev->syms, sp->struct_name);
        if (existsym == NULL || existsym->tp->cls != TC_STRUCT || existsym->state == SS_DEC)
        {
            sp->tp = new_type_never();
            return false;
        }
        else
        {
            sp->tp = existsym->tp;
            return true;
        }
    }
    return true;
}

#pragma endregion

#pragma region
static SES_INT *analyse_INT(ast *tree, env *ev);
static SES_FLOAT *analyse_FLOAT(ast *tree, env *ev);
static SES_ID *analyse_ID(ast *tree, env *ev);
static SES_TYPE *analyse_TYPE(ast *tree, env *ev);
static void analyse_Program(ast *tree, env *ev);
static void analyse_ExtDefList(ast *tree, env *ev);
static void analyse_ExtDef(ast *tree, env *ev);
static SES_VarDec *analyse_ExtDecList(ast *tree, env *ev);
static SES_Specifier *analyse_Specifier(ast *tree, env *ev);
static SES_Specifier *analyse_StructSpecifier(ast *tree, env *ev);
static SES_Tag *analyse_OptTag(ast *tree, env *ev);
static SES_Tag *analyse_Tag(ast *tree, env *ev);
static SES_VarDec *analyse_VarDec(ast *tree, env *ev);
static SES_FunDec *analyse_FunDec(ast *tree, env *ev);
static SES_VarDec *analyse_VarList(ast *tree, env *ev);
static SES_VarDec *analyse_ParamDec(ast *tree, env *ev);
static void analyse_CompSt(ast *tree, env *ev);
static void analyse_StmtList(ast *tree, env *ev);
static void analyse_Stmt(ast *tree, env *ev);
static void analyse_DefList(ast *tree, env *ev);
static void analyse_Def(ast *tree, env *ev);
static SES_VarDec *analyse_DecList(ast *tree, env *ev);
static SES_VarDec *analyse_Dec(ast *tree, env *ev);
static SES_Exp *analyse_Exp(ast *tree, env *ev);
static SES_Exp *analyse_Args(ast *tree, env *ev);
#pragma endregion

static SES_INT *analyse_INT(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "INT");
    assert(tree->type == ST_INT);
    SES_INT *tag = new (SES_INT);
    tag->value = tree->t_uint;
    tag->tp = new_type_meta(MT_INT);
    tree->tag = tag;
    return tag;
}
static SES_FLOAT *analyse_FLOAT(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "FLOAT");
    assert(tree->type == ST_FLOAT);
    SES_FLOAT *tag = new (SES_FLOAT);
    tag->tp = new_type_meta(MT_FLOAT);
    tree->tag = tag;
    return tag;
}
static SES_ID *analyse_ID(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s (%s)", "ID", tree->t_str);
    assert(tree->type == ST_ID);
    symbol *sym = st_find(ev->syms, tree->t_str);
    SES_ID *tag = new (SES_ID);
    tag->name = tree->t_str;
    tag->sym = sym;
    tree->tag = tag;
    return tag;
}
static SES_TYPE *analyse_TYPE(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "TYPE");
    assert(tree->type == ST_TYPE);
    SES_TYPE *tag = new (SES_TYPE);
    tag->tp = new_type_meta(tree->t_type);
    tree->tag = tag;
    return tag;
}
static void analyse_Program(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Program");
    // Program : ExtDefList
    //     ;

    assert(tree->type == ST_Program);
    analyse_ExtDefList(tree->children[0], ev);

    symbol_item *cur = ev->syms->table;
    while (cur != NULL)
    {
        symbol *sym = cur->sym;
        if (sym->tp->cls == TC_FUNC && sym->state == SS_DEC)
        {
            error_func_decnodef(sym->lineno, sym->name);
        }
        cur = cur->next;
    }
}
static void analyse_ExtDefList(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "ExtDefList");
    // ExtDefList : ExtDef ExtDefList
    //     | /* empty */
    //     ;

    assert(tree->type == ST_ExtDefList);

    if (tree->count == 2)
    {
        analyse_ExtDef(tree->children[0], ev);
        analyse_ExtDefList(tree->children[1], ev);
    }
}
static void analyse_ExtDef(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "ExtDef");
    // ExtDef : Specifier ExtDecList SEMI
    //     | Specifier SEMI
    //     | Specifier FunDec CompSt
    //     | Specifier FunDec SEMI
    //     ;
    assert(tree->type == ST_ExtDef);

    SES_Specifier *specifier = analyse_Specifier(tree->children[0], ev);

    if (specifier->struct_name != NULL) // struct dec/def
    {
        symbol *existsym = st_find(ev->syms, specifier->struct_name);
        if (existsym != NULL)
        {
            if (existsym->tp->cls == TC_STRUCT)
            {
                if (existsym->state == SS_DEF && specifier->tp != NULL) // struct and struct
                {
                    error_struct_redef(tree->first_line, specifier->struct_name);
                }
                else
                {
                    if (specifier->tp != NULL)
                        existsym->state = SS_DEF;
                }
            }
            else // var and struct
            {
                error_struct_redef(tree->first_line, specifier->struct_name);
            }
        }
        else if (specifier->tp == NULL) // struct dec
        {
            symbol *sym = new_symbol(specifier->struct_name, tree->first_line, new_type_struct(0, NULL), SS_DEC);
            st_pushfront(ev->syms, sym);
        }
        else
        {
            symbol *sym = new_symbol(specifier->struct_name, tree->first_line, specifier->tp, SS_DEF);
            st_pushfront(ev->syms, sym);
        }
    }

    if (tree->children[1]->type == ST_ExtDecList)
    {
        if (is_struct_specifier(specifier))
        {
            if (!resolve_struct_specifier_dec(specifier, ev))
            {
                error_struct_nodef(tree->first_line, specifier->struct_name);
            }
        }

        ev->declare_type = specifier->tp;
        SES_VarDec *decs = analyse_ExtDecList(tree->children[1], ev);
        while (decs != NULL)
        {
            symbol *existsym = st_findonly(ev->syms, decs->sym->name);
            if (existsym != NULL)
            {
                error_var_redef(decs->lineno, decs->sym->name);
            }
            else
            {
                st_pushfront(ev->syms, decs->sym);
            }
            decs = decs->next;
        }
        ev->declare_type = NULL;
    }
    else if (tree->children[1]->type == ST_FunDec)
    {
        ev->declare_type = specifier->tp;
        SES_FunDec *sf = analyse_FunDec(tree->children[1], ev);
        if (tree->children[2]->type == ST_CompSt) // function definition
        {
            env *funcev = sf->ev;
            funcev->ret_type = specifier->tp;
            analyse_CompSt(tree->children[2], funcev);
            funcev->ret_type = NULL;
            sf->sym->state = SS_DEF;
        }
        else if (tree->children[2]->type == ST_SEMI) // function declare
        {
            sf->sym->state = SS_DEC;
        }
        symbol *existsym = st_findonly(ev->syms, sf->sym->name);
        int lineno = tree->children[1]->first_line;
        if (existsym != NULL)
        {
            if (existsym->tp->cls == TC_FUNC)
            {
                if (existsym->state == SS_DEF)
                {
                    error_func_redef(lineno, sf->sym->name);
                }
                else if (!type_full_eq(existsym->tp, sf->sym->tp, false))
                {
                    error_func_decconflict(lineno, sf->sym->name);
                }
                else if (sf->sym->state == SS_DEF)
                {
                    existsym->state = SS_DEF;
                }
            }
            else
            {
                error_func_redef(lineno, sf->sym->name);
            }
        }
        else
        {
            st_pushfront(ev->syms, sf->sym);
        }
    }
    else if (tree->children[1]->type == ST_SEMI)
    {
    }
}
static SES_VarDec *analyse_ExtDecList(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "ExtDecList");
    // ExtDecList : VarDec
    //     | VarDec COMMA ExtDecList
    //     ;
    assert(tree->type == ST_ExtDecList);

    SES_VarDec *first = analyse_VarDec(tree->children[0], ev);
    if (tree->count == 3)
    {
        first->next = analyse_ExtDecList(tree->children[2], ev);
    }
    tree->tag = first;
    return first;
}
static SES_Specifier *analyse_Specifier(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Specifier");
    // Specifier : TYPE
    //     | StructSpecifier
    //     ;
    assert(tree->type == ST_Specifier);
    ast *child = tree->children[0];
    SES_Specifier *tag = NULL;
    if (child->type == ST_TYPE)
    {
        tag = new (SES_Specifier);
        SES_TYPE *ct = analyse_TYPE(child, ev);
        tag->tp = ct->tp;
    }
    else if (child->type == ST_StructSpecifier)
    {
        tag = analyse_StructSpecifier(child, ev);
    }
    assert(tag != NULL);
    tree->tag = tag;
    return tag;
}
static SES_Specifier *analyse_StructSpecifier(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "StructSpecifier");
    // StructSpecifier : STRUCT OptTag LC DefList RC
    //     | STRUCT Tag
    //     ;
    assert(tree->type == ST_StructSpecifier);
    SES_Specifier *tag = NULL;
    if (tree->count == 2)
    {
        SES_Tag *ctag = analyse_Tag(tree->children[1], ev);

        tag = new (SES_Specifier);
        tag->tp = NULL;
        tag->struct_name = ctag->name;
    }
    else
    {
        SES_Tag *ctag = analyse_OptTag(tree->children[1], ev);

        env *cev = new (env);
        cev->syms = new_symbol_table(ev->syms);
        cev->in_struct = true;
        analyse_DefList(tree->children[3], cev);
        cev->in_struct = false;

        tag = new (SES_Specifier);
        tag->struct_name = ctag->name;
        int memlen = st_len(cev->syms);
        type *tp = new_type_struct(memlen, st_revto_arr(cev->syms));
        tag->tp = tp;
    }
    tree->tag = tag;
    assert(tag != NULL);
    return tree->tag;
}
static SES_Tag *analyse_OptTag(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "OptTag");
    static int struct_id = 0;
    // OptTag : ID
    //     | /* empty */
    //     ;
    assert(tree->type == ST_OptTag);
    SES_Tag *tag = new (SES_Tag);
    if (tree->count == 0)
    {
        struct_id++;
        sprintf(tree->t_str, "@STRUCT%d", struct_id);
        tag->name = tree->t_str;
    }
    else
    {
        SES_ID *id = analyse_ID(tree->children[0], ev);
        // AT: id->sym not null means conflict struct name
        tag->name = id->name;
    }
    tree->tag = tag;
    return tag;
}
static SES_Tag *analyse_Tag(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Tag");
    // Tag : ID
    //     ;
    assert(tree->type == ST_Tag);
    SES_Tag *tag = new (SES_Tag);
    SES_ID *id = analyse_ID(tree->children[0], ev);
    // AT: id->sym not null means conflict struct name
    tag->name = id->name;
    tree->tag = tag;
    return tag;
}
static SES_VarDec *analyse_VarDec(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "VarDec");
    // VarDec : ID
    //     | VarDec LB INT RB
    //     ;
    assert(tree->type == ST_VarDec);
    assert(ev->declare_type != NULL);
    bool invardec = ev->in_vardec;
    if (tree->count == 1)
    {
        SES_ID *id = analyse_ID(tree->children[0], ev);
        SES_VarDec *tag = new (SES_VarDec);
        if (invardec)
        {
            tag->sym = new_symbol(id->name, tree->first_line, NULL, SS_DEC);
        }
        else
        {
            tag->sym = new_symbol(id->name, tree->first_line, ev->declare_type, SS_DEC);
        }
        tag->lineno = tree->first_line;
        tree->tag = tag;
        return tag;
    }
    else
    {
        ev->in_vardec = true;
        SES_VarDec *subvar = analyse_VarDec(tree->children[0], ev);
        SES_INT *len = analyse_INT(tree->children[2], ev);
        ev->in_vardec = invardec;
        int_list *li = new (int_list);
        li->data = len->value;
        li->next = subvar->lens;
        subvar->lens = li;
        subvar->lineno = tree->first_line;
        if (invardec)
        {
            tree->tag = subvar;
            return subvar;
        }
        else
        {
            int listlen = 0, i = 0;
            int_list *cur = subvar->lens;
            while (cur != NULL)
            {
                listlen++;
                cur = cur->next;
            }
            int *lens = (int *)malloc(listlen * sizeof(int));
            cur = subvar->lens;
            while (cur != NULL)
            {
                lens[listlen - 1 - i] = cur->data;
                i++;
                cur = cur->next;
            }
            type *arrtp = new_type_array(ev->declare_type, listlen, lens);
            subvar->sym->tp = arrtp;
            return subvar;
        }
    }
    assert(0);
}
static SES_FunDec *analyse_FunDec(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "FunDec");
    assert(ev->declare_type != NULL);
    // FunDec : ID LP VarList RP
    //     | ID LP RP
    //     ;
    assert(tree->type == ST_FunDec);

    SES_ID *id = analyse_ID(tree->children[0], ev);
    SES_FunDec *tag = new (SES_FunDec);
    tag->lineno = tree->first_line;

    env *funcev = new (env);
    funcev->syms = new_symbol_table(ev->syms);
    tag->ev = funcev;

    if (tree->count == 3)
    {
        symbol *sym = new_symbol(id->name, tree->first_line, new_type_func(0, NULL, ev->declare_type), SS_DEC);
        tag->sym = sym;
    }
    else
    {
        SES_VarDec *varlist = analyse_VarList(tree->children[2], funcev);
        int len = 0;
        SES_VarDec *cur = varlist;
        while (cur != NULL)
        {
            len++;
            cur = cur->next;
        }
        type **args = newarr(type, len);
        cur = varlist;
        int i = 0;
        while (cur != NULL)
        {
            args[i] = cur->sym->tp;
            i++;
            cur = cur->next;
        }
        type *ftp = new_type_func(len, args, ev->declare_type);
        symbol *sym = new_symbol(id->name, tree->first_line, ftp, SS_DEC);
        tag->sym = sym;
    }
    tree->tag = tag;
    return tag;
}
static SES_VarDec *analyse_VarList(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "VarList");
    // VarList : ParamDec COMMA VarList
    //     | ParamDec
    //     ;
    assert(tree->type == ST_VarList);

    SES_VarDec *first = analyse_ParamDec(tree->children[0], ev);
    if (tree->count == 3)
    {
        first->next = analyse_VarList(tree->children[2], ev);
    }
    tree->tag = first;
    return first;
}
static SES_VarDec *analyse_ParamDec(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "ParamDec");
    // ParamDec : Specifier VarDec
    //     ;
    assert(tree->type == ST_ParamDec);

    SES_Specifier *specifier = analyse_Specifier(tree->children[0], ev);
    if (is_struct_specifier(specifier))
    {
        if (!resolve_struct_specifier_dec(specifier, ev))
        {
            error_struct_nodef(tree->first_line, specifier->struct_name);
        }
    }
    assert(ev->declare_type == NULL);
    ev->declare_type = specifier->tp;

    SES_VarDec *vardec = analyse_VarDec(tree->children[1], ev);
    symbol *existsym = st_findonly(ev->syms, vardec->sym->name);
    if (existsym != NULL)
    {
        error_var_redef(vardec->lineno, vardec->sym->name);
    }
    else
    {
        st_pushfront(ev->syms, vardec->sym);
    }

    ev->declare_type = NULL;

    tree->tag = vardec;

    return vardec;
}
static void analyse_CompSt(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "CompSt");
    // CompSt : LC DefList StmtList RC
    //     ;
    assert(tree->type == ST_CompSt);

    symbol_table *cst = new_symbol_table(ev->syms);
    env *cenv = new (env);
    cenv->ret_type = ev->ret_type;
    cenv->syms = cst;

    analyse_DefList(tree->children[1], cenv);

    analyse_StmtList(tree->children[2], cenv);
}
static void analyse_StmtList(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "StmtList");
    // StmtList : Stmt StmtList
    //     | /* empty */
    //     ;
    assert(tree->type == ST_StmtList);

    if (tree->count == 0)
    {
    }
    else
    {
        analyse_Stmt(tree->children[0], ev);
        analyse_StmtList(tree->children[1], ev);
    }
}
static void analyse_Stmt(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Stmt");
    // Stmt : Exp SEMI
    //     | CompSt
    //     | RETURN Exp SEMI
    //     | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    //     | IF LP Exp RP Stmt ELSE Stmt
    //     | WHILE LP Exp RP Stmt
    //     ;
    assert(tree->type == ST_Stmt);
    if (tree->children[0]->type == ST_Exp)
    {
        analyse_Exp(tree->children[0], ev);
    }
    else if (tree->children[0]->type == ST_CompSt)
    {
        analyse_CompSt(tree->children[0], ev);
    }
    else if (tree->children[0]->type == ST_RETURN)
    {
        assert(ev->ret_type != NULL);
        SES_Exp *exp = analyse_Exp(tree->children[1], ev);
        if (!type_full_eq(ev->ret_type, exp->tp, false))
        {
            error_return_type(tree->children[1]->first_line);
        }
    }
    else if (tree->children[0]->type == ST_IF)
    {
        SES_Exp *exp = analyse_Exp(tree->children[2], ev);
        if (exp->tp->cls != TC_META || exp->tp->metatype != MT_INT)
        {
            error_op_type(tree->children[2]->first_line);
        }
        if (tree->count == 7) // if-else
        {
            analyse_Stmt(tree->children[4], ev);
            analyse_Stmt(tree->children[6], ev);
        }
        else // only if
        {
            analyse_Stmt(tree->children[4], ev);
        }
    }
}
static void analyse_DefList(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "DefList");
    // DefList : Def DefList
    //     | /* empty */
    //     ;
    assert(tree->type == ST_DefList);

    if (tree->count == 0)
    {
    }
    else
    {
        analyse_Def(tree->children[0], ev);
        analyse_DefList(tree->children[1], ev);
    }
}
static void analyse_Def(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Def");
    // Def : Specifier DecList SEMI
    //     ;
    assert(tree->type == ST_Def);

    SES_Specifier *specifier = analyse_Specifier(tree->children[0], ev);

    if (is_struct_specifier(specifier))
    {
        if (!resolve_struct_specifier_dec(specifier, ev))
        {
            error_struct_nodef(tree->first_line, specifier->struct_name);
        }
    }
    ev->declare_type = specifier->tp;
    SES_VarDec *decs = analyse_DecList(tree->children[1], ev);
    while (decs != NULL)
    {
        symbol *existsym = st_findonly(ev->syms, decs->sym->name);
        if (existsym != NULL)
        {
            if (ev->in_struct)
            {
                error_member_def(decs->lineno, decs->sym->name);
            }
            else
            {
                error_var_redef(decs->lineno, decs->sym->name);
            }
        }
        else
        {
            if (ev->in_struct && decs->hasinit) // init in struct
            {
                error_member_def(decs->lineno, decs->sym->name);
            }
            else
            {
                st_pushfront(ev->syms, decs->sym);
            }
        }
        decs = decs->next;
    }
    ev->declare_type = NULL;
}
static SES_VarDec *analyse_DecList(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "DecList");
    // DecList : Dec
    //     | Dec COMMA DecList
    //     ;
    assert(tree->type == ST_DecList);

    SES_VarDec *first = analyse_Dec(tree->children[0], ev);
    if (tree->count > 1)
    {
        first->next = analyse_DecList(tree->children[2], ev);
    }
    tree->tag = first;
    return first;
}
static SES_VarDec *analyse_Dec(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Dec");
    // Dec : VarDec
    //     | VarDec ASSIGNOP Exp
    //     ;
    assert(tree->type == ST_Dec);

    SES_VarDec *var = analyse_VarDec(tree->children[0], ev);
    if (tree->count == 1)
    {
    }
    else
    {
        var->hasinit = true;
        SES_Exp *exp = analyse_Exp(tree->children[2], ev);
        if (!type_full_eq(var->sym->tp, exp->tp, false))
        {
            error_assign_type(tree->first_line);
        }
    }
    tree->tag = var;
    return var;
}
static SES_Exp *analyse_Exp(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Exp");
    // Exp : Exp ASSIGNOP Exp
    //     | Exp AND Exp
    //     | Exp OR Exp
    //     | Exp RELOP Exp
    //     | Exp PLUS Exp
    //     | Exp MINUS Exp
    //     | Exp STAR Exp
    //     | Exp DIV Exp
    //     | LP Exp RP
    //     | MINUS Exp %prec NEG
    //     | NOT Exp
    //     | ID LP Args RP
    //     | ID LP RP
    //     | Exp LB Exp RB
    //     | Exp DOT ID
    //     | ID
    //     | INT
    //     | FLOAT
    //     ;
    assert(tree->type == ST_Exp);

    SES_Exp *tag = new (SES_Exp);

    if (tree->count == 1)
    {
        if (tree->children[0]->type == ST_INT) // INT
        {
            SES_INT *val = analyse_INT(tree->children[0], ev);
            tag->tp = val->tp;
        }
        else if (tree->children[0]->type == ST_FLOAT) // FLOAT
        {
            SES_FLOAT *val = analyse_FLOAT(tree->children[0], ev);
            tag->tp = val->tp;
        }
        else if (tree->children[0]->type == ST_ID) // ID
        {
            SES_ID *val = analyse_ID(tree->children[0], ev);
            if (val->sym == NULL)
            {
                error_var_nodef(tree->first_line, val->name);
                tag->tp = new_type_never();
            }
            else
            {
                tag->tp = val->sym->tp;
            }
        }
    }
    else if (tree->count == 2)
    {
        SES_Exp *exp = analyse_Exp(tree->children[1], ev);
        if (tree->children[0]->type == ST_MINUS) // MINUS Exp
        {
            if (!type_can_arithmetic(exp->tp))
            {
                error_op_type(tree->children[1]->first_line);
                tag->tp = exp->tp;
            }
            else
            {
                tag->tp = exp->tp;
            }
        }
        else if (tree->children[0]->type == ST_NOT) // NOT Exp
        {
            if (!type_can_logic(exp->tp))
            {
                error_op_type(tree->children[1]->first_line);
                tag->tp = exp->tp;
            }
            else
            {
                tag->tp = exp->tp;
            }
        }
    }
    else if (tree->count == 3)
    {
        if (tree->children[0]->type == ST_LP) // LP Exp RP
        {
            SES_Exp *exp = analyse_Exp(tree->children[1], ev);
            tag->tp = exp->tp;
        }
        else if (tree->children[0]->type == ST_ID) // ID LP RP
        {
            SES_ID *val = analyse_ID(tree->children[0], ev);
            if (val->sym == NULL)
            {
                error_func_nodef(tree->first_line, val->name);
                tag->tp = new_type_never();
            }
            else if (!type_can_call(val->sym->tp))
            {
                error_call(tree->first_line);
                tag->tp = new_type_never();
            }
            else if (val->sym->state != SS_DEF)
            {
                error_func_nodef(tree->first_line, val->name);
                tag->tp = new_type_never();
            }
            else if (val->sym->tp->argc != 0)
            {
                error_call_type(tree->first_line);
                tag->tp = val->sym->tp->ret;
            }
            else
            {
                tag->tp = val->sym->tp->ret;
            }
        }
        else if (tree->children[1]->type == ST_DOT) // Exp DOT ID
        {
            SES_Exp *exp = analyse_Exp(tree->children[0], ev);
            SES_ID *id = analyse_ID(tree->children[2], ev);
            if (!type_can_member(exp->tp))
            {
                error_member(tree->first_line);
                tag->tp = new_type_never();
            }
            else
            {
                symbol *member = type_can_membername(exp->tp, id->name);
                if (member == NULL)
                {
                    error_member_nodef(tree->children[2]->first_line, id->name);
                    tag->tp = new_type_never();
                }
                else
                {
                    tag->tp = member->tp;
                }
            }
        }
        else if (tree->children[1]->type == ST_AND || tree->children[1]->type == ST_OR) // Exp AND Exp, Exp OR Exp
        {
            tag = new (SES_Exp);
            SES_Exp *exp1 = analyse_Exp(tree->children[0], ev);
            SES_Exp *exp2 = analyse_Exp(tree->children[2], ev);
            if (!type_can_logic(exp1->tp))
            {
                error_op_type(tree->children[0]->first_line);
                tag->tp = new_type_meta(MT_INT);
            }
            else if (!type_can_logic(exp2->tp))
            {
                error_op_type(tree->children[2]->first_line);
                tag->tp = new_type_meta(MT_INT);
            }
            else
            {
                tag->tp = exp1->tp;
            }
        }
        else if (tree->children[1]->type == ST_ASSIGNOP)
        {
            tag = new (SES_Exp);
            SES_Exp *exp1 = analyse_Exp(tree->children[0], ev);
            SES_Exp *exp2 = analyse_Exp(tree->children[2], ev);

            bool isrval = true;
            if (tree->children[0]->count == 1 && tree->children[0]->children[0]->type == ST_ID)
                isrval = false;
            else if (tree->children[0]->count == 4 && tree->children[0]->children[1]->type == ST_LB)
                isrval = false;
            else if (tree->children[0]->count == 3 && tree->children[0]->children[1]->type == ST_DOT)
                isrval = false;
            if (isrval)
            {
                error_assign_rval(tree->first_line);
                tag->tp = new_type_never();
            }
            else if (!type_full_eq(exp1->tp, exp2->tp, false))
            {
                error_assign_type(tree->first_line);
                tag->tp = new_type_never();
            }
            else
            {
                tag->tp = exp1->tp;
            }
        }
        else // PLUS, MINUS, ...
        {
            tag = new (SES_Exp);
            SES_Exp *exp1 = analyse_Exp(tree->children[0], ev);
            SES_Exp *exp2 = analyse_Exp(tree->children[2], ev);
            if (!type_can_arithmetic(exp1->tp))
            {
                error_op_type(tree->children[0]->first_line);
                tag->tp = new_type_meta(MT_INT);
            }
            else if (!type_can_arithmetic(exp2->tp))
            {
                error_op_type(tree->children[2]->first_line);
                tag->tp = new_type_meta(MT_INT);
            }
            else if (!type_can_arithmetic2(exp1->tp, exp2->tp))
            {
                error_op_type(tree->children[2]->first_line);
                tag->tp = new_type_meta(MT_INT);
            }
            else
            {
                tag->tp = exp1->tp;
            }
        }
    }
    else if (tree->count == 4)
    {
        if (tree->children[0]->type == ST_ID) // ID LP Args RP
        {
            SES_ID *val = analyse_ID(tree->children[0], ev);
            SES_Exp *args = analyse_Args(tree->children[2], ev);
            tag = new (SES_Exp);
            if (val->sym == NULL)
            {
                error_func_nodef(tree->first_line, val->name);
                tag->tp = new_type_never();
            }
            else if (!type_can_call(val->sym->tp))
            {
                error_call(tree->first_line);
                tag->tp = new_type_never();
            }
            else if (val->sym->state != SS_DEF)
            {
                error_func_nodef(tree->first_line, val->name);
                tag->tp = new_type_never();
            }
            else
            {
                int i = 0;
                bool haspass = true;
                while (args != NULL && i < val->sym->tp->argc)
                {
                    if (!type_full_eq(args->tp, val->sym->tp->args[i], false))
                    {
                        error_call_type(args->lineno);
                        haspass = false;
                    }
                    args = args->next;
                    i++;
                }
                if (args != NULL)
                {
                    error_call_type(tree->first_line);
                    haspass = false;
                }
                if (!haspass)
                {
                    tag->tp = val->sym->tp->ret;
                }
                else
                {
                    tag->tp = val->sym->tp->ret;
                }
            }
        }
        else // Exp LB Exp RB
        {
            tag = new (SES_Exp);
            SES_Exp *exp1 = analyse_Exp(tree->children[0], ev);
            SES_Exp *exp2 = analyse_Exp(tree->children[2], ev);
            if (exp2->tp->cls != TC_META || exp2->tp->metatype != MT_INT)
            {
                error_index_arg(tree->children[2]->first_line);
                tag->tp = type_array_descending(exp1->tp);
            }
            else if (!type_can_index(exp1->tp))
            {
                error_index(tree->first_line);
                tag->tp = new_type_any();
            }
            else
            {
                tag->tp = type_array_descending(exp1->tp);
            }
        }
    }
    tag->lineno = tree->first_line;
    tree->tag = tag;
    return tag;
}
static SES_Exp *analyse_Args(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Args");
    // Args : Exp COMMA Args
    //     | Exp
    //     ;
    assert(tree->type == ST_Args);

    SES_Exp *first = analyse_Exp(tree->children[0], ev);
    if (tree->count > 1)
    {
        first->next = analyse_Args(tree->children[2], ev);
    }
    tree->tag = first;
    return first;
}

static void analyse(ast *tree)
{
    env *ev = new (env);
    ev->syms = new_symbol_table(NULL);
    analyse_Program(tree, ev);
}

static bool enable_semantics_log = false;
static bool enable_semantics_error = true;
static bool semantics_is_passed = false;
static char semantics_buffer[1024];

void semantics_error(int type, int lineno, char *format, ...)
{
    semantics_is_passed = 0;

    if (!enable_semantics_error)
        return;

    fprintf(stderr, "Error type %d at Line %d: ", type, lineno);

    va_list aptr;
    int ret;

    va_start(aptr, format);
    vsprintf(semantics_buffer, format, aptr);
    va_end(aptr);

    fprintf(stderr, "%s.\n", semantics_buffer);
}

void semantics_log(int lineno, char *format, ...)
{
    if (!enable_semantics_log)
        return;

    fprintf(stdout, "semantics log at Line %d: ", lineno);

    va_list aptr;
    int ret;

    va_start(aptr, format);
    vsprintf(semantics_buffer, format, aptr);
    va_end(aptr);

    fprintf(stdout, "%s\n", semantics_buffer);
}

void semantics_set_log(bool enable)
{
    enable_semantics_log = enable;
}

void semantics_set_error(bool enable)
{
    enable_semantics_error = enable;
}

void semantics_prepare()
{
    semantics_is_passed = 1;
}

bool semantics_work(ast *tree)
{
    analyse(tree);
    return semantics_has_passed();
}

bool semantics_has_passed()
{
    return semantics_is_passed;
}