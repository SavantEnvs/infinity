// Fuzz the filter/output expression grammar (src/parser/expression_lexer.l +
// expression_parser.y) through infinity::ExprParser::Parse, which the Thrift and HTTP APIs call
// on client-supplied filter / output / sort expression strings, then print and free the
// resulting ParsedExpr trees.
//
// State kept across inputs: none. A fresh ExprParser and ExpressionParserResult per input.
#include "expr_parser.h"
#include "expression_parser_result.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    std::string text(reinterpret_cast<const char *>(data), size);
    infinity::ExprParser parser;
    infinity::ExpressionParserResult result{};
    try {
        parser.Parse(text, &result);
        if (!result.IsError() && result.exprs_ptr_ != nullptr) {
            for (auto *expr : *result.exprs_ptr_) {
                try {
                    (void)expr->ToString();
                } catch (const std::exception &) {
                }
            }
        }
        (void)result.ToString();
    } catch (const std::exception &) {
    }
    result.Reset();
    return 0;
}
