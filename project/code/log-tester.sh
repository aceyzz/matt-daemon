#!/usr/bin/env bash
LOG_DIR="/var/log/matt_daemon"
LOG_FILE="matt_daemon.log"
LOG_PATH="${LOG_DIR}/${LOG_FILE}"
LOCK_PATH="/var/lock/matt_daemon.lock"
MAX_SIZE=1048576
RETAIN=5
HOST="127.0.0.1"
PORT=4242
TMP_DIR="$(mktemp -d)"
PAY_GEANT="${TMP_DIR}/payload_mega.txt"

pass() { printf "PASS\n"; }
fail() { printf "FAIL\n"; }
sep() { printf "%s\n" "────────────────────────────────────────────────────────"; }

have_nc() { command -v nc >/dev/null 2>&1; }

count_backups() {
  ls -1 "${LOG_DIR}/${LOG_FILE}."* 2>/dev/null | wc -l | awk '{print $1}'
}

list_backups() {
  ls -1 "${LOG_DIR}/${LOG_FILE}."* 2>/dev/null | sed 's#.*/##' | sort
}

size_of() {
  [ -f "$1" ] && stat -c %s "$1" 2>/dev/null || echo 0
}

send_linefile() {
  nc -w 2 "$HOST" "$PORT" < "$1" >/dev/null 2>&1
}

printf "%s\n" "MATT_DAEMON — ADVANCED LOGS TESTER"
sep
printf "PID: " && if [ -f "$LOCK_PATH" ]; then tr -d '\n' < "$LOCK_PATH"; echo; else echo "unknown"; fi
printf "LOG_DIR: %s\n" "$LOG_DIR"
printf "LOG_FILE: %s\n" "$LOG_FILE"
printf "MAX_SIZE: %d\n" "$MAX_SIZE"
printf "BACKUP_COUNT(limit): %d\n" "$RETAIN"
sep

printf "CHECK: nc present... "
if have_nc; then pass; else fail; echo "nc manquant"; exit 1; fi

printf "CHECK: daemon reachable on %s:%s... " "$HOST" "$PORT"
echo "ping" | nc -w 2 "$HOST" "$PORT" >/dev/null 2>&1 && pass || { fail; echo "port injoignable"; exit 1; }

printf "CHECK: log path exists... "
[ -f "$LOG_PATH" ] && pass || { fail; echo "fichier log introuvable: $LOG_PATH"; exit 1; }

BEFORE_COUNT=$(count_backups)
printf "STATE: backups before: %d\n" "$BEFORE_COUNT"
printf "STATE: list before: "
if [ "$BEFORE_COUNT" -gt 0 ]; then echo; list_backups; else echo "none"; fi
sep

printf "STEP: build giant line payload (~MAX_SIZE+8k)... "
dd if=/dev/zero bs=$((MAX_SIZE+8192)) count=1 2>/dev/null | tr '\0' 'A' > "$PAY_GEANT"
printf "\n" >> "$PAY_GEANT"
[ -s "$PAY_GEANT" ] && pass || { fail; echo "payload creation failed"; exit 1; }

printf "TEST: trigger rotation by sending one giant line... "
send_linefile "$PAY_GEANT"
sleep 1
AFTER_COUNT=$(count_backups)
echo
printf "RESULT: before=%d after=%d " "$BEFORE_COUNT" "$AFTER_COUNT"
if [ "$AFTER_COUNT" -gt "$BEFORE_COUNT" ]; then pass; else fail; fi
sep

printf "TEST: enforce retention with multiple rotations... "
ROT=0
TARGET=$((RETAIN + 3))
while [ $ROT -lt $TARGET ]; do
  send_linefile "$PAY_GEANT"
  ROT=$((ROT+1))
done
sleep 1
CUR_COUNT=$(count_backups)
printf "\nRESULT: count=%d limit=%d " "$CUR_COUNT" "$RETAIN"
if [ "$CUR_COUNT" -le "$RETAIN" ]; then pass; else fail; fi
sep

printf "TEST: backups are non-empty... "
OK=1
for b in $(list_backups); do
  SZ=$(size_of "${LOG_DIR}/${b}")
  if [ "$SZ" -le 0 ]; then OK=0; break; fi
done
[ "$OK" -eq 1 ] && pass || fail
sep

printf "TEST: current log is writable post-rotation... "
CUR_SZ=$(size_of "$LOG_PATH")
echo "probe" | nc -w 2 "$HOST" "$PORT" >/dev/null 2>&1
sleep 1
NEW_SZ=$(size_of "$LOG_PATH")
printf "\nRESULT: size_before=%d size_after=%d " "$CUR_SZ" "$NEW_SZ"
if [ "$NEW_SZ" -gt "$CUR_SZ" ]; then pass; else fail; fi
sep

printf "SUMMARY\n"
printf "Backups present: %d\n" "$(count_backups)"
printf "Backups list:\n"
list_backups || true
printf "Done.\n"

rm -f "$PAY_GEANT"
rmdir "$TMP_DIR" 2>/dev/null || true
