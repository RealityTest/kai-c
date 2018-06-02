typedef enum ExprMode {
    ExprMode_Invalid,
    ExprMode_Unresolved,
    ExprMode_Computed,
    ExprMode_Assignable,
    ExprMode_Addressable,
    ExprMode_Nil,
    ExprMode_File,
    ExprMode_Library,
    ExprMode_Type
} ExprMode;

typedef struct ExprInfo ExprInfo;
struct ExprInfo {
    Type *desiredType;
    Scope *scope;
    ExprMode mode;
    Val val;
};

#define DeclCase(kind, node) case StmtDeclKind_##kind: { \
    Decl_##kind *decl = &node->kind;
    
#define ExprCase(kind, node) case ExprKind_##kind: { \
    Expr_##kind *expr = &node->kind;


#define CaseEnd() } break;

void invalidateSymbol(Symbol *symbol) {
    if (symbol) {
        symbol->state = SymbolState_Resolved;
        symbol->kind = SymbolKind_Invalid;
    }
}

Inline
void resolveSymbol(Symbol *symbol, Type *type) {
    symbol->state = SymbolState_Resolved;
    symbol->type = type;
}

b32 declareSymbol(Package *pkg, Scope *scope, const char *name, Symbol **symbol, Decl *decl) {
    Symbol *old = Lookup(scope, name);
    if (old) {
        ReportError(pkg, RedefinitionError, decl->start, "Duplication definition of symbol %s", name);
        ReportNote(pkg, old->decl->start, "Previous definition of %s", name);
        *symbol = old;
        return true;
    }

    Symbol *sym = ArenaAlloc(&pkg->arena, sizeof(Symbol));
    sym->kind = SymbolKind_Invalid;
    sym->state = SymbolState_Resolving;
    sym->decl = decl;

    *symbol = sym;
    
    return false;
}

Type *lowerMeta(Package *pkg, Type *type, Position pos) {
    if (type->kind != TypeKind_Metatype) {
        ReportError(pkg, InvalidMetatypeError, pos, "%s cannot be used as a type", DescribeTypeKind(type->kind));
        return NULL;
    }

    return type->Metatype.instanceType;
}

Type *baseType(Type *type) {
repeat:
    if (type->kind == TypeKind_Alias) {
        type = type->Alias.symbol->type;
        goto repeat;
    }

    return type;
}

b32 isInteger(Type *type) {
    type = baseType(type);
    if (type->kind == TypeKind_Int || type->kind == TypeKind_UntypedInt) {
        return true;
    }

    return false;
}

b32 isFloat(Type *type) {
    type = baseType(type);
    if (type->kind == TypeKind_Float || type->kind == TypeKind_UntypedFloat) {
        return true;
    }

    return false;
}

b32 convert(Type *type, Type *target) {
    if (type == UntypedIntType) {
        return target == UntypedIntType || target->kind == TypeKind_Int;
    }

    if (type == UntypedFloatType) {
        return target == UntypedFloatType || target->kind == TypeKind_Float;
    }

    // FIXME: Brett, this doesn't work for aliased types
    return type == target;
}

Scope *pushScope(Package *pkg, Scope *parent) {
    Scope *scope = ArenaCalloc(&pkg->arena, sizeof(Scope));
    scope->parent = parent;
    return scope;
}

Scope *popScope(Package *pkg, Scope *scope) {
    ASSERT(scope->parent);
    return scope->parent;
}

Symbol *Lookup(Scope *scope, const char *name) {
    do {
        Symbol *symbol = MapGet(&scope->members, name);
        if (symbol) {
            return symbol;
        }

        scope = scope->parent;
    } while (scope);

    return NULL;
}

