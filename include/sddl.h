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

#ifndef SDDL_INCLUDED
#define SDDL_INCLUDED

#include "red_json.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef enum {
    SDDL_SUCCESS,

    /*
     * Parsing SDDL content failed.
     */
    SDDL_ERROR_PARSING,
} SDDLResultEnum;

typedef enum
{
    SDDL_DATATYPE_INVALID,
    SDDL_DATATYPE_VOID,
    SDDL_DATATYPE_STRING,
    SDDL_DATATYPE_BOOL,
    SDDL_DATATYPE_INT8,
    SDDL_DATATYPE_UINT8,
    SDDL_DATATYPE_INT16,
    SDDL_DATATYPE_UINT16,
    SDDL_DATATYPE_INT32,
    SDDL_DATATYPE_UINT32,
    SDDL_DATATYPE_FLOAT32,
    SDDL_DATATYPE_FLOAT64,
    SDDL_DATATYPE_DATETIME,
    SDDL_DATATYPE_STRUCT,
    SDDL_DATATYPE_ARRAY,
} SDDLDatatypeEnum;

typedef enum
{
    SDDL_DIRECTION_INVALID,
    SDDL_DIRECTION_INHERIT,
    SDDL_DIRECTION_INOUT,
    SDDL_DIRECTION_IN,
    SDDL_DIRECTION_OUT
} SDDLDirectionEnum;

typedef enum
{
    SDDL_OPTIONALITY_INVALID,
    SDDL_OPTIONALITY_OPTIONAL,
    SDDL_OPTIONALITY_REQUIRED,
} SDDLOptionalityEnum;

typedef enum
{
    SDDL_NUMERIC_DISPLAY_HINT_INVALID,
    SDDL_NUMERIC_DISPLAY_HINT_NORMAL,
    SDDL_NUMERIC_DISPLAY_HINT_PERCENTAGE,
    SDDL_NUMERIC_DISPLAY_HINT_SCIENTIFIC,
    SDDL_NUMERIC_DISPLAY_HINT_HEX
} SDDLNumericDisplayHintEnum;

typedef struct SDDLDocument_t * SDDLDocument;

typedef struct SDDLVarDecl_t * SDDLVarDecl;

typedef struct SDDLParseResult_t * SDDLParseResult;

SDDLParseResult sddl_load_and_parse(const char *filename);
SDDLParseResult sddl_load_and_parse_file(FILE *file);
SDDLParseResult sddl_parse(const char *sddl);

bool sddl_parse_result_ok(SDDLParseResult result);
SDDLDocument sddl_parse_result_document(SDDLParseResult result);
SDDLDocument sddl_parse_result_ref_document(SDDLParseResult result);
unsigned sddl_parse_result_num_errors(SDDLParseResult result);
const char * sddl_parse_result_error(SDDLParseResult result, unsigned index);
unsigned sddl_parse_result_num_warnings(SDDLParseResult result);
const char * sddl_parse_result_warning(SDDLParseResult result, unsigned index);
void sddl_free_parse_result(SDDLParseResult result);

const char * sddl_document_description(SDDLDocument doc);
unsigned sddl_document_num_authors(SDDLDocument doc);
const char * sddl_document_author(SDDLDocument doc, unsigned index);

unsigned sddl_document_num_vars(SDDLDocument doc);
SDDLVarDecl sddl_document_var_by_idx(SDDLDocument doc, unsigned index);
SDDLVarDecl sddl_document_var_by_name(SDDLDocument doc, const char *name);


const char * sddl_var_name(SDDLVarDecl var);
SDDLDatatypeEnum sddl_var_datatype(SDDLVarDecl var);
const char * sddl_var_description(SDDLVarDecl var);
const double * sddl_var_max_value(SDDLVarDecl var);
const double * sddl_var_min_value(SDDLVarDecl var);
SDDLNumericDisplayHintEnum sddl_var_numeric_display_hint(SDDLVarDecl var);
const char * sddl_var_regex(SDDLVarDecl var);
const char * sddl_var_units(SDDLVarDecl var);

SDDLDirectionEnum sddl_var_direction(SDDLVarDecl var);
SDDLDirectionEnum sddl_var_concrete_direction(SDDLVarDecl var);

unsigned sddl_var_struct_num_members(SDDLVarDecl var);
SDDLVarDecl sddl_var_struct_member_by_idx(SDDLVarDecl var, unsigned index);
SDDLVarDecl sddl_var_struct_member_by_name(SDDLVarDecl var, const char *name);

unsigned sddl_var_array_num_elements(SDDLVarDecl var);

void sddl_var_set_extra(SDDLVarDecl var, void *extra);
void * sddl_var_extra(SDDLVarDecl var);

RedJsonObject sddl_var_json(SDDLVarDecl var);

// Sets *outName to point to a newly allocated string
SDDLResultEnum sddl_parse_decl(
        const char *decl, 
        SDDLDirectionEnum *outDirection, 
        SDDLDatatypeEnum *outDatatype, 
        char **outName,
        SDDLDatatypeEnum *outArrayElementDatatype,
        size_t *outArraySize);

// Create new Cloud Variable Declaration for a basic variable with default
// properties.
SDDLVarDecl sddl_var_new_basic(
        SDDLDatatypeEnum datatype, 
        SDDLDirectionEnum direction, 
        const char *name);

SDDLVarDecl sddl_var_new_basic(SDDLDatatypeEnum datatype, SDDLDirectionEnum direction, const char *name);
SDDLVarDecl sddl_var_new_array(SDDLDatatypeEnum childDatatype, size_t numItems, SDDLDirectionEnum direction, const char *name);
SDDLVarDecl sddl_var_new_struct(SDDLDirectionEnum direction, const char *name);
bool sddl_var_struct_add_member(SDDLVarDecl strct, SDDLVarDecl member);

bool sddl_var_is_basic(SDDLVarDecl var);
bool sddl_datatype_is_basic(SDDLDatatypeEnum datatype);

const char *sddl_var_decl_string(SDDLVarDecl var);

const char * sddl_direction_string(SDDLDirectionEnum direction);
const char * sddl_datatype_string(SDDLDatatypeEnum datatype);

#endif

