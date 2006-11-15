BEGIN TRANSACTION;
CREATE TABLE "mt_cds_object" (
  "id" integer primary key,
  "ref_id" integer default NULL,
  "parent_id" integer NOT NULL default '0',
  "object_type" tinyint unsigned NOT NULL,
  "upnp_class" varchar(80) default NULL,
  "dc_title" varchar(255) default NULL,
  "location" text default NULL,
  "location_hash" integer unsigned default NULL,
  "metadata" text default NULL,
  "auxdata" text default NULL,
  "resources" text default NULL,
  "update_id" integer NOT NULL default '0',
  "mime_type" varchar(40) default NULL,
  "flags" integer unsigned NOT NULL default '1',
  CONSTRAINT "cds_object_ibfk_1" FOREIGN KEY ("ref_id") REFERENCES "cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT "cds_object_ibfk_2" FOREIGN KEY ("parent_id") REFERENCES "cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);
INSERT INTO "mt_cds_object" VALUES(-1, NULL, -1, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, 1);
INSERT INTO "mt_cds_object" VALUES(0, NULL, -1, 1, 'object.container', 'Root', NULL, NULL, NULL, NULL, NULL, 0, NULL, 1);
INSERT INTO "mt_cds_object" VALUES(1, NULL, 0, 1, 'object.container', 'PC Directory', NULL, NULL, NULL, NULL, NULL, 0, NULL, 1);
CREATE TABLE "mt_cds_active_item" (
  "id" integer primary key,
  "action" varchar(255) NOT NULL,
  "state" varchar(255) NOT NULL,
  CONSTRAINT "mt_cds_active_item_ibfk_1" FOREIGN KEY ("id") REFERENCES "mt_cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE "mt_internal_setting" (
  "key" varchar(40) primary key NOT NULL,
  "value" varchar(255) NOT NULL
);
INSERT INTO "mt_internal_setting" VALUES('db_version', '1');
CREATE INDEX mt_cds_object_ref_id ON mt_cds_object(ref_id);
CREATE INDEX mt_cds_object_parent_id ON mt_cds_object(parent_id,object_type,dc_title);
CREATE INDEX mt_location_parent ON mt_cds_object(location_hash,parent_id);
CREATE INDEX mt_internal_setting_key ON mt_internal_setting(key);
COMMIT;
