/*
 * Copyright 2014 Gregory Prisament
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
    unsigned numAuthors;
    char **authors;
    char *description;
    unsigned numProperties;
    SDDLProperty *properties;
};

struct SDDLProperty_t
{
    char *name;
    SDDLPropertyTypeEnum type;
    char *description;
    void *extra;
};

struct SDDLControl_t
{
    struct SDDLProperty_t base;
    SDDLDatatypeEnum datatype;
    SDDLControlTypeEnum controlType;
    double *maxValue;
    double *minValue;
    SDDLNumericDisplayHintEnum numericDisplayHint;
    char *regex;
    char *units;
};

struct SDDLSensor_t
{
    struct SDDLProperty_t base;
    SDDLDatatypeEnum datatype;
    double *maxValue;
    double *minValue;
    SDDLNumericDisplayHintEnum numericDisplayHint;
    char *regex;
    char *units;
};

struct SDDLClass_t
{
    struct SDDLProperty_t base;
    unsigned numAuthors;
    char **authors;
    unsigned numProperties;
    SDDLProperty *properties;
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

static SDDLControlTypeEnum _control_type_from_string(const char *sz)
{
    if (!strcmp(sz, "parameter"))
    {
        return SDDL_CONTROL_TYPE_PARAMETER;
    }
    else if (!strcmp(sz, "trigger"))
    {
        return SDDL_CONTROL_TYPE_TRIGGER;
    }
    return SDDL_CONTROL_TYPE_INVALID;
}

static SDDLControl _sddl_parse_control(RedString decl, RedJsonObject def)
{
    SDDLControl out;
    unsigned numKeys;
    unsigned i;
    char **keysArray;

    out = calloc(1, sizeof(struct SDDLControl_t));
    if (!out)
    {
        return NULL;
    }

    RedStringList split = RedString_Split(decl, ' ');
    RedString name = RedStringList_GetString(split, 1);
    out->base.name = RedString_strdup(RedString_GetChars(name));
    RedStringList_Free(split);

    out->base.type = SDDL_PROPERTY_TYPE_CONTROL;
    out->base.extra = NULL;
    out->base.description = RedString_strdup("");
    out->controlType = SDDL_CONTROL_TYPE_PARAMETER;
    out->datatype = SDDL_DATATYPE_FLOAT32;
    out->numericDisplayHint = SDDL_NUMERIC_DISPLAY_HINT_NORMAL;
    out->units = RedString_strdup("");

    numKeys = RedJsonObject_NumItems(def);
    keysArray = RedJsonObject_NewKeysArray(def);

    for (i = 0; i < numKeys; i++)
    {
        RedJsonValue val = RedJsonObject_Get(def, keysArray[i]);
        RedString key = RedString_New(keysArray[i]);

        if (RedString_Equals(key, "control-type"))
        {
            char *controlTypeString;
            if (!RedJsonValue_IsString(val))
            {
                printf("control-type must be string\n");
                return NULL;
            }
            controlTypeString = RedJsonValue_GetString(val);
            out->controlType = _control_type_from_string(controlTypeString);
            if (out->controlType == SDDL_CONTROL_TYPE_INVALID)
            {
                printf("invalid control-type %s\n", controlTypeString);
                return NULL;
            }
        }
        else if (RedString_Equals(key, "datatype"))
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
            free(out->base.description);
            out->base.description = RedString_strdup(description);
            if (!out->base.description)
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
            out->numericDisplayHint = _display_hint_from_string(displayHintString);
            if (out->numericDisplayHint == SDDL_NUMERIC_DISPLAY_HINT_INVALID)
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

static SDDLSensor _sddl_parse_sensor(RedString decl, RedJsonObject def)
{
    SDDLSensor out;
    unsigned numKeys;
    unsigned i;
    char **keysArray;

    out = calloc(1, sizeof(struct SDDLSensor_t));
    if (!out)
    {
        return NULL;
    }

    RedStringList split = RedString_Split(decl, ' ');
    RedString name = RedStringList_GetString(split, 1);
    out->base.name = RedString_strdup(RedString_GetChars(name));
    RedStringList_Free(split);

    out->base.type = SDDL_PROPERTY_TYPE_SENSOR;
    out->base.extra = NULL;
    /* TODO: set defaults */

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
            out->base.description = RedString_strdup(description);
            if (!out->base.description)
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
            out->numericDisplayHint = _display_hint_from_string(displayHintString);
            if (out->numericDisplayHint == SDDL_NUMERIC_DISPLAY_HINT_INVALID)
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
            out->units = RedString_strdup(units);
            if (!out->units)
            {
                printf("OOM duplicating units string\n");
                return NULL;
            }
        }
        else
        {
            printf("Unexpected field: %s", RedString_GetChars(key));
        }
        RedString_Free(key);
    }
    
    return out;
}

