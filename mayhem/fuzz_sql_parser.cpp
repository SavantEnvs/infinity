// Fuzz the SQL grammar (src/parser/lexer.l + parser.y) through infinity::SQLParser::Parse, the
// same entry point the server uses for every SQL text it receives (Thrift/HTTP/PG wire), then
// walk the resulting statement AST via ParserResult::ToString and free it with Reset.
//
// State kept across inputs: none. A fresh SQLParser (its own reentrant flex scanner) and a fresh
// ParserResult are made per input.
#include "sql_parser.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // SQLParser::Parse takes a NUL-terminated std::string (sql_scan_string uses c_str()).
    std::string sql(reinterpret_cast<const char *>(data), size);
    infinity::SQLParser parser;
    infinity::ParserResult result{};
    try {
        parser.Parse(sql, &result);
        if (!result.IsError() && result.statements_ptr_ != nullptr) {
            for (auto *stmt : *result.statements_ptr_) {
                (void)stmt->Type();
                try {
                    (void)stmt->ToString();
                } catch (const std::exception &) {
                    // Several statements' ToString throw ParserException ("not implemented").
                }
            }
        }
        (void)result.ToString();
    } catch (const std::exception &) {
        // ParserException / std::exception is the parser's documented error channel.
    }
    result.Reset();
    return 0;
}
