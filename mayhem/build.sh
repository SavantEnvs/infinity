#!/usr/bin/env bash
# mayhem/build.sh — build infinity's SQL and expression parsers (src/parser) and their harnesses.
#
# Upstream builds src/parser inside a C++23-modules build of the whole engine (clang>=20,
# CMake>=4, vcpkg: arrow, rocksdb, boost, thrift, oatpp, ...). The parser itself only needs the
# standard library, nlohmann::json, fmt and simdjson, and its headers guard their
# `import std.compat;` / `import third_party;` lines with PARESER_USE_STD_MODULE /
# PARESER_USE_THIRD_PARTY_MODULE. So this builds the parser sources as plain C++23:
#   * mayhem/parser_prelude.h is force-included (defines both guards, includes the headers the
#     modules would have exported, plus mayhem/arrow_decls.h for the few Arrow types the parser
#     names only in Parquet-import comparison operators);
#   * the flex-generated lexer.cpp / expression_lexer.cpp carry an unguarded `import std;`, so
#     they are compiled from copies under build-mayhem/ with that one line removed;
#   * search_parser.cpp / search_lexer.cpp import infinity_core (the full-text engine) and are
#     not part of this build.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
cd "$SRC"

P="$SRC/src/parser"
OUT="$SRC/build-mayhem"          # gitignored by upstream's `build*/`
mkdir -p "$OUT/gen"

# Generated-lexer copies without the unguarded `import std;` (rewritten only when changed).
for l in lexer expression_lexer; do
  sed '/^import std;$/d' "$P/$l.cpp" > "$OUT/gen/$l.cpp.tmp"
  cmp -s "$OUT/gen/$l.cpp.tmp" "$OUT/gen/$l.cpp" 2>/dev/null && rm -f "$OUT/gen/$l.cpp.tmp" \
    || mv -f "$OUT/gen/$l.cpp.tmp" "$OUT/gen/$l.cpp"
done

SOURCES=()
while IFS= read -r f; do SOURCES+=("$f"); done < <(
  cd "$P" && find . -name '*.cpp' ! -name main.cpp ! -name search_parser.cpp ! -name search_lexer.cpp \
    ! -name lexer.cpp ! -name expression_lexer.cpp | LC_ALL=C sort)
SOURCES+=("$OUT/gen/lexer.cpp" "$OUT/gen/expression_lexer.cpp")

INC=(-include "$SRC/mayhem/parser_prelude.h" -I"$SRC/mayhem" -I"$P" -I"$SRC/src")
CXXSTD=(-std=c++23 -w)

# build_lib <dir> <flags...>: compile every parser TU into <dir>/libsql_parser.a (make-driven so a
# re-run only recompiles what changed).
build_lib() {
  local dir="$1"; shift
  local flags="$*"
  mkdir -p "$dir/obj"
  {
    echo "CXX := $CXX"
    echo "FLAGS := ${CXXSTD[*]} ${INC[*]} $flags"
    echo "OBJS :="
    local i=0
    for s in "${SOURCES[@]}"; do
      case "$s" in /*) abs="$s" ;; *) abs="$P/${s#./}" ;; esac
      o="$dir/obj/$i.o"
      echo "OBJS += $o"
      echo "$o: $abs"
      printf '\t$(CXX) $(FLAGS) -MMD -MP -c %s -o %s\n' "$abs" "$o"
      echo "-include $dir/obj/$i.d"
      i=$((i+1))
    done
    echo "$dir/libsql_parser.a: \$(OBJS) $dir/flags.txt"
    printf '\trm -f $@ && llvm-ar rcs $@ $(OBJS)\n'
  } > "$dir/Makefile.new"
  echo "$flags" > "$dir/flags.txt.new"
  cmp -s "$dir/flags.txt.new" "$dir/flags.txt" 2>/dev/null || { mv -f "$dir/flags.txt.new" "$dir/flags.txt"; rm -f "$dir"/obj/*.o; }
  rm -f "$dir/flags.txt.new"
  mv -f "$dir/Makefile.new" "$dir/Makefile"
  make -s -f "$dir/Makefile" -j"$MAYHEM_JOBS" "$dir/libsql_parser.a"
}

LIBS=(-lfmt -lsimdjson)

# 1) Sanitized + DWARF-3 parser library and the two harnesses (fuzzer + standalone).
# SanCov on the parser itself (not just the harness); the ASan runtime supplies the callbacks for
# the standalone reproducer, so this is skipped on the no-sanitizer build.
COV=""; [ -z "$SANITIZER_FLAGS" ] || COV="-fsanitize=fuzzer-no-link"
build_lib "$OUT/fuzz" "-O1 $SANITIZER_FLAGS $DEBUG_FLAGS $COV"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OUT/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"
for h in sql_parser expr_parser column_def_json; do
  $CXX "${CXXSTD[@]}" "${INC[@]}" $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE \
    "$SRC/mayhem/fuzz_$h.cpp" "$OUT/lsan_off.o" "$OUT/fuzz/libsql_parser.a" "${LIBS[@]}" -o "$SRC/fuzz_$h"
  $CXX "${CXXSTD[@]}" "${INC[@]}" $SANITIZER_FLAGS $DEBUG_FLAGS \
    "$SRC/mayhem/fuzz_$h.cpp" "$OUT/standalone_main.o" "$OUT/lsan_off.o" "$OUT/fuzz/libsql_parser.a" "${LIBS[@]}" \
    -o "$SRC/fuzz_$h-standalone"
done

# 2) Known-answer self-test, normal flags (no sanitizers), for mayhem/test.sh.
build_lib "$OUT/test" "-O1 $COVERAGE_FLAGS"
for m in sql expr; do
  $CXX "${CXXSTD[@]}" "${INC[@]}" -O1 $COVERAGE_FLAGS -DSELFTEST_${m^^} "$SRC/mayhem/selftest/parser_selftest.cpp" \
    "$OUT/test/libsql_parser.a" "${LIBS[@]}" -o "$OUT/parser_selftest_$m"
done
echo "build.sh: OK"
