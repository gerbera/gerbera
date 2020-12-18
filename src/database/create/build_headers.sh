#!/usr/bin/env bash

set -Eeuo pipefail
trap cleanup SIGINT SIGTERM ERR EXIT

cleanup() {
  trap - SIGINT SIGTERM ERR EXIT
  rm "$FN.Z"
}

readonly DB_TYPE="$1"
readonly ZPIPE_PATH="$2"
readonly DEST_DIR="$3"

case "$DB_TYPE" in
 "mysql"*)
  FN=mysql.sql
  DEF=MS_CREATE_SQL
  VARIABLE=mysql_create_sql
  FN_FINAL=mysql_create_sql.h
  ;;
 "sqlite"*)
  FN=sqlite3.sql
  DEF=SL3_CREATE_SQL
  VARIABLE=sqlite3_create_sql
  FN_FINAL=sqlite3_create_sql.h
  ;;
 *)
  echo 'Invalid DB Type'
  exit
  ;;
esac

# zpipe path
$ZPIPE_PATH< ${FN} > "${FN}.Z"

# Because this needs to work on BSD too
REAL_SIZE=$(wc -c < ${FN})
GZIPPED_SIZE=$(wc -c < ${FN}.Z)

{
  cat "${FN}.tmpl.h"
  echo "#define ${DEF}_INFLATED_SIZE ${REAL_SIZE}"
  echo "#define ${DEF}_DEFLATED_SIZE ${GZIPPED_SIZE}"
  echo "const unsigned char ${VARIABLE}[] = {"
  hexdump -ve '/1 "0x%02x,"' < ${FN}.Z
  echo "};"
  cat "${FN}.tmpl.f"
} > "${DEST_DIR}"/${FN_FINAL}