static SDDLClass _sddl_parse_class(RedString decl, RedJsonObject def)
{
    SDDLClass cls;
    unsigned numKeys;
    unsigned i;
    char **keysArray;

    cls = calloc(1, sizeof(struct SDDLClass_t));
    if (!cls)
    {
        return NULL;
    }

    RedStringList split = RedString_Split(decl, ' ');
    RedString name = RedStringList_GetString(split, 1);
    cls->base.name = RedString_strdup(RedString_GetChars(name));
    RedStringList_Free(split);

    cls->base.type = SDDL_PROPERTY_TYPE_CLASS;
    cls->base.extra = NULL;
    cls->base.description = RedString_strdup("");
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
            SDDLControl control = _sddl_parse_control(key, obj);
            if (!control)
            {
                return NULL;
            }
            cls->numProperties++;
            cls->properties = realloc(
                    cls->properties, 
                    cls->numProperties*
                    sizeof(SDDLProperty));
            if (!cls->properties)
            {
                printf("OOM expanding cls->properties\n");
                return NULL;
            }
            cls->properties[cls->numProperties - 1] = SDDL_PROPERTY(control);
        }
        else if  (RedString_BeginsWith(key, "sensor "))
        {
            if (!RedJsonValue_IsObject(val))
            {
                printf("Expected object for sensor definition\n");
                return NULL;
            }
            RedJsonObject obj = RedJsonValue_GetObject(val);
            SDDLSensor sensor = _sddl_parse_sensor(key, obj);
            if (!sensor)
            {
                return NULL;
            }
            cls->numProperties++;
            cls->properties = realloc(
                    cls->properties, 
                    cls->numProperties*
                    sizeof(SDDLProperty));
            if (!cls->properties)
            {
                printf("OOM expanding cls->properties\n");
                return NULL;
            }
            cls->properties[cls->numProperties - 1] = SDDL_PROPERTY(sensor);
        }
        else if  (RedString_BeginsWith(key, "class "))
        {
            if (!RedJsonValue_IsObject(val))
            {
                printf("Expected object for class definition\n");
                return NULL;
            }
            RedJsonObject obj = RedJsonValue_GetObject(val);
            SDDLClass class = _sddl_parse_class(key, obj);
            if (!class)
            {
                return NULL;
            }
            cls->numProperties++;
            cls->properties = realloc(
                    cls->properties, 
                    cls->numProperties*
                    sizeof(SDDLProperty));
            if (!cls->properties)
            {
                printf("OOM expanding cls->properties\n");
                return NULL;
            }
            cls->properties[cls->numProperties - 1] = SDDL_PROPERTY(class);
        }
        else if (RedString_Equals(key, "authors"))
        {
            int j;
            if (!RedJsonValue_IsArray(val))
            {
                printf("Expected list for authors\n");
                return NULL;
            }
            
            RedJsonArray authorList = RedJsonValue_GetArray(val);
            cls->numAuthors = RedJsonArray_NumItems(authorList);
            cls->authors = calloc(cls->numAuthors, sizeof(char *));
            if (!cls->authors)
            {
                printf("OOM - Failed to allocate author list\n");
                return NULL;
            }
            for (j = 0; j < cls->numAuthors; j++)
            {
                char * author;
                if (!RedJsonArray_IsEntryString(authorList, j))
                {
                    printf("Expected string for authors list entry\n");
                    return NULL;
                }
                author = RedJsonArray_GetEntryString(authorList, j);
                cls->authors[j] = RedString_strdup(author);
                if (!cls->authors)
                {
                    printf("OOM - Failed to duplicate author\n");
                    return NULL;
                }
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
            if (cls->base.description)
                free(cls->base.description);
            cls->base.description = RedString_strdup(description);
            if (!cls->base.description)
            {
                printf("OOM duplicating description string\n");
                return NULL;
            }
        }

        RedString_Free(key);
    }

    return cls;
}

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
        if (RedString_BeginsWith(key, "control "))
        {
            if (!RedJsonValue_IsObject(val))
            {
                printf("Expected object for control definition\n");
                return NULL;
            }
            RedJsonObject obj = RedJsonValue_GetObject(val);
            SDDLControl control = _sddl_parse_control(key, obj);
            if (!control)
            {
                return NULL;
            }
            doc->numProperties++;
            doc->properties = realloc(
                    doc->properties, 
                    doc->numProperties*
                    sizeof(SDDLProperty));
            if (!doc->properties)
            {
                printf("OOM expanding doc->properties\n");
                return NULL;
            }
            doc->properties[doc->numProperties - 1] = SDDL_PROPERTY(control);
        }
        else if  (RedString_BeginsWith(key, "sensor "))
        {
            if (!RedJsonValue_IsObject(val))
            {
                printf("Expected object for sensor definition\n");
                return NULL;
            }
            RedJsonObject obj = RedJsonValue_GetObject(val);
            SDDLSensor sensor = _sddl_parse_sensor(key, obj);
            if (!sensor)
            {
                return NULL;
            }
            doc->numProperties++;
            doc->properties = realloc(
                    doc->properties, 
                    doc->numProperties*
                    sizeof(SDDLProperty));
            if (!doc->properties)
            {
                printf("OOM expanding doc->properties\n");
                return NULL;
            }
            doc->properties[doc->numProperties - 1] = SDDL_PROPERTY(sensor);
        }
        else if  (RedString_BeginsWith(key, "class "))
        {
            if (!RedJsonValue_IsObject(val))
            {
                printf("Expected object for class definition\n");
                return NULL;
            }
            RedJsonObject obj = RedJsonValue_GetObject(val);
            SDDLClass class = _sddl_parse_class(key, obj);
            if (!class)
            {
                return NULL;
            }
            doc->numProperties++;
            doc->properties = realloc(
                    doc->properties, 
                    doc->numProperties*
                    sizeof(SDDLProperty));
            if (!doc->properties)
            {
                printf("OOM expanding doc->properties\n");
                return NULL;
            }
            doc->properties[doc->numProperties - 1] = SDDL_PROPERTY(class);
        }
        else if (RedString_Equals(key, "authors"))
        {
            char * author;
            int j;
            if (!RedJsonValue_IsArray(val))
            {
                printf("Expected list for authors\n");
                return NULL;
            }
            
            RedJsonArray authorList = RedJsonValue_GetArray(val);
            doc->numAuthors = RedJsonArray_NumItems(authorList);
            doc->authors = calloc(doc->numAuthors, sizeof(char *));
            if (!doc->authors)
            {
                printf("OOM - Failed to allocate author list\n");
                return NULL;
            }
            for (j = 0; j < doc->numAuthors; j++)
            {
                if (!RedJsonArray_IsEntryString(authorList, j))
                {
                    printf("Expected string for authors list entry\n");
                    return NULL;
                }
                author = RedJsonArray_GetEntryString(authorList, j);
                doc->authors[j] = RedString_strdup(author);
                if (!doc->authors)
                {
                    printf("OOM - Failed to duplicate author\n");
                    return NULL;
                }
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
            if (doc->description)
                free(doc->description);
            doc->description = RedString_strdup(description);
            if (!doc->description)
            {
                printf("OOM duplicating description string\n");
                return NULL;
            }
        }

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
    return doc->numAuthors;
}

