
typedef u8 CheckerInfoKind;
enum Enum_CheckerInfoKind {
    // None is the zero value and so the default for zero initialized checker info.
    CheckerInfoKindNone,
    CheckerInfoKindConstant,
    CheckerInfoKindVariable,
    CheckerInfoKindForeign,
    CheckerInfoKindIdent,
    CheckerInfoKindSelector,
    CheckerInfoKindBasicExpr,
    CheckerInfoKindLabel,
    CheckerInfoKindGoto,
    CheckerInfoKindFor,
    CheckerInfoKindSwitch,
    CheckerinfoKindCase,
    NUM_CHECKER_INFO_KINDS,
};

STATIC_ASSERT(STMT_KIND_END <= UINT8_MAX, "enum values overflow storage type");

typedef u8 Conversion;
#define ConversionKindMask 0x0F // Lower 3 bits denote the class
#define ConversionKindNone    0
#define ConversionKindSame    1
#define ConversionKindFtoI    2
#define ConversionKindItoF    3
#define ConversionKindPtoI    4
#define ConversionKindItoP    5
#define ConversionKindBool    6
#define ConversionKindTuple   7 // Information on the conversion can be found on their receiver.
#define ConversionKindAny    15

#define ConversionFlag_Extend 0x10 // 0001
#define ConversionFlag_Signed 0x20 // 0010
#define ConversionFlag_Float  0x40 // 0100 (Source type is a Float)

typedef struct CheckerInfo_Constant CheckerInfo_Constant;
struct CheckerInfo_Constant {
    Symbol *symbol;
};

typedef struct CheckerInfo_Variable CheckerInfo_Variable;
struct CheckerInfo_Variable {
    Symbol **symbols;
    Conversion *conversions;
};

typedef struct CheckerInfo_Foreign CheckerInfo_Foreign;
struct CheckerInfo_Foreign {
    Symbol *symbol;
};

typedef struct CheckerInfo_Ident CheckerInfo_Ident;
struct CheckerInfo_Ident {
    Conversion coerce;
    Symbol *symbol;
};

typedef u8 SelectorKind;
#define SelectorKindNone   0x0
#define SelectorKindStruct 0x1
#define SelectorKindImport 0x2

typedef struct Selector_Struct Selector_Struct;
struct Selector_Struct {
    u32 index;  // The member index in the structure
    u32 offset; // The member offset in the structure (in bits)
};

typedef struct Selector_Import Selector_Import;
struct Selector_Import {
    Symbol *symbol;
    Package *package;
};

typedef union SelectorValue SelectorValue;
union SelectorValue {
    Selector_Struct Struct;
    Selector_Import Import;
};

typedef struct CheckerInfo_Selector CheckerInfo_Selector;
struct CheckerInfo_Selector {
    Conversion coerce;
    Type *type;
    b8 isConstant;
    Val val;
    
    SelectorKind kind;
    SelectorValue value;
};

typedef struct CheckerInfo_BasicExpr CheckerInfo_BasicExpr;
struct CheckerInfo_BasicExpr {
    Conversion coerce;
    Type *type;
    b8 isConstant;
    Val val;
};

typedef struct CheckerInfo_Label CheckerInfo_Label;
struct CheckerInfo_Label {
    Symbol *symbol;
};

typedef struct CheckerInfo_Goto CheckerInfo_Goto;
struct CheckerInfo_Goto {
    // NOTE: When the statement provides an expression this can be NULL. In this case the backend should generate the
    //  expression value and branch to the address returned
    Symbol *target;
};

typedef struct CheckerInfo_For CheckerInfo_For;
struct CheckerInfo_For {
    Symbol *continueTarget;
    Symbol *breakTarget;
};

typedef struct CheckerInfo_Switch CheckerInfo_Switch;
struct CheckerInfo_Switch {
    Symbol *breakTarget;
};

typedef struct CheckerInfo_Case CheckerInfo_Case;
struct CheckerInfo_Case {
    Symbol *fallthroughTarget;
};

STATIC_ASSERT(offsetof(CheckerInfo_Ident,     coerce) == 0, "conversion must be at offset 0 for expressions");
STATIC_ASSERT(offsetof(CheckerInfo_Selector,  coerce) == 0, "conversion must be at offset 0 for expressions");
STATIC_ASSERT(offsetof(CheckerInfo_BasicExpr, coerce) == 0, "conversion must be at offset 0 for expressions");

typedef struct CheckerInfo CheckerInfo;
struct CheckerInfo {
    CheckerInfoKind kind;
    union {
        Conversion coerce; // Present when CheckerInfo is for an expression
        CheckerInfo_Constant Constant;
        CheckerInfo_Variable Variable;
        CheckerInfo_Foreign Foreign;
        CheckerInfo_Selector Selector;
        CheckerInfo_Ident Ident;
        CheckerInfo_BasicExpr BasicExpr;
        CheckerInfo_Label Label;
        CheckerInfo_Goto Goto;
        CheckerInfo_For For;
        CheckerInfo_Switch Switch;
        CheckerInfo_Case Case;
    };
};

#ifdef __cplusplus
extern "C" {
#endif
Symbol *Lookup(Scope *scope, const char *name);
Type *TypeFromCheckerInfo(CheckerInfo info);
b32 IsInteger(Type *type);
b32 IsSigned(Type *type);
b32 IsFloat(Type *type);
#ifdef __cplusplus
}
#endif

