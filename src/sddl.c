// Copyright 2014 SimpleThings, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "sddl.h"
#include "red_string.h"
#include "red_json.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct SDDLParseResult_t
{
    bool ok;
    SDDLDocument doc;
    RedStringList errors;
    RedStringList warnings;
};

struct SDDLDocument_t
{
    unsigned refcnt;
    unsigned num_authors;
    char **authors;
    char *description;
    unsigned num_vars;
    SDDLVarDecl *vars;
};

struct SDDLVarDecl_t
{
    char *name;
    char *description;
    void *extra;
    SDDLDatatypeEnum datatype;
    double *maxValue;
    double *minValue;
    SDDLNumericDisplayHintEnum numeric_display_hint;
    char *regex;
    char *units;
    unsigned struct_num_members;
    SDDLVarDecl *struct_members;
    unsigned array_num_elements;
    SDDLVarDecl array_element;
    RedJsonObject json;
};

static SDDLNumericDisplayHintEnum _display_hint_from_string(const char *sz)
{
    if (!strcmp(sz, "normal"))
    {
        return SDDL_NUMERIC_DISPLAY_HINT_NORMAL;
    }
    else if (!strcmp(sz, "percentage"))
    {
        return SDDL_NUMERIC_DISPLAY_HINT_PERCENTAGE;
    }
    else if (!strcmp(sz, "hex"))
    {
        return SDDL_NUMERIC_DISPLAY_HINT_HEX;
    }
    else if (!strcmp(sz, "scientific"))
    {
        return SDDL_NUMERIC_DISPLAY_HINT_SCIENTIFIC;
    }
    return SDDL_NUMERIC_DISPLAY_HINT_INVALID;
}

static SDDLDatatypeEnum _datatype_from_string(const char *sz)
{
    if (!strcmp(sz, "void"))
    {
        return SDDL_DATATYPE_VOID;
    }
    else if (!strcmp(sz, "string"))
    {
        return SDDL_DATATYPE_STRING;
    }
    else if (!strcmp(sz, "bool"))
    {
        return SDDL_DATATYPE_BOOL;
    }
    else if (!strcmp(sz, "int8"))
    {
        return SDDL_DATATYPE_INT8;
    }
    else if (!strcmp(sz, "uint8"))
    {
        return SDDL_DATATYPE_UINT8;
    }
    else if (!strcmp(sz, "int16"))
    {
        return SDDL_DATATYPE_INT16;
    }
    else if (!strcmp(sz, "uint16"))
    {
        return SDDL_DATATYPE_UINT16;
    }
    else if (!strcmp(sz, "int32"))
    {
        return SDDL_DATATYPE_INT32;
    }
    else if (!strcmp(sz, "uint32"))
    {
        return SDDL_DATATYPE_UINT32;
    }
    else if (!strcmp(sz, "float32"))
    {
        return SDDL_DATATYPE_FLOAT32;
    }
    else if (!strcmp(sz, "float64"))
    {
        return SDDL_DATATYPE_FLOAT64;
    }
    else if (!strcmp(sz, "datetime"))
    {
        return SDDL_DATATYPE_DATETIME;
    }
    return SDDL_DATATYPE_INVALID;
}

