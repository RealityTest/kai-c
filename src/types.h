
extern Type *InvalidType;
extern Type *AnyType;
extern Type *VoidType;

extern Type *BoolType;

extern Type *I8Type;
extern Type *I16Type;
extern Type *I32Type;
extern Type *I64Type;

extern Type *U8Type;
extern Type *U16Type;
extern Type *U32Type;
extern Type *U64Type;

extern Type *F32Type;
extern Type *F64Type;

extern Type *IntType;
extern Type *UintType;
extern Type *IntptrType;
extern Type *UintptrType;
extern Type *RawptrType;

extern Type *UntypedIntType;
extern Type *UntypedFloatType;

extern Symbol *FalseSymbol;
extern Symbol *TrueSymbol;

#define TYPE_KINDS                  \
    FOR_EACH(Invalid, "invalid")    \
    FOR_EACH(Void, "void")          \
    FOR_EACH(Bool, "bool")          \
    FOR_EACH(Int, "int")            \
    FOR_EACH(Float, "float")        \
    FOR_EACH(Pointer, "pointer")    \
    FOR_EACH(Array, "array")        \
    FOR_EACH(Slice, "slice")        \
    FOR_EACH(Any, "any")            \
    FOR_EACH(Struct, "struct")      \
    FOR_EACH(Union, "union")        \
    FOR_EACH(Metatype, "meta")      \
    FOR_EACH(Alias, "alias")        \
    FOR_EACH(Function, "function")  \

typedef enum TypeKind {
#define FOR_EACH(kind, ...) TypeKind_##kind,
    TYPE_KINDS
#undef FOR_EACH
} TypeKind;

#define FOR_EACH(kind, ...) typedef struct Type_##kind Type_##kind;
    TYPE_KINDS
#undef FOR_EACH

typedef u8 TypeFlag;
#define TypeFlag_None 0
#define TypeFlag_Untyped  0x1
#define TypeFlag_NoReturn 0x1
#define TypeFlag_Signed   0x2

struct Type_Pointer {
    TypeFlag Flags;
    Type *pointeeType;
};

struct Type_Array {
    TypeFlag Flags;
    i64 length;
    Type *elementType;
};

struct Type_Slice {
    TypeFlag Flags;
    Type *elementType;
};

struct Type_Struct {
    TypeFlag Flags;
    DynamicArray(Type *) members;
};

struct Type_Union {
    TypeFlag Flags;
    u32 tagWidth;
    u32 dataWidth;
    DynamicArray(Type *) cases;
};

struct Type_Function {
    TypeFlag Flags;
    DynamicArray(Type *) params;
    DynamicArray(Type *) results;
};

struct Type_Metatype {
    TypeFlag Flags;
    Type *instanceType;
};

struct Type_Alias {
    TypeFlag Flags;
    Symbol *symbol;
};

_Static_assert(offsetof(Type_Pointer,  Flags) == 0, "Flags must be at offset 0");
_Static_assert(offsetof(Type_Array,    Flags) == 0, "Flags must be at offset 0");
_Static_assert(offsetof(Type_Slice,    Flags) == 0, "Flags must be at offset 0");
_Static_assert(offsetof(Type_Struct,   Flags) == 0, "Flags must be at offset 0");
_Static_assert(offsetof(Type_Union,    Flags) == 0, "Flags must be at offset 0");
_Static_assert(offsetof(Type_Function, Flags) == 0, "Flags must be at offset 0");
_Static_assert(offsetof(Type_Metatype, Flags) == 0, "Flags must be at offset 0");
_Static_assert(offsetof(Type_Alias,    Flags) == 0, "Flags must be at offset 0");

struct Type {
    TypeKind kind;
    u32 width;
    u32 align;

    union {
        TypeFlag Flags;
        Type_Pointer Pointer;
        Type_Array Array;
        Type_Slice Slice;
        Type_Struct Struct;
        Type_Union Union;
        Type_Function Function;

        Type_Metatype Metatype;
        Type_Alias Alias;
    };
};

#ifdef __cplusplus
extern "C" {
const char *DescribeType(Type *type);
const char *DescribeTypeKind(TypeKind kind);
}
#endif
