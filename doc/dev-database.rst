.. index:: Database Schema

Modify Database Schema
======================

- src/database/sql_database.h: Update ``DBVERSION``
- src/database/sqlite3/sqlite3.sql and src/database/mysql/mysql.sql: Modify ``CREATE TABLE`` statements
- src/database/sqlite3/sqlite3-upgrade.xml and src/database/mysql/mysql-upgrade.xml: Add schema update commands
- src/database/sqlite3/sqlite_database.cc and src/database/mysql/mysql_database.cc:
     - Add correct hashy to hashies list (run ``ctest`` to get error on wrong hash)
     - Update hashies[0] for the create scripts (run ``ctest`` to get error on wrong hash)