static SDDLVarDecl _sddl_parse_var(RedString decl, RedJsonObject def)
{
    SDDLVarDecl out;
    unsigned numKeys;
    unsigned i;
    char **keysArray;

    out = calloc(1, sizeof(struct SDDLVarDecl_t));
    if (!out)
    {
        return NULL;
    }

    RedStringList split = RedString_Split(decl, ' ');
    RedString name = RedStringList_GetString(split, 1);
    out->name = RedString_strdup(RedString_GetChars(name));
    RedStringList_Free(split);

    out->extra = NULL;
    out->description = RedString_strdup("");
    out->datatype = SDDL_DATATYPE_FLOAT32;
    out->numeric_display_hint = SDDL_NUMERIC_DISPLAY_HINT_NORMAL;
    out->units = RedString_strdup("");

    numKeys = RedJsonObject_NumItems(def);
    keysArray = RedJsonObject_NewKeysArray(def);

    for (i = 0; i < numKeys; i++)
    {
        RedJsonValue val = RedJsonObject_Get(def, keysArray[i]);
        RedString key = RedString_New(keysArray[i]);

        if (RedString_Equals(key, "datatype"))
        {
            char *datatypeString;
            if (!RedJsonValue_IsString(val))
            {
                printf("datatype must be string\n");
                return NULL;
            }
            datatypeString = RedJsonValue_GetString(val);
            out->datatype = _datatype_from_string(datatypeString);
            if (out->datatype == SDDL_DATATYPE_INVALID)
            {
                printf("invalid datatype %s\n", datatypeString);
                return NULL;
            }
        }
        else if (RedString_Equals(key, "description"))
        {
            char *description;
            if (!RedJsonValue_IsString(val))
            {
                printf("description must be string\n");
                return NULL;
            }
            description = RedJsonValue_GetString(val);
            free(out->description);
            out->description = RedString_strdup(description);
            if (!out->description)
            {
                printf("OOM duplicating description string\n");
                return NULL;
            }
        }
        else if (RedString_Equals(key, "max-value"))
        {
            if (RedJsonValue_IsNull(val))
            {
                out->maxValue = NULL;
            }
            else
            {
                double * pMaxValue;
                if (!RedJsonValue_IsNumber(val))
                {
                    printf("max-value must be number or null\n");
                    return NULL;
                }
                pMaxValue = malloc(sizeof(double));
                if (!pMaxValue)
                {
                    printf("OOM allocating max-value\n");
                    return NULL;
                }
                *pMaxValue = RedJsonValue_GetNumber(val);
                out->maxValue = pMaxValue;
            }
        }
        else if (RedString_Equals(key, "min-value"))
        {
            if (RedJsonValue_IsNull(val))
            {
                out->minValue = NULL;
            }
            else
            {
                double * pMinValue;
                if (!RedJsonValue_IsNumber(val))
                {
                    printf("min-value must be number or null\n");
                    return NULL;
                }
                pMinValue = malloc(sizeof(double));
                if (!pMinValue)
                {
                    printf("OOM allocating min-value\n");
                    return NULL;
                }
                *pMinValue = RedJsonValue_GetNumber(val);
                out->minValue = pMinValue;
            }
        }
        else if (RedString_Equals(key, "numeric-display-hint"))
        {
            char *displayHintString;
            if (!RedJsonValue_IsString(val))
            {
                printf("datatype must be string\n");
                return NULL;
            }
            displayHintString = RedJsonValue_GetString(val);
            out->numeric_display_hint = _display_hint_from_string(displayHintString);
            if (out->numeric_display_hint == SDDL_NUMERIC_DISPLAY_HINT_INVALID)
            {
                printf("invalid numeric-display-hint %s", displayHintString);
                return NULL;
            }
        }
        else if (RedString_Equals(key, "regex"))
        {
            char *regex;
            if (!RedJsonValue_IsString(val))
            {
                printf("regex must be string\n");
                return NULL;
            }
            regex = RedJsonValue_GetString(val);
            out->regex = RedString_strdup(regex);
            if (!out->regex)
            {
                printf("OOM duplicating regex string\n");
                return NULL;
            }
        }
        else if (RedString_Equals(key, "units"))
        {
            char *units;
            if (!RedJsonValue_IsString(val))
            {
                printf("units must be string\n");
                return NULL;
            }
            units = RedJsonValue_GetString(val);
            free(out->units);
            out->units = RedString_strdup(units);
            if (!out->units)
            {
                printf("OOM duplicating units string\n");
                return NULL;
            }
        }
        else
        {
            printf("Unexpected field: %s\n", RedString_GetChars(key));
        }
        RedString_Free(key);
    }

    return out;
}

