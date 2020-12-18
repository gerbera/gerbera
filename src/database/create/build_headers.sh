#!/usr/bin/env bash

set -Eeuo pipefail
trap cleanup SIGINT SIGTERM ERR EXIT

cleanup() {
  trap - SIGINT SIGTERM ERR EXIT
  rm "$FN.Z"
  rm "$FN.Z.c"
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

echo "#define ${DEF}_INFLATED_SIZE ${REAL_SIZE}" > "${FN}.Z.c"
echo "#define ${DEF}_DEFLATED_SIZE ${GZIPPED_SIZE}" >> "${FN}.Z.c"

./bin2hex.pl ${FN}.Z 1 | sed "s/char bin_data/const unsigned char ${VARIABLE}/" >> "${FN}.Z.c"

cat "${FN}.tmpl.h" "${FN}.Z.c" "${FN}.tmpl.f" > "${DEST_DIR}"/${FN_FINAL}
