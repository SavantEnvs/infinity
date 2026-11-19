// Declarations of the few Apache Arrow types src/parser names.
//
// The parser declares comparison operators between its own types and arrow::DataType /
// ListType / StructType, used only by the engine's Parquet import (physical_import). Nothing
// on the SQL/expression parsing path calls them, and Arrow is not packaged in Debian, so the
// fuzz build declares just these types instead of building Arrow. No arrow object is ever
// constructed by the harnesses.
#pragma once
#include <memory>
#include <string>

namespace arrow {
struct Type {
    enum type {
        NA = 0, BOOL, UINT8, INT8, UINT16, INT16, UINT32, INT32, UINT64, INT64, HALF_FLOAT, FLOAT,
        DOUBLE, STRING, BINARY, FIXED_SIZE_BINARY, DATE32, DATE64, TIMESTAMP, TIME32, TIME64,
        INTERVAL_MONTHS, INTERVAL_DAY_TIME, DECIMAL128, DECIMAL256, LIST, STRUCT, SPARSE_UNION,
        DENSE_UNION, DICTIONARY, MAP, EXTENSION, FIXED_SIZE_LIST, DURATION, LARGE_STRING,
        LARGE_BINARY, LARGE_LIST
    };
};

class DataType {
public:
    explicit DataType(Type::type id) : id_(id) {}
    virtual ~DataType() = default;
    Type::type id() const { return id_; }

private:
    Type::type id_;
};

class Field {
public:
    Field(std::string name, std::shared_ptr<DataType> type) : name_(std::move(name)), type_(std::move(type)) {}
    const std::shared_ptr<DataType> &type() const { return type_; }

private:
    std::string name_;
    std::shared_ptr<DataType> type_;
};

class ListType : public DataType {
public:
    explicit ListType(std::shared_ptr<Field> value_field)
        : DataType(Type::LIST), value_field_(std::move(value_field)), value_type_(value_field_->type()) {}
    const std::shared_ptr<Field> &value_field() const { return value_field_; }
    const std::shared_ptr<DataType> &value_type() const { return value_type_; }

private:
    std::shared_ptr<Field> value_field_;
    std::shared_ptr<DataType> value_type_;
};

class FixedSizeListType : public DataType {
public:
    FixedSizeListType(std::shared_ptr<DataType> value_type, int32_t list_size)
        : DataType(Type::FIXED_SIZE_LIST), value_type_(std::move(value_type)), list_size_(list_size) {}
    const std::shared_ptr<DataType> &value_type() const { return value_type_; }
    int32_t list_size() const { return list_size_; }

private:
    std::shared_ptr<DataType> value_type_;
    int32_t list_size_;
};

class StructType : public DataType {
public:
    StructType() : DataType(Type::STRUCT) {}
    std::shared_ptr<Field> GetFieldByName(const std::string &) const { return nullptr; }
};
} // namespace arrow
