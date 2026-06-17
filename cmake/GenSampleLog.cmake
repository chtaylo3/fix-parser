# Script-mode (cmake -P) helper: generate large sample FIX logs from the vendored
# fixsim corpus. Produces two artifacts:
#   - OUT_MULTILINE : ~MULTILINE_COUNT messages, one per line (debugger-friendly)
#   - OUT_SINGLELINE: a single physical line of ~SINGLELINE_MB MB (flagship stress)
#
# Required -D args: SRC44, SRC42, OUT_MULTILINE, OUT_SINGLELINE,
#                   MULTILINE_COUNT, SINGLELINE_MB.

file(READ "${SRC44}" _f44)
file(READ "${SRC42}" _f42)
string(STRIP "${_f44}" _f44)
string(STRIP "${_f42}" _f42)
string(REPLACE "\n" ";" _lines44 "${_f44}")
string(REPLACE "\n" ";" _lines42 "${_f42}")
set(_all ${_lines44} ${_lines42})
list(LENGTH _all _unit_count)  # messages per base unit (16)

# ---- Multiline: one message per line --------------------------------------
# Build a ~1600-message superchunk in memory, then append it to reach the count.
set(_super "")
foreach(_rep RANGE 1 100)
  foreach(_m IN LISTS _all)
    string(APPEND _super "${_m}\n")
  endforeach()
endforeach()
math(EXPR _super_msgs "${_unit_count} * 100")
math(EXPR _iters "(${MULTILINE_COUNT} + ${_super_msgs} - 1) / ${_super_msgs}")

file(WRITE "${OUT_MULTILINE}" "")
foreach(_i RANGE 1 ${_iters})
  file(APPEND "${OUT_MULTILINE}" "${_super}")
endforeach()
message(STATUS "Wrote ${OUT_MULTILINE} (~${MULTILINE_COUNT} messages)")

# ---- Single line: messages pipe-joined, no newlines -----------------------
# Build a superchunk of pipe-joined messages, then append until target size.
string(REPLACE ";" "|" _unit "${_all}")   # 16 messages joined by '|'
set(_superline "")
foreach(_rep RANGE 1 100)
  string(APPEND _superline "${_unit}|")
endforeach()
string(LENGTH "${_superline}" _super_bytes)
math(EXPR _target_bytes "${SINGLELINE_MB} * 1024 * 1024")
math(EXPR _iters2 "(${_target_bytes} + ${_super_bytes} - 1) / ${_super_bytes}")

file(WRITE "${OUT_SINGLELINE}" "")
foreach(_i RANGE 1 ${_iters2})
  file(APPEND "${OUT_SINGLELINE}" "${_superline}")
endforeach()
message(STATUS "Wrote ${OUT_SINGLELINE} (~${SINGLELINE_MB} MB single line)")
