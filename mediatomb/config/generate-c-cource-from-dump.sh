#!/bin/sh

case "$1" in 
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
 "pgsql"*)
  FN=pgsql.sql
  DEF=PG_CREATE_SQL
  VARIABLE=pgsql_create_sql
  FN_FINAL=pgsql_create_sql.h
  ;;
 *)
  echo 'illegal parameter'
  exit
  ;;
esac

./zpipe < $FN > ${FN}.Z

REAL_SIZE=`du -b --apparent-size $FN | awk '{print $1}'`
GZIPPED_SIZE=`du -b --apparent-size $FN.Z | awk '{print $1}`

FN_Z_C=${FN}.Z.c

echo "#define ${DEF}_INFLATED_SIZE $REAL_SIZE" > $FN_Z_C
echo "#define ${DEF}_DEFLATED_SIZE $GZIPPED_SIZE" >> $FN_Z_C

./bin2hex.pl ${FN}.Z 1 | sed "s/char bin_data/const unsigned char ${VARIABLE}/" >> $FN_Z_C 

cat ${FN}.tmpl.h $FN_Z_C ${FN}.tmpl.f > ${FN_FINAL}