const char * sddl_document_author(SDDLDocument doc, unsigned index)
{
    return doc->authors[index];
}

unsigned sddl_document_num_properties(SDDLDocument doc)
{
    return doc->numProperties;
}

SDDLProperty sddl_document_property(SDDLDocument doc, unsigned index) 
{
    return doc->properties[index];
}

SDDLProperty sddl_document_lookup_property(SDDLDocument doc, const char*propName) 
{
    unsigned i;
    for (i = 0; i < sddl_document_num_properties(doc); i++)
    {
        if (!strcmp(doc->properties[i]->name, propName))
        {
            return doc->properties[i];
        }
    }
    return NULL;
}
SDDLClass sddl_document_lookup_class(SDDLDocument doc, const char*propName) 
{
    SDDLProperty prop = sddl_document_lookup_property(doc, propName);
    if (!prop)
        return NULL;
    if (!sddl_is_class(prop))
        return NULL;
    return SDDL_CLASS(prop);
}

bool sddl_is_control(SDDLProperty prop)
{
    return (prop->type == SDDL_PROPERTY_TYPE_CONTROL);
}
bool sddl_is_sensor(SDDLProperty prop)
{
    return (prop->type == SDDL_PROPERTY_TYPE_SENSOR);
}
bool sddl_is_class(SDDLProperty prop)
{
    return (prop->type == SDDL_PROPERTY_TYPE_CLASS);
}

