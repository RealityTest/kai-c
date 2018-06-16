// NOTE: element #1 is the error code and element #2 is a short description of the error case.
// The description isn't used by the compiler itself but is there for tools to utilize
// in the future.
#define ERROR_CODES \
    FOR_EACH(TODO, "No individual error code exists for this error currently") \
    FOR_EACH(InvalidEscape, "Escape sequence is an invalid Unicode codepoint") \
    FOR_EACH(InvalidCharacterEscape, "Escape sequence is a invalid") \
    FOR_EACH(InvalidNumericEscape, "Escape sequence is an invalid Unicode codepoint") \
    FOR_EACH(InvalidCodePoint, "An invalid Unicode codepoint") \
    FOR_EACH(StringContainsNewline, "A string literal contains a newline") \
    FOR_EACH(UnexpectedEOF, "An unexpected EOF while parsing a string literal") \
    FOR_EACH(ExpectedDigit, "Expected a digit") \
    FOR_EACH(DigitOutOfRange, "Digit is out of range of base") \
    FOR_EACH(FloatOverflow, "Float literal overflow") \
    FOR_EACH(IntOverflow, "Integer literal overflow") \
    FOR_EACH(WrongDoubleQuote, "User entered `“` (0x201c) as a quote instead of ASCII") \
    FOR_EACH(Syntax, "Syntax error") \
    FOR_EACH(Fatal, "Fatal error") \
    FOR_EACH(Redefinition, "Redefinition") \
    FOR_EACH(MultipleConstantDecl, "Defined more than one constant item at a time") \
    FOR_EACH(ArityMismatch, "The amount of declarations doesn't match the amount of values") \
    FOR_EACH(NotAType, "A type was expected but something else was given") \
    FOR_EACH(UndefinedIdent, "Use of an undefined identifier") \
    FOR_EACH(InvalidConversion, "Unable to convert type to target type") \
    FOR_EACH(UninitImplicitArray, "Implicit-length array was provided without an initial value") \
    FOR_EACH(UninitFunctionType, "A function type wasn't provided a body") \
    FOR_EACH(TypeNotAnExpression, "A type is not a valid expression in the provided context") \
    FOR_EACH(NotNilable, "The type cannot be converted to nil") \
    FOR_EACH(InvalidPointeeType, "The pointee type is not a valid type") \
    FOR_EACH(ParamNameMissing, "The parameter doesn't have a name") \
    FOR_EACH(AddressOfNonAddressable, "The expr could not have it's address taken") \
    FOR_EACH(InvalidUnaryOperation, "Unary operation invalid for type") \
    FOR_EACH(InvalidBinaryOperation, "Binary operation invalid for type") \
    FOR_EACH(DivisionByZero, "Divided by zero") \
    FOR_EACH(TypeMismatch, "Type did not match") \
    FOR_EACH(BadCondition, "Expected a numeric or pointer type to act as a condition") \
    FOR_EACH(UnrepresentableValue, "The value could not be represented in the desired type without loss of information") \
    FOR_EACH(CastArgumentCount, "Cast did not receive exactly 1 expression") \
    FOR_EACH(AutocastExpectsDesiredType, "Context did not provide an expected type for autocast")

typedef enum ErrorCode {
#define FOR_EACH(e, s) e##Error,
    ERROR_CODES
#undef FOR_EACH
} ErrorCode;

typedef struct DiagnosticNote DiagnosticNote;
struct DiagnosticNote {
    const char *msg;
    DiagnosticNote *next;
};

typedef struct DiagnosticError DiagnosticError;
struct DiagnosticError {
    const char *msg;
    DiagnosticNote *note;
};

b32 shouldPrintErrorCode() {
#if NO_ERROR_CODES
    return false;
#endif
    return FlagErrorCodes;
}

#define HasErrors(p) (p)->diagnostics.errors

