// Force-included (-include) into every src/parser translation unit by mayhem/build.sh.
//
// Upstream compiles src/parser as part of a C++23-modules build (`import std.compat;`,
// `import third_party;`, clang>=20 + vcpkg). The parser headers guard those imports with
// PARESER_USE_STD_MODULE / PARESER_USE_THIRD_PARTY_MODULE, so defining both and providing the
// same declarations as textual headers lets the parser build as an ordinary static library.
#pragma once
#define PARESER_USE_STD_MODULE 1
#define PARESER_USE_THIRD_PARTY_MODULE 1
#ifdef __cplusplus
#include <bits/stdc++.h>
#include <source_location>
#include <format>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <simdjson.h>

// src/common/third_party.cppm re-exports these ondemand names at simdjson:: scope.
namespace simdjson {
using ondemand::array;
using ondemand::document;
using ondemand::field;
using ondemand::json_type;
using ondemand::number;
using ondemand::number_type;
using ondemand::object;
using ondemand::parser;
using ondemand::value;
} // namespace simdjson

#include "arrow_decls.h"
#endif