SDDLControl SDDL_CONTROL(SDDLProperty prop)
{
    assert(sddl_is_control(prop));
    return (SDDLControl)prop;
}
SDDLSensor SDDL_SENSOR(SDDLProperty prop)
{
    assert(sddl_is_sensor(prop));
    return (SDDLSensor)prop;
}
SDDLClass SDDL_CLASS(SDDLProperty prop)
{
    assert(sddl_is_class(prop));
    return (SDDLClass)prop;
}

const char * sddl_control_name(SDDLControl control)
{
    return control->base.name;
}
SDDLControlTypeEnum sddl_control_type(SDDLControl control)
{
    return control->controlType;
}
SDDLDatatypeEnum sddl_control_datatype(SDDLControl control)
{
    return control->datatype;
}
const char * sddl_control_description(SDDLControl control)
{
    return control->base.description;
}
const double * sddl_control_max_value(SDDLControl control)
{
    return control->maxValue;
}
const double * sddl_control_min_value(SDDLControl control)
{
    return control->minValue;
}
SDDLNumericDisplayHintEnum sddl_control_numeric_display_hint(SDDLControl control)
{
    return control->numericDisplayHint;
}
const char * sddl_control_regex(SDDLControl control)
{
    return control->regex;
}
const char * sddl_control_units(SDDLControl control)
{
    return control->units;
}

void sddl_control_set_extra(SDDLControl control, void *extra)
{
    control->base.extra = extra;
}
void * sddl_control_extra(SDDLControl control)
{
    return control->base.extra;
}

const char * sddl_sensor_name(SDDLSensor sensor)
{
    return sensor->base.name;
}
SDDLDatatypeEnum sddl_sensor_datatype(SDDLSensor sensor)
{
    return sensor->datatype;
}
const char * sddl_sensor_description(SDDLSensor sensor)
{
    return sensor->base.description;
}
const double * sddl_sensor_max_value(SDDLSensor sensor)
{
    return sensor->maxValue;
}
const double * sddl_sensor_min_value(SDDLSensor sensor)
{
    return sensor->minValue;
}
SDDLNumericDisplayHintEnum sddl_sensor_numeric_display_hint(SDDLSensor sensor)
{
    return sensor->numericDisplayHint;
}
const char * sddl_sensor_regex(SDDLSensor sensor)
{
    return sensor->regex;
}
const char * sddl_sensor_units(SDDLSensor sensor)
{
    return sensor->units;
}
void sddl_sensor_set_extra(SDDLSensor sensor, void *extra)
{
    sensor->base.extra = extra;
}
void * sddl_sensor_extra(SDDLSensor sensor)
{
    return sensor->base.extra;
}

const char * sddl_class_name(SDDLClass cls)
{
    return cls->base.name;
}
RedJsonObject sddl_class_json(SDDLClass cls)
{
    return cls->json;
}
unsigned sddl_class_num_authors(SDDLClass cls)
{
    return cls->numAuthors;
}
const char * sddl_class_author(SDDLClass cls, unsigned index)
{
    return cls->authors[index];
}
const char * sddl_class_description(SDDLClass cls)
{
    return cls->base.description;
}
unsigned sddl_class_num_properties(SDDLClass cls)
{
    return cls->numProperties;
}
SDDLProperty sddl_class_property(SDDLClass cls, unsigned index) 
{
    return cls->properties[index];
}

SDDLProperty sddl_class_lookup_property(SDDLClass cls, const char*propName) 
{
    unsigned i;
    for (i = 0; i < sddl_class_num_properties(cls); i++)
    {
        if (!strcmp(cls->properties[i]->name, propName))
        {
            return cls->properties[i];
        }
    }
    return NULL;
}

SDDLControl sddl_class_lookup_control(SDDLClass cls, const char*propName) 
{
    SDDLProperty prop = sddl_class_lookup_property(cls, propName);
    if (!prop)
        return NULL;
    if (!sddl_is_control(prop))
        return NULL;
    return SDDL_CONTROL(prop);
}

SDDLSensor sddl_class_lookup_sensor(SDDLClass cls, const char*propName) 
{
    SDDLProperty prop = sddl_class_lookup_property(cls, propName);
    if (!prop)
        return NULL;
    if (!sddl_is_sensor(prop))
        return NULL;
    return SDDL_SENSOR(prop);
}