Type *checkExpr(Package *pkg, Expr *expr, ExprInfo *exprInfo) {
    switch (expr->kind) {
    case ExprKind_Ident: {
        Expr_Ident ident = expr->Ident; 
        Symbol *symbol = Lookup(exprInfo->scope, ident.name);
        if (!symbol) {
            ReportError(pkg, UndefinedIdentError, expr->start, "Use of undefined identifier '%s'", ident.name);
            exprInfo->mode = ExprMode_Invalid;
            return InvalidType;
        }

        symbol->used = true;
        if (symbol->state != SymbolState_Resolved) {
            exprInfo->mode = ExprMode_Unresolved;
            return InvalidType;
        }

        CheckerInfo *solve = &pkg->checkerInfo[expr->id];
        solve->kind = CheckerInfoKind_Ident;
        solve->Ident.symbol = symbol;

        switch (symbol->kind) {
        case SymbolKind_Type: {
            exprInfo->mode = ExprMode_Type;
        } break;

        case SymbolKind_Constant: {
            if (symbol == TrueSymbol || symbol == FalseSymbol) {
                exprInfo->mode = ExprMode_Computed;
                break;
            }
        }
        default:
            exprInfo->mode = ExprMode_Addressable;
        }

        exprInfo->val = symbol->val;
        return symbol->type;
    } break;

    case ExprKind_LitInt: {
        Expr_LitInt lit = expr->LitInt;

        Type *type = InvalidType;

        if (exprInfo->desiredType) {
            if (isInteger(exprInfo->desiredType)) {
                exprInfo->val.u64 = lit.val;
            }

            else if (isFloat(exprInfo->desiredType)) {
                exprInfo->val.f64 = (f64)lit.val;
            }

            else {
                ReportError(
                    pkg, InvalidConversionError, expr->start,
                    "Unable to convert type %s to expected type type %s",
                    DescribeType(UntypedIntType), DescribeType(exprInfo->desiredType)
                );

                return InvalidType;
            }

            type = exprInfo->desiredType;
        } else {
            type = UntypedIntType;
        }
        
        exprInfo->mode = ExprMode_Computed;
        CheckerInfo *info = &pkg->checkerInfo[expr->id];
        info->BasicLit.type = type;
        return type;
    };

    case ExprKind_LitFloat: {
        Expr_LitFloat lit = expr->LitFloat;

        Type *type = InvalidType;

        if (exprInfo->desiredType) {
            if (isInteger(exprInfo->desiredType)) {
                exprInfo->val.u64 = (u64)lit.val;
            }

            else if (isFloat(exprInfo->desiredType)) {
                exprInfo->val.f64 = lit.val;
            }

            else {
                ReportError(
                    pkg, InvalidConversionError, expr->start,
                    "Unable to convert type %s to expected type type %s",
                    DescribeType(UntypedFloatType), DescribeType(exprInfo->desiredType)
                );

                return InvalidType;
            }

            type = exprInfo->desiredType;
        } else {
            type = UntypedFloatType;
        }

        exprInfo->mode = ExprMode_Computed;
        CheckerInfo *info = &pkg->checkerInfo[expr->id];
        info->BasicLit.type = type;
        return type;
    } break;
    }

    return InvalidType;
}

Type *checkFuncType(Expr *funcExpr, ExprInfo *exprInfo) {
    Expr_TypeFunction func = funcExpr->TypeFunction;

    return NULL;
}

b32 checkConstDecl(Package *pkg, Scope *scope, b32 isGlobal, Decl *declStmt) {
    Decl_Constant decl = declStmt->Constant;

    ASSERT(scope);

    if (ArrayLen(decl.names) != 1) {
        ReportError(pkg, MultipleConstantDeclError, decl.start, "Constant declarations must declare at most one item");

        if (ArrayLen(decl.names) > 0) {
            For (decl.names) {
                const char *name = decl.names[i]->name;
                Symbol *symbol = MapGet(&pkg->symbolMap, name);
                invalidateSymbol(symbol);
            }
        }

        return false;
    }

    if (ArrayLen(decl.values) > 1) {
        ReportError(pkg, ArityMismatchError, decl.start, "Constant declarations only allow for a single value, but got %zu", ArrayLen(decl.values));
        return false;
    }

    Type *expectedType = NULL;

    if (decl.type) {
        ExprInfo info = {.scope = scope};
        expectedType = lowerMeta(pkg, checkExpr(pkg, decl.type, &info), decl.type->start);
        if (info.mode == ExprMode_Unresolved) {
            return true;
        }
    }

    Expr_Ident *name = decl.names[0];
    Expr *value = decl.values[0];
    Symbol *symbol = MapGet(&pkg->symbolMap, name->name);
    symbol->state = SymbolState_Resolving;

    switch (value->kind) {
    case ExprKind_LitFunction: {
        Expr_LitFunction func = value->LitFunction;
        ExprInfo info = {.scope = scope};
        Type *type = checkFuncType(func.type, &info);
        if (info.mode == ExprMode_Unresolved) {
            return true;
        }
        resolveSymbol(symbol, type);
    } break;

    case ExprKind_TypeStruct:
    case ExprKind_TypeUnion:
    case ExprKind_TypeEnum: {
        UNIMPLEMENTED();
    } break;
    }

    ExprInfo info = {.scope = scope, .desiredType = expectedType};
    Type *type = checkExpr(pkg, value, &info);
    if (info.mode == ExprMode_Unresolved) {
        return true;
    }

    if (expectedType) {
        if (!convert(type, expectedType)) {
            ReportError(
                pkg, InvalidConversionError, value->start,
                "Unable to convert type %s to expected type type %s",
                DescribeType(type), DescribeType(expectedType)
            );
            invalidateSymbol(symbol);
        }
    }

    // TODO: check for tuple
    
    symbol->type = type;
    symbol->state = SymbolState_Resolved;

    CheckerInfo *solve = &pkg->checkerInfo[declStmt->id];
    solve->kind = CheckerInfoKind_Decl;
    solve->Decl.symbol = symbol;
    solve->Decl.isGlobal = isGlobal;

    return false;
}