void ReportError(Package *p, ErrorCode code, Position pos, const char *msg, ...) {
    va_list args;
    char msgBuffer[512]; // TODO: Static & Thread Local?
    va_start(args, msg);
    vsnprintf(msgBuffer, sizeof(msgBuffer), msg, args);
    char errorBuffer[512];

    int errlen = shouldPrintErrorCode() ?
        snprintf(errorBuffer, sizeof(errorBuffer), "ERROR(%s:%u:%u, E%04d): %s\n", pos.name, pos.line, pos.column, code, msgBuffer) :
        snprintf(errorBuffer, sizeof(errorBuffer), "ERROR(%s:%u:%u): %s\n",        pos.name, pos.line, pos.column,       msgBuffer);

    // NOTE: snprintf returns how long the string would have been instead of its truncated length
    // We're clamping it here to prevent an overrun.
    errlen = MIN(errlen, sizeof(errorBuffer));

    char *errorMsg = ArenaAlloc(&p->diagnostics.arena, errlen + 1);
    memcpy(errorMsg, errorBuffer, errlen + 1);
    va_end(args);

    DiagnosticError error = { .msg = errorMsg, .note = NULL };
    ArrayPush(p->diagnostics.errors, error);
}

void ReportNote(Package *p, Position pos, const char *msg, ...) {
    ASSERT(p->diagnostics.errors);
    va_list args;
    char msgBuffer[512]; // TODO: Static & Thread Local?
    va_start(args, msg);
    vsnprintf(msgBuffer, sizeof(msgBuffer), msg, args);
    char noteBuffer[512];

    int notelen = snprintf(noteBuffer, sizeof(noteBuffer), "NOTE(%s:%u:%u): %s\n", pos.name, pos.line, pos.column, msgBuffer);

    // NOTE: snprintf returns how long the string would have been instead of its truncated length
    // We're clamping it here to prevent an overrun.
    notelen = MIN(notelen, sizeof(noteBuffer));

    char *noteMsg = ArenaAlloc(&p->diagnostics.arena, notelen + 1);
    noteMsg = memcpy(noteMsg, noteBuffer, notelen + 1);
    va_end(args);

    DiagnosticNote *note = ArenaAlloc(&p->diagnostics.arena, sizeof(DiagnosticNote));
    note->msg = noteMsg;
    note->next = NULL;

    DiagnosticNote **indirect = &p->diagnostics.errors[ArrayLen(p->diagnostics.errors) - 1].note;
    while ((*indirect) != NULL)
        indirect = &(*indirect)->next;

    *indirect = note;
}

#if TEST
char outputErrorBuffer[8096];
#define outputDiagnostic(fmt, __VA_ARGS__) snprintf(outputErrorBuffer, sizeof(outputErrorBuffer), fmt, __VA_ARGS__)
#else
#define outputDiagnostic(fmt, __VA_ARGS__) fprintf(stderr, fmt, __VA_ARGS__)
#endif

void OutputReportedErrors(Package *p) {
    For (p->diagnostics.errors) {
        outputDiagnostic("%s", p->diagnostics.errors[i].msg);
        for (DiagnosticNote *note = p->diagnostics.errors[i].note; note; note = note->next) {
            outputDiagnostic("%s", note->msg);
        }
    }
    ArrayFree(p->diagnostics.errors);
    ArenaFree(&p->diagnostics.arena);
}
#undef OutputReportedErrorsPrinter

#if TEST
void test_errorReporting() {

    Position builtinPosition = {0};
    Package mainPackage = {0};
    ReportError(&mainPackage, SyntaxError, builtinPosition, "Error Reporting value of five %d", 5);
    ReportNote(&mainPackage, builtinPosition, "Note Reporting value of six %d", 6);
    ASSERT(mainPackage.diagnostics.errors != NULL);
    OutputReportedErrors(&mainPackage);
    ASSERT(mainPackage.diagnostics.errors == NULL);
    ASSERT(mainPackage.diagnostics.arena.blocks == NULL);
}
#endif
