#pragma once

#include "mlib/mlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MXmlTokenType {
    MXmlTokenType_EOF,
    MXmlTokenType_TAG_START,      // <tag
    MXmlTokenType_TAG_END,        // >
    MXmlTokenType_TAG_CLOSE,      // </tag>
    MXmlTokenType_TAG_SELF_CLOSE, // />
    MXmlTokenType_ATTR_NAME,
    MXmlTokenType_ATTR_VALUE,
    MXmlTokenType_TEXT,
    MXmlTokenType_COMMENT,
    MXmlTokenType_DECLARATION,    // <?xml ... ?>
    MXmlTokenType_ERROR
} MXmlTokenType;

typedef enum MXmlError {
    MXmlError_NONE,
    MXmlError_UNEXPECTED_EOF,
    MXmlError_SYNTAX_ERROR,
    MXmlError_UNCLOSED_TAG,
    MXmlError_UNCLOSED_QUOTE,
} MXmlError;

typedef struct MXmlToken {
    MXmlTokenType type;
    MStrView name;  // Tag or attribute name (only set on open or close tag currently)
    MStrView value; // Attribute value or text content
} MXmlToken;

typedef enum MXmlState {
    MXmlState_TEXT,
    MXmlState_IN_TAG,
} MXmlState;

typedef struct MXml {
    MStrView xml;
    const char* cur;
    const char* lineStart;
    int line;
    MXmlError error;
    MXmlState state;
} MXml;

void MXml_Init(MXml* parser, MStrView xml);
MXmlToken MXml_NextToken(MXml* parser);

#ifdef __cplusplus
}
#endif
