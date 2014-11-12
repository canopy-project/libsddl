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

#include <sddl.h>
#include <red_test.h>

int main(int argc, const char *argv[])
{
    RedTest test;
    test = RedTest_Begin(argv[0], NULL, NULL);

    SDDLParseResult result = sddl_load_and_parse("test1.sddl");
    RedTest_Verify(test, "test1.sddl - Parsing OK", sddl_parse_result_ok(result));

    SDDLDocument doc = sddl_parse_result_document(result);
    RedTest_Verify(test, "test1.sddl - 0 vars", sddl_document_num_vars(doc) == 0);
    sddl_free_parse_result(result);

    return RedTest_End(test);
}