b32 checkVarDecl(Package *pkg, Scope *scope, b32 isGlobal, Decl *declStmt) {
    Decl_Variable var = declStmt->Variable;

    Type *expectedType = NULL;

    if (var.type) {
        ExprInfo info = {.scope = scope};
        expectedType = lowerMeta(pkg, checkExpr(pkg, var.type, &info), var.type->start);
        if (info.mode == ExprMode_Unresolved) {
            return true;
        }
    }

    DynamicArray(Symbol *) symbols = NULL;
    ArrayFit(symbols, ArrayLen(var.names));

    if (isGlobal) {
        For (var.names) {
            Symbol *symbol = MapGet(&pkg->symbolMap, var.names[i]->name);
            ArrayPush(symbols, symbol);
        }
    } else {
        For (var.names) {
            Symbol *symbol;
            // FIXME(Brett): figure out how I want to recover from a duplicate
            declareSymbol(pkg, scope, var.names[i]->name, &symbol, declStmt);
            ArrayPush(symbols, symbol);
        }
    }

    // NOTE: decl like `x, y: i32`
    if (ArrayLen(var.values) == 0) {
        ASSERT(expectedType);
        For (symbols) {
            symbols[i]->type = expectedType;
            symbols[i]->state = SymbolState_Resolved;
        }

        if (expectedType->kind == TypeKind_Array && expectedType->Array.length == -1) {
            ReportError(pkg, UninitImplicitArrayError, var.type->start, "Implicit-length array must have an initial value");
        }

        if (expectedType->kind == TypeKind_Function) {
            ReportError(pkg, UninitFunctionTypeError, var.type->start, "Variables of a function type must be initialized");
            ReportNote(pkg, var.type->start, "If you want an uninitialized function pointer use *%s instead", DescribeType(expectedType));
        }
    }
    
    else {
        if (ArrayLen(var.values) != ArrayLen(var.names)) {
            ReportError(pkg, ArityMismatchError, var.start, "The amount of identifiers (%zu) doesn't match the amount of values (%zu)", ArrayLen(var.names), ArrayLen(var.values));

            For (symbols) {
                invalidateSymbol(symbols[i]);
            }
            return false;
        }

        // TODO(Brett): check for multi-value call
        ExprInfo info = {.scope = scope, .desiredType = expectedType};
        For (var.names) {
            Type *type = checkExpr(pkg, var.values[i], &info);
            if (info.mode == ExprMode_Unresolved) {
                return true;
            }

            if (expectedType && !convert(type, expectedType)) {
                ReportError(
                    pkg, InvalidConversionError, var.values[i]->start,
                    "Unable to convert type %s to expected type type %s",
                    DescribeType(type), DescribeType(expectedType)
                );
                invalidateSymbol(symbols[i]);
            }

            if (type->kind == TypeKind_Metatype) {
                ReportError(pkg, MetatypeNotAnExprError, var.values[i]->start, "Metatype is not a valid expression");
                invalidateSymbol(symbols[i]);
                continue;
            }

            symbols[i]->type = expectedType ? expectedType : type;
            symbols[i]->kind = SymbolKind_Variable;
            symbols[i]->state = SymbolState_Resolved;
        }
    }

    CheckerInfo *solve = &pkg->checkerInfo[declStmt->id];
    solve->DeclList.symbols = symbols;
    solve->DeclList.isGlobal = isGlobal;
    solve->kind = CheckerInfoKind_DeclList;

    return false;
}

b32 checkImportDecl(Package *pkg, Decl *declStmt) {
    Decl_Import import = declStmt->Import;
    UNIMPLEMENTED();
    return false;
}

b32 check(Package *pkg, Stmt *stmt) {
    b32 shouldRequeue;

    Scope *scope = pkg->globalScope;

    switch (stmt->kind) {
    case StmtDeclKind_Constant: {
        shouldRequeue = checkConstDecl(pkg, scope, true, (Decl *)stmt);
    } break;

    case StmtDeclKind_Variable: {
        shouldRequeue = checkVarDecl(pkg, scope, true, (Decl *)stmt);
    } break;

    case StmtDeclKind_Import: {
        shouldRequeue = checkImportDecl(pkg, (Decl *)stmt);
    } break;

    default:
        ASSERT_MSG_VA(false, "Statement of type '%s' went unchecked", AstDescriptions[stmt->kind]);
    }

    return shouldRequeue;
}

#if TEST
void test_checkerTest() {

}
#endif
