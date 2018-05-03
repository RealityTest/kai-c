enum TokenKind {
    TK_Invalid,
    TK_Eof,
    TK_Comment,

    TK_Ident,
    TK_Directive,

    TK_Int,
    TK_Float,
    TK_String,

    TK_Add,
    TK_Sub,
    TK_Mul,
    TK_Div,
    TK_Rem,

    TK_And,
    TK_Or,
    TK_Xor,
    TK_Shl,
    TK_Shr,

    TK_AssignAdd,
    TK_AssignSub,
    TK_AssignMul,
    TK_AssignDiv,
    TK_AssignRem,

    TK_AssignAnd,
    TK_AssignOr,
    TK_AssignXor,
    TK_AssignShl,
    TK_AssignShr,

    TK_Land,
    TK_Lor,
    TK_Lss,
    TK_Gtr,
    TK_Not,

    TK_Eql,
    TK_Neq,
    TK_Leq,
    TK_Geq,

    TK_Assign,
    TK_Ellipsis,
    TK_Dollar,
    TK_Question,
    TK_RetArrow,
    TK_Lparen,
    TK_Lbrack,
    TK_Lbrace,
    TK_Rparen,
    TK_Rbrack,
    TK_Rbrace,

    TK_Comma,
    TK_Period,
    TK_Colon,
    TK_Semicolon,

    TK_Cast,
    TK_Bitcast,
    TK_Autocast,

    TK_Using,

    TK_Goto,
    TK_Break,
    TK_Continue,
    TK_Fallthrough,

    TK_Return,

    TK_If,
    TK_For,
    TK_Else,
    TK_Defer,
    TK_In,
    TK_Switch,
    TK_Case,

    TK_Fn,
    TK_Union,
    TK_Variant,
    TK_Enum,
    TK_Struct,

    TK_Nil
};

struct Position {
    String filename;
    u32 offset,
        line,
        column;
};

struct Token {
    TokenKind kind;
    String lit;
    Position pos;
};

Token InvalidToken = {
    TK_Invalid,
    STR("<invalid>")
};

struct Lexer {
    String path;
    String data;

    u32 lineCount;
    u32 offset;
    u32 column;

    u32 currentCp;

    u8 currentCpWidth;

    b8 insertSemi;
    b8 insertSemiBeforeLBrace;
};

void NextCodePoint(Lexer *l);

b32 MakeLexer(Lexer *l, String path) {
    String data;
    b32 ok = ReadFile(&data, path);
    if (!ok) {
        return false;
    }

    l->path = path;
    l->data = data;
    l->offset = 0;
    l->lineCount = 1;
    l->insertSemi = false;
    l->insertSemiBeforeLBrace = false;

    NextCodePoint(l);

    return true;
}

void NextCodePoint(Lexer *l) {
    if (l->offset < l->data.len) {
        String curr = Slice(l->data, l->offset, l->data.len - l->offset);
        u32 cp, cpWidth;
        cp = DecodeCodePoint(&cpWidth, curr);

        if (cp == '\n') {
            l->lineCount += (l->insertSemi) ? 1 : 0;
            l->column = 0;
        }

        l->currentCp = cp;
        l->currentCpWidth = cpWidth;
        l->offset += cpWidth;
        l->column += 1;
    } else {
        l->offset = l->data.len - 1;
        if (l->data[l->offset] == '\n') {
            l->lineCount += 1;
            l->column = 0;
        }

        l->currentCp = FileEnd;
        l->currentCpWidth = 1;
    }
}

void skipWhitespace(Lexer *l) {
    while (true) {
        switch (l->currentCp) {
        case '\n': {
            if (l->insertSemi)
                return;
            NextCodePoint(l);
        } break;

        case ' ':
        case '\t':
        case '\r': {
            NextCodePoint(l);
        } break;

        default:
            return;
        }
    }
}

u32 digitValue(u32 cp) {
    if (cp >= '0' && cp <= '9')
        return cp - '0';
    else if (cp >= 'A' && cp <= 'F')
        return cp - 'A' + 10;
    else if (cp >= 'a' && cp <= 'f')
        return cp - 'a' + 10;
    else
        return 16;
}

void scanMatissa(Lexer *l, u32 base) {
    while (l->currentCp != FileEnd) {
        if (l->currentCp != '_' && digitValue(l->currentCp) >= base)
            break;

        NextCodePoint(l);
    }
}

Token scanNumber(Lexer *l, b32 seenDecimal) {
    u32 start = l->offset;

    Token token;
    token.kind = TK_Int;
    token.lit = Slice(l->data, start-l->currentCpWidth, start);
    token.pos.filename = l->path;
    token.pos.line = l->lineCount;
    token.pos.offset = l->offset;
    token.pos.column = l->column;

    b32 mustBeInteger = false;

    if (seenDecimal) {
        token.kind = TK_Float;
        scanMatissa(l, 10);
    }

    if (l->currentCp == '0' && !seenDecimal) {
        NextCodePoint(l);

        switch (l->currentCp) {
        case 'x': {
            NextCodePoint(l);
            scanMatissa(l, 16);
            mustBeInteger = true;
        } break;

        case 'b': {
            NextCodePoint(l);
            scanMatissa(l, 2);
            mustBeInteger = true;
        } break;

        default:
            scanMatissa(l, 10);
        }
    }

    if (!seenDecimal && !mustBeInteger) {
        scanMatissa(l, 10);
    }

    if (l->currentCp == '.' && !seenDecimal && !mustBeInteger) {
        NextCodePoint(l);
        token.kind = TK_Float;
        scanMatissa(l, 10);
    }

    // TODO(Brett): exponent

    token.lit.len = l->offset - start;
    return token;
}

TokenKind GetTokenKind(String ident) {
    return TK_Ident;
}

Token NextToken(Lexer *l) {
    skipWhitespace(l);

    u8 *start = &l->data[l->offset - l->currentCpWidth];
    String lit = MakeString(start, l->currentCpWidth);

    Token token = InvalidToken;
    token.pos.filename = l->path;
    token.pos.line = l->lineCount;
    token.pos.offset = l->offset;
    token.pos.column = l->column;

    l->insertSemi = false;

    u32 cp = l->currentCp;
    if (IsAlpha(cp)) {
        u32 offset = l->offset;
        while (IsAlpha(cp) || IsNumeric(cp)) {
            NextCodePoint(l);
            cp = l->currentCp;
        }

        lit.len = l->offset = offset;
        token.kind = GetTokenKind(lit);

        switch (token.kind) {
        case TK_Ident:
        case TK_Break:
        case TK_Continue:
        case TK_Fallthrough:
        case TK_Return:
        case TK_Nil: {
            l->insertSemi = true;
        } break;

        case TK_If:
        case TK_For:
        case TK_Switch: {
            l->insertSemiBeforeLBrace = true;
        } break;
        }
    } else if (IsNumeric(cp)) {
        l->insertSemi = true;
        token = scanNumber(l, false);
    } else {
        NextCodePoint(l);

        switch (cp) {
        case FileEnd: {
            if (l->insertSemi) {
                l->insertSemi = false;
                token.kind = TK_Semicolon;
                token.lit = STR("\n");
            } else {
                token.kind = TK_Eof;
                lit.len = 0;
            }
        } break;



        }
    }

    token.lit = lit;
    return token;
}
