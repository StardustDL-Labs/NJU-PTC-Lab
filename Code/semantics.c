#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include "semantics.h"
#include "ast.h"
#include "symbol.h"
#include "common.h"

void semantics_error(int type, int lineno, char *format, ...);
void semantics_log(int lineno, char *format, ...);

typedef struct
{
    symbol_table *syms;
    union {
        type *declare_type;
    };
} env;

typedef struct
{
    type *tp;
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
    symbol *sym;
} SES_Specifier;

typedef struct
{
    type *tp;
    symbol *sym;
    env *ev;
} SES_FunDec;

typedef struct
{
    char *name;
} SES_Tag;

typedef struct
{
    type *tp;
    char *name;
} SES_StructSpecifier;

typedef struct __SES_Exp
{
    type *tp;
    struct __SES_Exp *next;
} SES_Exp;

typedef struct __SES_VarDec
{
    symbol *sym;
    struct __SES_VarDec *next;
} SES_VarDec;

typedef struct __SES_Args
{
    type *tp;
    struct __SES_Args *next;
} SES_Args;

#pragma region
static void analyse_EMPTY(ast *tree, env *ev);
static void analyse_SEMI(ast *tree, env *ev);
static void analyse_COMMA(ast *tree, env *ev);
static void analyse_ASSIGNOP(ast *tree, env *ev);
static void analyse_RELOP(ast *tree, env *ev);
static void analyse_PLUS(ast *tree, env *ev);
static void analyse_MINUS(ast *tree, env *ev);
static void analyse_STAR(ast *tree, env *ev);
static void analyse_DIV(ast *tree, env *ev);
static void analyse_AND(ast *tree, env *ev);
static void analyse_OR(ast *tree, env *ev);
static void analyse_NOT(ast *tree, env *ev);
static void analyse_DOT(ast *tree, env *ev);
static void analyse_LP(ast *tree, env *ev);
static void analyse_RP(ast *tree, env *ev);
static void analyse_LB(ast *tree, env *ev);
static void analyse_RB(ast *tree, env *ev);
static void analyse_LC(ast *tree, env *ev);
static void analyse_RC(ast *tree, env *ev);
static void analyse_STRUCT(ast *tree, env *ev);
static void analyse_RETURN(ast *tree, env *ev);
static void analyse_IF(ast *tree, env *ev);
static void analyse_ELSE(ast *tree, env *ev);
static void analyse_WHILE(ast *tree, env *ev);
static SES_INT *analyse_INT(ast *tree, env *ev);
static SES_FLOAT *analyse_FLOAT(ast *tree, env *ev);
static SES_ID *analyse_ID(ast *tree, env *ev);
static SES_TYPE *analyse_TYPE(ast *tree, env *ev);
static void analyse_Program(ast *tree, env *ev);
static void analyse_ExtDefList(ast *tree, env *ev);
static void analyse_ExtDef(ast *tree, env *ev);
static SES_VarDec *analyse_ExtDecList(ast *tree, env *ev);
static SES_Specifier *analyse_Specifier(ast *tree, env *ev);
static SES_StructSpecifier *analyse_StructSpecifier(ast *tree, env *ev);
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

static void analyse_EMPTY(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "EMPTY");
    assert(tree->type == ST_EMPTY);
}
static void analyse_SEMI(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "SEMI");
    assert(tree->type == ST_SEMI);
}
static void analyse_COMMA(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "COMMA");
    assert(tree->type == ST_COMMA);
}
static void analyse_ASSIGNOP(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "ASSIGNOP");
    assert(tree->type == ST_ASSIGNOP);
}
static void analyse_RELOP(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "RELOP");
    assert(tree->type == ST_RELOP);
}
static void analyse_PLUS(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "PLUS");
    assert(tree->type == ST_PLUS);
}
static void analyse_MINUS(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "MINUS");
    assert(tree->type == ST_MINUS);
}
static void analyse_STAR(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "STAR");
    assert(tree->type == ST_STAR);
}
static void analyse_DIV(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "DIV");
    assert(tree->type == ST_DIV);
}
static void analyse_AND(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "AND");
    assert(tree->type == ST_AND);
}
static void analyse_OR(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "OR");
    assert(tree->type == ST_OR);
}
static void analyse_NOT(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "NOT");
    assert(tree->type == ST_NOT);
}
static void analyse_DOT(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "DOT");
    assert(tree->type == ST_DOT);
}

static void analyse_LP(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "LP");
    assert(tree->type == ST_LP);
}
static void analyse_RP(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "RP");
    assert(tree->type == ST_RP);
}
static void analyse_LB(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "LB");
    assert(tree->type == ST_LB);
}
static void analyse_RB(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "RB");
    assert(tree->type == ST_RB);
}
static void analyse_LC(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "LC");
    assert(tree->type == ST_LC);
}
static void analyse_RC(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "RC");
    assert(tree->type == ST_RC);
}
static void analyse_STRUCT(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "STRUCT");
    assert(tree->type == ST_STRUCT);
}
static void analyse_RETURN(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "RETURN");
    assert(tree->type == ST_RETURN);
}
static void analyse_IF(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "IF");
    assert(tree->type == ST_IF);
}
static void analyse_ELSE(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "ELSE");
    assert(tree->type == ST_ELSE);
}
static void analyse_WHILE(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "WHILE");
    assert(tree->type == ST_WHILE);
}
#pragma endregion

