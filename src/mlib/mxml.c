#include "mlib/mxml.h"
#include "mlib/utf8.h"

void MXml_Init(MXml* parser, MStrView xml) {
    parser->xml = xml;
    parser->cur = xml.str;
    parser->lineStart = xml.str;
    parser->line = 1;
    parser->error = MXmlError_NONE;
    parser->state = MXmlState_TEXT;
}

static u32 MXml_Peek(MXml* p) {
    size_t len = 0;
    return UTF8_Decode((u8*)p->cur, &len);
}

static u32 MXml_Advance(MXml* p) {
    u32 cp = UTF8_Next(&p->cur);
    if (cp == '\n') {
        p->line++;
        p->lineStart = p->cur;
    }
    return cp;
}

static void MXml_SkipWS(MXml* p) {
    while (MCharIsAnyWhitespace((u8)(MXml_Peek(p)))) {
        MXml_Advance(p);
    }
}

MXmlToken MXml_NextToken(MXml* p) {
    MXmlToken token = {0};

    if (p->state == MXmlState_IN_TAG) {
        MXml_SkipWS(p);
        if (*p->cur == '>') {
            MXml_Advance(p);
            p->state = MXmlState_TEXT;
            token.type = MXmlTokenType_TAG_END;
            return token;
        }
        if (*p->cur == '/' && p->cur[1] == '>') {
            MXml_Advance(p); MXml_Advance(p);
            p->state = MXmlState_TEXT;
            token.type = MXmlTokenType_TAG_SELF_CLOSE;
            return token;
        }
        if (*p->cur == '\0') {
            token.type = MXmlTokenType_EOF;
            return token;
        }

        // Attribute name
        const char* start = p->cur;
        while (*p->cur && !MCharIsAnyWhitespace((u8)*p->cur) &&
                *p->cur != '=' && *p->cur != '>' && *p->cur != '/') {
            MXml_Advance(p);
        }
        token.type = MXmlTokenType_ATTR_NAME;
        token.name = MStrViewMakeP(start, (u32)(p->cur - start));

        MXml_SkipWS(p);
        if (*p->cur == '=') {
            MXml_Advance(p);
            MXml_SkipWS(p);
            char quote = 0;
            if (*p->cur == '"' || *p->cur == '\'') {
                quote = *p->cur;
                MXml_Advance(p);
            }
            start = p->cur;
            if (quote) {
                while (*p->cur && *p->cur != quote) {
                    MXml_Advance(p);
                }
                token.value = MStrViewMakeP(start, (u32)(p->cur - start));
                if (*p->cur == quote) MXml_Advance(p);
            } else {
                while (*p->cur && !MCharIsAnyWhitespace((u8)*p->cur)
                        && *p->cur != '>' && *p->cur != '/') {
                    MXml_Advance(p);
                }
                token.value = MStrViewMakeP(start, (u32)(p->cur - start));
            }
        }
        return token;
    }

    char* textStart = p->cur;
    MXml_SkipWS(p);

    if (*p->cur == '\0') {
        token.type = MXmlTokenType_EOF;
        return token;
    }

    if (*p->cur == '<') {
        MXml_Advance(p); // skip '<'
        if (MXml_Peek(p) == '?') {
            MXml_Advance(p);
            token.type = MXmlTokenType_DECLARATION;
            const char* start = p->cur;
            while (*p->cur && !(*p->cur == '?' && p->cur[1] == '>')) {
                MXml_Advance(p);
            }
            token.value = MStrViewMakeP(start, (u32)(p->cur - start));
            if (*p->cur) {
                MXml_Advance(p); // skip '?'
                MXml_Advance(p); // skip '>'
            }
            return token;
        }

        if (MXml_Peek(p) == '!') {
            MXml_Advance(p);
            if (p->cur[0] == '-' && p->cur[1] == '-') {
                MXml_Advance(p); MXml_Advance(p);
                token.type = MXmlTokenType_COMMENT;
                const char* start = p->cur;
                while (*p->cur && !(p->cur[0] == '-' && p->cur[1] == '-' && p->cur[2] == '>')) {
                    MXml_Advance(p);
                }
                token.value = MStrViewMakeP(start, (u32)(p->cur - start));
                if (*p->cur) {
                    MXml_Advance(p); MXml_Advance(p); MXml_Advance(p);
                }
                return token;
            }
        }

        if (MXml_Peek(p) == '/') {
            MXml_Advance(p);
            token.type = MXmlTokenType_TAG_CLOSE;
            const char* start = p->cur;
            while (*p->cur && *p->cur != '>') {
                MXml_Advance(p);
            }
            token.name = MStrViewMakeP(start, (u32)(p->cur - start));
            if (*p->cur == '>') MXml_Advance(p);
            return token;
        }

        token.type = MXmlTokenType_TAG_START;
        const char* start = p->cur;
        while (*p->cur && !MCharIsAnyWhitespace((u8)*p->cur) &&
                *p->cur != '>' && *p->cur != '/') {
            MXml_Advance(p);
        }
        token.name = MStrViewMakeP(start, (u32)(p->cur - start));
        p->state = MXmlState_IN_TAG;
        return token;
    }

    token.type = MXmlTokenType_TEXT;
    while (*p->cur && *p->cur != '<') {
        MXml_Advance(p);
    }
    token.value = MStrViewMakeP(textStart, (u32)(p->cur - textStart));
    return token;
}

