#!/usr/bin/env bash
# mayhem/test.sh — known-answer test of infinity's SQL and expression parsers.
# Runs the parser_selftest built by mayhem/build.sh (normal flags) over
# mayhem/selftest/{sql,expr}_cases.txt and compares each output line with the golden
# mayhem/selftest/{sql,expr}_expected.txt (statement types / expression ToString / ERR).
# One CTRF test per case line.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

BIN="$SRC/build-mayhem/parser_selftest"   # _sql / _expr
D="$SRC/mayhem/selftest"
if [ ! -x "${BIN}_sql" ] || [ ! -x "${BIN}_expr" ]; then
  echo "test.sh: $BIN missing — run mayhem/build.sh first" >&2
  emit_ctrf parser-selftest 0 1
  exit 1
fi

passed=0; failed=0
for mode in sql expr; do
  expected="$D/${mode}_expected.txt"
  actual="$(mktemp)"
  "${BIN}_$mode" "$mode" "$D/${mode}_cases.txt" > "$actual" 2>/dev/null || echo "parser_selftest $mode exited $?" >&2
  n=$(wc -l < "$expected")
  for i in $(seq 1 "$n"); do
    e=$(sed -n "${i}p" "$expected"); a=$(sed -n "${i}p" "$actual")
    if [ "$e" == "$a" ]; then passed=$((passed+1)); else
      failed=$((failed+1)); echo "FAIL $mode case $i: expected [$e] got [$a]" >&2; fi
  done
  rm -f "$actual"
done
emit_ctrf parser-selftest "$passed" "$failed"
