if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED NAMESPACE)
    message(FATAL_ERROR "INPUT, OUTPUT and NAMESPACE are required")
endif()

file(READ "${INPUT}" SCRIPT_SOURCE)
string(FIND "${SCRIPT_SOURCE}" ")NCTPAYLOAD\"" SCRIPT_DELIMITER_INDEX)
if(NOT SCRIPT_DELIMITER_INDEX EQUAL -1)
    message(FATAL_ERROR "script contains generated raw string delimiter")
endif()

get_filename_component(OUTPUT_DIR "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
string(LENGTH "${SCRIPT_SOURCE}" SCRIPT_LENGTH)
set(SCRIPT_OFFSET 0)
set(SCRIPT_HEADER "#pragma once

#include <string>

namespace ${NAMESPACE} {
inline std::string source() {
    return std::string(
")

while(SCRIPT_OFFSET LESS SCRIPT_LENGTH)
    math(EXPR SCRIPT_REMAINING "${SCRIPT_LENGTH} - ${SCRIPT_OFFSET}")
    if(SCRIPT_REMAINING GREATER 8000)
        set(SCRIPT_CHUNK_LENGTH 8000)
    else()
        set(SCRIPT_CHUNK_LENGTH "${SCRIPT_REMAINING}")
    endif()
    string(SUBSTRING "${SCRIPT_SOURCE}" "${SCRIPT_OFFSET}" "${SCRIPT_CHUNK_LENGTH}" SCRIPT_CHUNK)
    string(APPEND SCRIPT_HEADER "R\"NCTPAYLOAD(${SCRIPT_CHUNK})NCTPAYLOAD\"
")
    math(EXPR SCRIPT_OFFSET "${SCRIPT_OFFSET} + ${SCRIPT_CHUNK_LENGTH}")
endwhile()

string(APPEND SCRIPT_HEADER "    );
}
}
")
file(WRITE "${OUTPUT}" "${SCRIPT_HEADER}")


# ============================================================================
#  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
#  Inert per-file identifier required by the EULA. Uniquely tags this file so
#  this program is distinguishable from any other build. No functional logic.
#  file: src/executor/embed_payload.cmake
#  mark: 17379e51fb2bb18b88e2319bb9aadddc
# ============================================================================
