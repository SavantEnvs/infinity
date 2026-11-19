// Known-answer driver for the SQL and expression parsers (used by mayhem/test.sh).
// Reads one case per line from argv[2]; argv[1] is "sql" or "expr". For each case prints one
// line: "ERR" when the parser reports an error, else the parsed statement types (sql) or the
// ToString of each parsed expression (expr). test.sh diffs this against a golden file.
// The SQL and expression grammars' generated token enums collide, so the driver is compiled
// twice: -DSELFTEST_SQL (sql mode) and -DSELFTEST_EXPR (expr mode).
#ifdef SELFTEST_SQL
#include "sql_parser.h"
#else
#include "expr_parser.h"
#include "expression_parser_result.h"
#endif

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "usage: parser_selftest sql|expr <cases>\n";
        return 2;
    }
    std::string mode = argv[1];
    std::ifstream in(argv[2]);
    if (!in) {
        std::cerr << "cannot open " << argv[2] << "\n";
        return 2;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty())
            continue;
        std::string out;
        try {
#ifdef SELFTEST_SQL
            if (mode == "sql") {
                infinity::SQLParser p;
                infinity::ParserResult r{};
                p.Parse(line, &r);
                if (r.IsError() || r.statements_ptr_ == nullptr) {
                    out = "ERR";
                } else {
                    out = std::to_string(r.statements_ptr_->size());
                    for (auto *s : *r.statements_ptr_)
                        out += " " + infinity::StatementType2Str(s->Type());
                }
                r.Reset();
            } else {
                out = "BADMODE";
            }
#else
            if (mode == "expr") {
                infinity::ExprParser p;
                infinity::ExpressionParserResult r{};
                p.Parse(line, &r);
                if (r.IsError() || r.exprs_ptr_ == nullptr) {
                    out = "ERR";
                } else {
                    for (auto *e : *r.exprs_ptr_)
                        out += (out.empty() ? "" : " | ") + e->ToString();
                }
                r.Reset();
            } else {
                out = "BADMODE";
            }
#endif
        } catch (const std::exception &e) {
            out = std::string("EXC ") + e.what();
        }
        std::cout << out << "\n";
    }
    return 0;
}