static SES_INT *analyse_INT(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "INT");
    assert(tree->type == ST_INT);
    SES_INT *tag = new (SES_INT);
    tag->tp = new_type_meta(MT_INT);
    tree->tag = tag;
    return tree->tag;
}
static SES_FLOAT *analyse_FLOAT(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "FLOAT");
    assert(tree->type == ST_FLOAT);
    SES_FLOAT *tag = new (SES_FLOAT);
    tag->tp = new_type_meta(MT_FLOAT);
    tree->tag = tag;
    return tree->tag;
}
static SES_ID *analyse_ID(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "ID");
    assert(tree->type == ST_ID);
    symbol *sym = st_find(ev->syms, tree->t_str);
    SES_ID *tag = new (SES_ID);
    tag->name = tree->t_str;
    tag->sym = sym;
    tree->tag = tag;
    return tree->tag;
}
static SES_TYPE *analyse_TYPE(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "TYPE");
    assert(tree->type == ST_TYPE);
    SES_TYPE *tag = new (SES_TYPE);
    tag->tp = new_type_meta(tree->t_type);
    tree->tag = tag;
    return tree->tag;
}
static void analyse_Program(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "Program");
    // Program : ExtDefList
    //     ;

    assert(tree->type == ST_Program);
    analyse_ExtDefList(tree->children[0], ev);
}
static void analyse_ExtDefList(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "ExtDefList");
    // ExtDefList : ExtDef ExtDefList
    //     | /* empty */
    //     ;

    assert(tree->type == ST_ExtDefList);

    for (int i = 0; i < tree->count; i++)
        analyse_ExtDef(tree->children[i], ev);
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

    ast *specifier = tree->children[0];

    analyse_Specifier(specifier, ev);

    if (tree->children[1]->type == ST_ExtDecList)
    {
        ev->declare_type = ((SES_Specifier *)specifier->tag)->tp;
        analyse_ExtDecList(tree->children[1], ev);
        ev->declare_type = NULL;
    }
    else if (tree->children[1]->type == ST_FunDec)
    {
        ev->declare_type = ((SES_Specifier *)specifier->tag)->tp;
        ast *fundec = tree->children[1];
        analyse_FunDec(fundec, ev);
        SES_FunDec *sf = (SES_FunDec *)fundec->tag;
        if (tree->children[2]->type == ST_CompSt) // function definition
        {
            env *funcev = sf->ev;
            analyse_CompSt(tree->children[2], funcev);
            sf->sym->state = SS_DEF;
            st_pushfront(ev->syms, sf->sym);
        }
        else if (tree->children[2]->type == ST_SEMI) // function declare
        {
            sf->sym->state = SS_DEC;
            st_pushfront(ev->syms, sf->sym);
        }
        else
            assert(0);
    }
    else if (tree->children[1]->type == ST_SEMI)
    {
    }
    else
        assert(0);
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
    else
    {
        assert(0);
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
    if (child->type == ST_TYPE)
    {
        analyse_TYPE(child, ev);
        SES_Specifier *tag = new (SES_Specifier);
        tag->tp = ((SES_TYPE *)child->tag)->tp;
        tree->tag = tag;
    }
    else if (child->type == ST_StructSpecifier)
    {
        analyse_StructSpecifier(child, ev);
        SES_Specifier *tag = new (SES_Specifier);
        tag->tp = ((SES_StructSpecifier *)child->tag)->tp;
        tree->tag = tag;
    }
    else
    {
        assert(0);
    }
    return tree->tag;
}
static SES_StructSpecifier *analyse_StructSpecifier(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "StructSpecifier");
    // StructSpecifier : STRUCT OptTag LC DefList RC
    //     | STRUCT Tag
    //     ;
    assert(tree->type == ST_StructSpecifier);
    if (tree->count == 2)
    {
        analyse_Tag(tree->children[1], ev);
        SES_Tag *ctag = (SES_Tag *)tree->children[1]->tag;

        SES_StructSpecifier *tag = new (SES_StructSpecifier);
        //TODO
    }
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
}
static SES_FunDec *analyse_FunDec(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "FunDec");
    // FunDec : ID LP VarList RP
    //     | ID LP RP
    //     ;
    assert(tree->type == ST_FunDec);
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
    else
    {
        assert(0);
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
}
static void analyse_CompSt(ast *tree, env *ev)
{
    semantics_log(tree->first_line, "%s", "CompSt");
    // CompSt : LC DefList StmtList RC
    //     ;
    assert(tree->type == ST_CompSt);

    symbol_table *cst = new_symbol_table(ev->syms);
    env *cenv = new (env);
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

    SES_VarDec *tag = analyse_VarDec(tree->children[0], ev);
    if (tree->count > 1)
    {
    }
    else
    {
        //TODO check exp type assign
    }
    tree->tag = tag;
    return tag;
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
