.. index:: Database Schema

Database Schema
===============

The gerbera database contains 8 tables.

- 3 core tables store details of the media (audio, video and images) and associated items (like subtitles or album art images)

  - ``mt_cds_object``: for media items or directories
  - ``mt_metadata``: metadata like artist name or track number and
  - ``grb_cds_resource``: resource details like bitrate or image size

- Table ``mt_autoscan`` contains data on autoscan directories.
- Table ``grb_playstatus`` contains statistics on played media items.
- Table ``grb_client`` stores details on connected clients.
- Tables ``mt_internal_setting`` and ``grb_config_value`` store settings (like database version) and configuration values changed via UI.

.. image:: _static/gerbera-db.png

Modify Schema
-------------

- Update ``DBVERSION``

  - `src/database/sql_database.h`

- Modify ``CREATE TABLE`` statements

  - `src/database/sqlite3/sqlite3.sql`
  - `src/database/postgres/postgres.sql`
  - `src/database/mysql/mysql.sql`

- Add schema update commands

  - `src/database/sqlite3/sqlite3-upgrade.xml`
  - `src/database/postgres/postgres-upgrade.xml`
  - `src/database/mysql/mysql-upgrade.xml`

- Add drop schema commands

  - `src/database/sqlite3/sqlite3-drop.sql`
  - `src/database/postgres/postgres-drop.sql`
  - `src/database/mysql/mysql-drop.sql`

- Implement complex Database Migration

  - `src/database/sql_migration.h`
  - `src/database/sql_migration.cc`

- Hash codes to avoid script manipulations

  - run ``ctest`` to get error on wrong hash for
  - `src/database/sqlite3/sqlite_database.cc`
  - `src/database/postgres/postgres_database.cc`
  - `src/database/mysql/mysql_database.cc`

     - Add correct hashy to hashies list
     - Update ``hashies[0]`` for the create scripts
     - Update ``hashies[DBVERSION]`` for the drop scripts