#if 0
static SDDLVarDecl _sddl_parse_class(RedString decl, RedJsonObject def)
{
    SDDLVarDecl cls;
    unsigned numKeys;
    unsigned i;
    char **keysArray;

    cls = calloc(1, sizeof(struct SDDLVarDecl_t));
    if (!cls)
    {
        return NULL;
    }

    RedStringList split = RedString_Split(decl, ' ');
    RedString name = RedStringList_GetString(split, 1);
    cls->name = RedString_strdup(RedString_GetChars(name));
    RedStringList_Free(split);

    cls->extra = NULL;
    cls->description = RedString_strdup("");
    cls->json = def;
    /* TODO: set defaults */

    numKeys = RedJsonObject_NumItems(def);
    keysArray = RedJsonObject_NewKeysArray(def);
    
    for (i = 0; i < numKeys; i++)
    {
        RedJsonValue val = RedJsonObject_Get(def, keysArray[i]);
        RedString key = RedString_New(keysArray[i]);
        if (RedString_BeginsWith(key, "control "))
        {
            if (!RedJsonValue_IsObject(val))
            {
                printf("Expected object for control definition\n");
                return NULL;
            }
            RedJsonObject obj = RedJsonValue_GetObject(val);
            SDDLVarDecl control = _sddl_parse_control(key, obj);
            if (!control)
            {
                return NULL;
            }
            cls->struct_num_members++;
            cls->struct_members = realloc(
                    cls->struct_members, 
                    cls->struct_num_members*
                    sizeof(SDDLVarDecl));
            if (!cls->struct_members)
            {
                printf("OOM expanding cls->struct_members\n");
                return NULL;
            }
            cls->struct_members[cls->struct_num_members - 1] = control;
        }
        else if (RedString_Equals(key, "authors"))
        {
            if (!RedJsonValue_IsArray(val))
            {
                printf("Expected list for authors\n");
                return NULL;
            }
        }
        else if (RedString_Equals(key, "description"))
        {
            char *description;
            if (!RedJsonValue_IsString(val))
            {
                printf("description must be string\n");
                return NULL;
            }
            description = RedJsonValue_GetString(val);
            if (cls->description)
                free(cls->description);
            cls->description = RedString_strdup(description);
            if (!cls->description)
            {
                printf("OOM duplicating description string\n");
                return NULL;
            }
        }

        RedString_Free(key);
    }

    return cls;
}
#endif

SDDLDocument sddl_ref_document(SDDLDocument doc)
{
    doc->refcnt++;
    return doc;
}

void sddl_unref_document(SDDLDocument doc)
{
    if (doc->refcnt >= 1)
    {
        doc->refcnt--;
    }
    if (doc->refcnt == 0)
    {
        /* TODO: free document correctly */
        free(doc);
    }
}

static SDDLParseResult _new_parse_result()
{
    SDDLParseResult pr;
    pr = calloc(1, sizeof(struct SDDLParseResult_t));
    if (!pr) 
        goto fail;
    pr->ok = false;

    pr->errors = RedStringList_New();
    if (!pr->errors)
        goto fail;

    pr->warnings = RedStringList_New();
    if (!pr->warnings)
        goto fail;

    pr->doc = calloc(1, sizeof(struct SDDLDocument_t));
    if (!pr->doc)
        goto fail;
    pr->doc->refcnt = 1;

    return pr;
fail:
    if (pr) 
    {
        RedStringList_Free(pr->errors);
        RedStringList_Free(pr->warnings);
        free(pr->doc);
        free(pr);
    }
    return NULL;
}


SDDLParseResult sddl_load_and_parse(const char *filename)
{
    FILE *fp;
    SDDLParseResult out;
    fp = fopen(filename, "r");
    if (!fp)
    {
        return NULL;
    }
    out = sddl_load_and_parse_file(fp);
    fclose(fp);
    return out;
}

