// Fuzz infinity::ColumnDef::FromJson, the deserializer the storage layer runs on every column
// definition it reads back from the catalog (new_catalog_impl.cpp, table_meta_impl.cpp). It
// walks simdjson ondemand over the record and recurses through DataType::Deserialize (array /
// decimal / embedding / tensor / sparse type info, SparseInfo::Deserialize) and
// ConstantExpr::Deserialize (column default values). The result is re-serialized with ToJson
// and printed with ToString.
//
// State kept across inputs: none.
#include "definition/column_def.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    std::string_view json(reinterpret_cast<const char *>(data), size);
    try {
        auto col = infinity::ColumnDef::FromJson(json);
        if (col) {
            (void)col->ToString();
            (void)col->ToJson().dump();
        }
    } catch (const std::exception &) {
        // simdjson_error, nlohmann::json::exception and ParserException are the documented
        // error channels for malformed records.
    }
    return 0;
}
