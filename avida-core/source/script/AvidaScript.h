/*
 *  AvidaScript.h
 *  avida_test_language
 *
 *  Created by David on 1/14/06.
 *  Copyright 1999-2011 Michigan State University. All rights reserved.
 *
 *
 *  This file is part of Avida.
 *
 *  Avida is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 *  Avida is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with Avida.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AvidaScript_h
#define AvidaScript_h

#include "cString.h"

typedef enum eASTokens {
  AS_TOKEN_SUPPRESS = 1,
  AS_TOKEN_ENDL,
  AS_TOKEN_COMMA,
  
  AS_TOKEN_OP_BIT_NOT, // 4
  AS_TOKEN_OP_BIT_AND,
  AS_TOKEN_OP_BIT_OR,
  
  AS_TOKEN_OP_LOGIC_NOT, // 7
  AS_TOKEN_OP_LOGIC_AND,
  AS_TOKEN_OP_LOGIC_OR,
  
  AS_TOKEN_OP_ADD, // 10
  AS_TOKEN_OP_SUB,
  AS_TOKEN_OP_MUL,
  AS_TOKEN_OP_DIV,
  AS_TOKEN_OP_MOD,
  
  AS_TOKEN_DOT, // 15
  AS_TOKEN_ASSIGN,
  AS_TOKEN_REF,
  
  AS_TOKEN_OP_EQ, // 18
  AS_TOKEN_OP_LE,
  AS_TOKEN_OP_GE,
  AS_TOKEN_OP_LT,
  AS_TOKEN_OP_GT,
  AS_TOKEN_OP_NEQ,
  
  AS_TOKEN_PREC_OPEN, // 24
  AS_TOKEN_PREC_CLOSE,
  
  AS_TOKEN_IDX_OPEN, // 26
  AS_TOKEN_IDX_CLOSE,
  
  AS_TOKEN_ARR_OPEN, // 28
  AS_TOKEN_ARR_CLOSE,
  AS_TOKEN_ARR_RANGE,
  AS_TOKEN_ARR_EXPAN,
  AS_TOKEN_ARR_WILD,
  
  AS_TOKEN_LITERAL_DICT, // 33
  AS_TOKEN_DICT_MAPPING,
  AS_TOKEN_LITERAL_MATRIX,
  
  AS_TOKEN_TYPE_ARRAY, // 36
  AS_TOKEN_TYPE_BOOL,
  AS_TOKEN_TYPE_CHAR,
  AS_TOKEN_TYPE_DICT,
  AS_TOKEN_TYPE_FLOAT,
  AS_TOKEN_TYPE_INT,
  AS_TOKEN_TYPE_MATRIX,
  AS_TOKEN_TYPE_STRING,
  AS_TOKEN_TYPE_VAR,
  AS_TOKEN_TYPE_VOID,
  
  AS_TOKEN_CMD_IF, // 46
  AS_TOKEN_CMD_ELSE,
  AS_TOKEN_CMD_ELSEIF,
  AS_TOKEN_CMD_WHILE,
  AS_TOKEN_CMD_FOREACH,
  AS_TOKEN_CMD_FUNCTION,
  
  AS_TOKEN_CMD_RETURN, // 52
  
  AS_TOKEN_BUILTIN_CALL, // 53
  AS_TOKEN_BUILTIN_METHOD,
  
  AS_TOKEN_ID, // 55
  
  AS_TOKEN_FLOAT, // 56
  AS_TOKEN_INT,
  AS_TOKEN_STRING,
  AS_TOKEN_CHAR,
  AS_TOKEN_BOOL,
  
  AS_TOKEN_UNKNOWN, // 60
  AS_TOKEN_INVALID
} ASToken_t;


typedef enum eASParseErrors {
  AS_PARSE_ERR_UNEXPECTED_TOKEN,
  AS_PARSE_ERR_UNTERMINATED_EXPR,
  AS_PARSE_ERR_NULL_EXPR,
  AS_PARSE_ERR_EOF,
  AS_PARSE_ERR_EMPTY,
  AS_PARSE_ERR_INTERNAL,
  
  AS_PARSE_ERR_UNKNOWN
} ASParseError_t;


typedef enum eASSemanticErrors {
  AS_SEMANTIC_WARN_LOSS_OF_PRECISION,
  AS_SEMANTIC_WARN_NO_DIMENSIONS,
  AS_SEMANTIC_WARN_NO_RETURN,
  AS_SEMANTIC_WARN_UNREACHABLE,
  AS_SEMANTIC_WARN__LAST,
  
  AS_SEMANTIC_ERR_ARGUMENT_DEFAULT_REQUIRED,
  AS_SEMANTIC_ERR_ARGUMENT_MISSING_REQUIRED,
  AS_SEMANTIC_ERR_BUILTIN_CALL_SIGNATURE_MISMATCH,
  AS_SEMANTIC_ERR_CANNOT_CAST,
  AS_SEMANTIC_ERR_CANNOT_COMPARE,
  AS_SEMANTIC_ERR_CANNOT_OVERRIDE_LIB_FUNCTION,
  AS_SEMANTIC_ERR_FUNCTION_CALL_SIGNATURE_MISMATCH,
  AS_SEMANTIC_ERR_FUNCTION_DEFAULT_CALL_INVALID,
  AS_SEMANTIC_ERR_FUNCTION_DEFAULT_VARIABLE_REF_INVALID,
  AS_SEMANTIC_ERR_FUNCTION_REDEFINITION,
  AS_SEMANTIC_ERR_FUNCTION_RTYPE_MISMATCH,
  AS_SEMANTIC_ERR_FUNCTION_SIGNATURE_MISMATCH,
  AS_SEMANTIC_ERR_FUNCTION_UNDECLARED,
  AS_SEMANTIC_ERR_FUNCTION_UNDEFINED,
  AS_SEMANTIC_ERR_INVALID_ASSIGNMENT_TARGET,
  AS_SEMANTIC_ERR_INVALID_CHAR_LITERAL,
  AS_SEMANTIC_ERR_TOO_MANY_ARGUMENTS,
  AS_SEMANTIC_ERR_UNDEFINED_TYPE_OP,
  AS_SEMANTIC_ERR_UNPACK_WILD_NONARRAY,
  AS_SEMANTIC_ERR_VARIABLE_DIMENSIONS_INVALID,
  AS_SEMANTIC_ERR_VARIABLE_UNDEFINED,
  AS_SEMANTIC_ERR_VARIABLE_REDEFINITION,
  AS_SEMANTIC_ERR_INTERNAL,
  
  AS_SEMANTIC_ERR_UNKNOWN
} ASSemanticError_t;

typedef enum eASDirectInterpretErrors {
  AS_DIRECT_INTERPRET_ERR_CANNOT_RESIZE_MATRIX_ROW,
  AS_DIRECT_INTERPRET_ERR_DIVISION_BY_ZERO,
  AS_DIRECT_INTERPRET_ERR_INDEX_OUT_OF_BOUNDS,
  AS_DIRECT_INTERPRET_ERR_INDEX_ERROR,
  AS_DIRECT_INTERPRET_ERR_INVALID_ARRAY_SIZE,
  AS_DIRECT_INTERPRET_ERR_KEY_NOT_FOUND,
  AS_DIRECT_INTERPRET_ERR_MATRIX_OP_TYPE_MISMATCH,
  AS_DIRECT_INTERPRET_ERR_MATRIX_SIZE_MISMATCH,
  AS_DIRECT_INTERPRET_ERR_NOBJ_METHOD_LOOKUP_FAILED,
  AS_DIRECT_INTERPRET_ERR_NOBJ_TYPE_MISMATCH,
  AS_DIRECT_INTERPRET_ERR_OBJECT_ASSIGN_FAIL,
  AS_DIRECT_INTERPRET_ERR_TYPE_CAST,
  AS_DIRECT_INTERPRET_ERR_UNDEFINED_TYPE_OP,
  AS_DIRECT_INTERPRET_ERR_UNPACK_VALUE_TOO_LARGE,
  AS_DIRECT_INTERPRET_ERR_UNPACK_VALUE_TOO_SMALL,  
  AS_DIRECT_INTERPRET_ERR_INTERNAL,
  
  AS_DIRECT_INTERPRET_ERR_UNKNOWN
} ASDirectInterpretError_t;

typedef enum eASBuiltInFunction {
  AS_BUILTIN_CAST_BOOL,
  AS_BUILTIN_CAST_CHAR,
  AS_BUILTIN_CAST_INT,
  AS_BUILTIN_CAST_FLOAT,
  AS_BUILTIN_CAST_STRING,
  AS_BUILTIN_IS_ARRAY,
  AS_BUILTIN_IS_BOOL,
  AS_BUILTIN_IS_CHAR,
  AS_BUILTIN_IS_DICT,
  AS_BUILTIN_IS_INT,
  AS_BUILTIN_IS_FLOAT,
  AS_BUILTIN_IS_MATRIX,
  AS_BUILTIN_IS_STRING,
  AS_BUILTIN_CLEAR,
  AS_BUILTIN_COPY,
  AS_BUILTIN_HASKEY,
  AS_BUILTIN_KEYS,
  AS_BUILTIN_LEN,
  AS_BUILTIN_REMOVE,
  AS_BUILTIN_RESIZE,
  AS_BUILTIN_VALUES,
  
  AS_BUILTIN_UNKNOWN
} ASBuiltIn_t;

enum eASExitCodes {
  AS_EXIT_OK = 0,
  AS_EXIT_FILE_NOT_FOUND = 200,
  AS_EXIT_FAIL_PARSE,
  AS_EXIT_FAIL_SEMANTIC,
  AS_EXIT_FAIL_INTERPRET,
  
  AS_EXIT_INTERNAL_ERROR,

  AS_EXIT_UNKNOWN
};

typedef enum eASTypes {
  AS_TYPE_ARRAY = 0,
  AS_TYPE_BOOL,
  AS_TYPE_CHAR,
  AS_TYPE_DICT,
  AS_TYPE_FLOAT,
  AS_TYPE_INT,
  AS_TYPE_OBJECT_REF,
  AS_TYPE_MATRIX,
  AS_TYPE_STRING,
  AS_TYPE_VAR,
  
  AS_TYPE_RUNTIME,
  
  AS_TYPE_VOID,

  AS_TYPE_INVALID
} ASType_t;

struct sASTypeInfo {
  ASType_t type;
  cString info;
  
  sASTypeInfo() : type(AS_TYPE_INVALID) { ; }
  sASTypeInfo(ASType_t in_type) : type(in_type) { ; }
  sASTypeInfo(ASType_t in_type, const cString& in_info) : type(in_type), info(in_info) { ; }
  
  bool operator==(const sASTypeInfo& ot) const { return (type == ot.type && info == ot.info); }
  bool operator!=(const sASTypeInfo& ot) const { return (type != ot.type || info != ot.info); }
};

enum eASMethodReturnValues {
  AS_NOT_FOUND = -1
};


class cASNativeObject;

namespace AvidaScript {
  const char* mapBuiltIn(ASBuiltIn_t builtin);  
  const char* mapToken(ASToken_t token);
  const char* mapType(const sASTypeInfo& type);
  
  template<typename T> inline sASTypeInfo TypeOf() { return sASTypeInfo(AS_TYPE_INVALID); }
  template<> inline sASTypeInfo TypeOf<bool>() { return sASTypeInfo(AS_TYPE_BOOL); }
  template<> inline sASTypeInfo TypeOf<char>() { return sASTypeInfo(AS_TYPE_CHAR); }
  template<> inline sASTypeInfo TypeOf<int>() { return sASTypeInfo(AS_TYPE_INT); }
  template<> inline sASTypeInfo TypeOf<double>() { return sASTypeInfo(AS_TYPE_FLOAT); }
  template<> inline sASTypeInfo TypeOf<const cString&>() { return sASTypeInfo(AS_TYPE_STRING); }
  template<> inline sASTypeInfo TypeOf<cString>() { return sASTypeInfo(AS_TYPE_STRING); }
  template<> inline sASTypeInfo TypeOf<void>() { return sASTypeInfo(AS_TYPE_VOID); }
  template<> inline sASTypeInfo TypeOf<cASNativeObject>() { return sASTypeInfo(AS_TYPE_OBJECT_REF); }
};

#endif