SDDLParseResult sddl_load_and_parse_file(FILE *file)
{
    /* Read entire file into memory */
    long filesize;
    char *buffer;
    SDDLParseResult out;
    fseek(file, 0, SEEK_END);
    filesize = ftell(file); 
    fseek(file, 0, SEEK_SET);
    buffer = calloc(1, filesize+1);
    fread(buffer, 1, filesize, file);
    out = sddl_parse(buffer);
    free(buffer);
    return out;
}

typedef struct
{
    SDDLDatatypeEnum datatype;
    SDDLOptionalityEnum optionality;
    SDDLDirectionEnum direction;
    char *name;
} VarKeyInfo;

typedef enum
{
    _KEY_TOKEN_TYPE_INVALID,
    _KEY_TOKEN_TYPE_DATATYPE,
    _KEY_TOKEN_TYPE_DIRECTION,
    _KEY_TOKEN_TYPE_OPTIONALITY,
    _KEY_TOKEN_TYPE_VAR_NAME,
} _KeyTokenType;

typedef struct
{
    _KeyTokenType type;
    union
    {
        SDDLDatatypeEnum datatype;
        SDDLOptionalityEnum optionality;
        SDDLDirectionEnum direction;
        char *name;
    };
} _KeyToken;

_KeyToken _KeyTokenFromString(const char *s)
{
    _KeyToken out;
    out.type = _KEY_TOKEN_TYPE_INVALID;

    if (!strcmp(s, "void"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_VOID;
    }
    else if (!strcmp(s, "string"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_STRING;
    }
    else if (!strcmp(s, "bool"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_BOOL;
    }
    else if (!strcmp(s, "int8"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_INT8;
    }
    else if (!strcmp(s, "uint8"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_UINT8;
    }
    else if (!strcmp(s, "int16"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_INT16;
    }
    else if (!strcmp(s, "uint16"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_UINT16;
    }
    else if (!strcmp(s, "int32"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_INT32;
    }
    else if (!strcmp(s, "uint32"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_UINT32;
    }
    else if (!strcmp(s, "float32"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_FLOAT32;
    }
    else if (!strcmp(s, "float64"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_FLOAT64;
    }
    else if (!strcmp(s, "datetime"))
    {
        out.type = _KEY_TOKEN_TYPE_DATATYPE;
        out.datatype = SDDL_DATATYPE_STRING;
    }

    else if (!strcmp(s, "inout"))
    {
        out.type = _KEY_TOKEN_TYPE_DIRECTION;
        out.direction = SDDL_DIRECTION_INOUT;
    }
    else if (!strcmp(s, "in"))
    {
        out.type = _KEY_TOKEN_TYPE_DIRECTION;
        out.direction = SDDL_DIRECTION_IN;
    }
    else if (!strcmp(s, "out"))
    {
        out.type = _KEY_TOKEN_TYPE_DIRECTION;
        out.direction = SDDL_DIRECTION_OUT;
    }

    else if (!strcmp(s, "optional"))
    {
        out.type = _KEY_TOKEN_TYPE_OPTIONALITY;
        out.optionality = SDDL_OPTIONALITY_OPTIONAL;
    }
    else if (!strcmp(s, "required"))
    {
        out.type = _KEY_TOKEN_TYPE_OPTIONALITY;
        out.optionality = SDDL_OPTIONALITY_REQUIRED;
    }

    return out;
}

// True if the key declares a Cloud Variable, false otherwise
bool _parse_var_key(SDDLParseResult result, const char *key, VarKeyInfo *out)
{
    RedStringList parts;
    unsigned i;

    parts = RedString_SplitChars(key, ' ');
    if (RedStringList_NumStrings(parts) < 2)
    {
        return false;
    }

    out->datatype = SDDL_DATATYPE_INVALID;
    out->optionality = SDDL_OPTIONALITY_INVALID;
    out->direction = SDDL_DIRECTION_INVALID;
    out->name = NULL;

    for (i = 0; i < RedStringList_NumStrings(parts); i++)
    {
        const char *part;
        _KeyToken token;
        part = RedStringList_GetStringChars(parts, i);

        token = _KeyTokenFromString(part);
        switch (token.type)
        {
            case _KEY_TOKEN_TYPE_DATATYPE:
            {
                if (out->datatype != SDDL_DATATYPE_INVALID)
                {
                    RedStringList_AppendChars(result->errors, "Datatype already specified");
                    return false;
                }

                out->datatype = token.datatype;
                break;
            }
            case _KEY_TOKEN_TYPE_DIRECTION:
            {
                if (out->direction != SDDL_DIRECTION_INVALID)
                {
                    RedStringList_AppendChars(result->errors, "Direction already specified");
                    return false;
                }

                out->direction = token.direction;
                break;
            }
            case _KEY_TOKEN_TYPE_OPTIONALITY:
            {
                if (out->optionality != SDDL_OPTIONALITY_INVALID)
                {
                    RedStringList_AppendChars(result->errors, "Optionality already specified");
                    return false;
                }

                out->optionality = token.optionality;
                break;
            }
            default:
            {
                if (out->datatype == SDDL_DATATYPE_INVALID)
                {
                    RedStringList_AppendChars(result->errors, "Datatype or qualifier expected.");
                    return false;
                }
                if (out->name != NULL)
                {
                    RedStringList_AppendChars(result->errors, "Variable name already specified.");
                    return false;
                }

                out->name = RedString_strdup(part);
            }
        }
    }
    return true;
}

SDDLParseResult sddl_parse(const char *sddl)
{
    RedJsonObject jsonObj;
    SDDLDocument doc;
    SDDLParseResult result;
    unsigned numKeys;
    unsigned i;
    char **keysArray;

    result = _new_parse_result();
    if (!result)
    {
        return NULL;
    }

    doc = result->doc;

    doc->description = RedString_strdup("");

    jsonObj = RedJson_Parse(sddl);
    if (!jsonObj)
    {
        RedStringList_AppendChars(result->errors, "JSON parsing failed!");
        return result;
    }

    numKeys = RedJsonObject_NumItems(jsonObj);
    keysArray = RedJsonObject_NewKeysArray(jsonObj);

    for (i = 0; i < numKeys; i++)
    {
        RedJsonValue val = RedJsonObject_Get(jsonObj, keysArray[i]);
        RedString key = RedString_New(keysArray[i]);
        bool ok;
        VarKeyInfo varKeyInfo;

        ok =  _parse_var_key(result, RedString_GetChars(key), &varKeyInfo);
        if (!ok)
        {
            return NULL;
        }

        if (!RedJsonValue_IsObject(val))
        {
            RedStringList_AppendChars(result->errors, "Expected object for variable metadata");
            return NULL;
        }

        SDDLVarDecl var = _sddl_parse_var(key, RedJsonValue_GetObject(val));
        if (!var)
        {
            return NULL;
        }

        doc->num_vars++;
        doc->vars = realloc(
                doc->vars, 
                doc->num_vars*
                sizeof(SDDLVarDecl));
        if (!doc->vars)
        {
            printf("OOM expanding doc->vars\n");
            return NULL;
        }
        doc->vars[doc->num_vars - 1] = var;


        RedString_Free(key);
    }

    result->doc = doc;
    result->ok = true;
    return result;
}

bool sddl_parse_result_ok(SDDLParseResult result)
{
    if (!result)
        return false;
    return result->ok;
}

SDDLDocument sddl_parse_result_document(SDDLParseResult result)
{
    return result->doc;
}

SDDLDocument sddl_parse_result_ref_document(SDDLParseResult result)
{
    return sddl_ref_document(result->doc);
}

unsigned sddl_parse_result_num_errors(SDDLParseResult result)
{
    return RedStringList_NumStrings(result->errors);
}

const char * sddl_parse_result_error(SDDLParseResult result, unsigned index)
{
    return RedStringList_GetStringChars(result->errors, index);
}

unsigned sddl_parse_result_num_warnings(SDDLParseResult result)
{
    return RedStringList_NumStrings(result->warnings);
}

const char * sddl_parse_result_warning(SDDLParseResult result, unsigned index)
{
    return RedStringList_GetStringChars(result->warnings, index);
}

void sddl_free_parse_result(SDDLParseResult result)
{
    if (result) {
        RedStringList_Free(result->errors);
        RedStringList_Free(result->warnings);
        sddl_unref_document(result->doc);
        free(result);
    }
}

const char * sddl_document_description(SDDLDocument doc)
{
    return doc->description;
}

unsigned sddl_document_num_authors(SDDLDocument doc)
{
    return doc->num_authors;
}

const char * sddl_document_author(SDDLDocument doc, unsigned index)
{
    return doc->authors[index];
}

unsigned sddl_document_num_vars(SDDLDocument doc)
{
    return doc->num_vars;
}

SDDLVarDecl sddl_document_var_by_idx(SDDLDocument doc, unsigned index) 
{
    return doc->vars[index];
}

SDDLVarDecl sddl_document_var_by_name(SDDLDocument doc, const char* name) 
{
    unsigned i;
    for (i = 0; i < sddl_document_num_vars(doc); i++)
    {
        if (!strcmp(doc->vars[i]->name,  name))
        {
            return doc->vars[i];
        }
    }
    return NULL;
}

const char * sddl_var_name(SDDLVarDecl var)
{
    return var->name;
}

SDDLDatatypeEnum sddl_var_datatype(SDDLVarDecl var)
{
    return var->datatype;
}

const char * sddl_var_description(SDDLVarDecl cls)
{
    return cls->description;
}

const double * sddl_var_max_value(SDDLVarDecl var)
{
    return var->maxValue;
}

const double * sddl_var_min_value(SDDLVarDecl var)
{
    return var->minValue;
}

SDDLNumericDisplayHintEnum sddl_var_numeric_display_hint(SDDLVarDecl var)
{
    return var->numeric_display_hint;
}

const char * sddl_var_regex(SDDLVarDecl var)
{
    return var->regex;
}

const char * sddl_var_units(SDDLVarDecl var)
{
    return var->units;
}

unsigned sddl_var_struct_num_members(SDDLVarDecl var)
{
    return var->struct_num_members;
}

SDDLVarDecl var_struct_member_by_idx(SDDLVarDecl var, unsigned index) 
{
    return var->struct_members[index];
}

SDDLVarDecl sddl_var_struct_member_by_name(SDDLVarDecl var, const char* name) 
{
    unsigned i;
    for (i = 0; i < sddl_var_struct_num_members(var); i++)
    {
        if (!strcmp(var->struct_members[i]->name, name))
        {
            return var->struct_members[i];
        }
    }
    return NULL;
}

unsigned sddl_var_array_num_elements(SDDLVarDecl var)
{
    assert(var->datatype == SDDL_DATATYPE_ARRAY);
    return var->array_num_elements;
}

SDDLVarDecl sddl_var_array_element(SDDLVarDecl var)
{
    return var->array_element;
}

void sddl_var_set_extra(SDDLVarDecl var, void *extra)
{
    var->extra = extra;
}

void * sddl_var_extra(SDDLVarDecl var)
{
    return var->extra;
}

RedJsonObject sddl_var_json(SDDLVarDecl var)
{
    return var->json;
}

SDDLResultEnum sddl_parse_decl(const char *decl, SDDLDirectionEnum *outDirection, SDDLDatatypeEnum *outDatatype, char **outName)
{
    VarKeyInfo info;
    bool ok;
    SDDLParseResult result = _new_parse_result();
    ok = _parse_var_key(result, decl, &info);
    if (!ok)
    {
        return SDDL_ERROR_PARSING;
    }
    *outDirection = info.direction;
    *outDatatype = info.datatype;
    *outName = RedString_strdup(info.name);
    sddl_free_parse_result(result);
    return SDDL_SUCCESS;
}
