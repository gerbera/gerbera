BEGIN TRANSACTION;
CREATE TABLE "mt_cds_object" (
  "id" integer primary key,
  "ref_id" integer default NULL,
  "parent_id" integer NOT NULL default '0',
  "object_type" tinyint NOT NULL,
  "upnp_class" varchar(80) default NULL,
  "dc_title" varchar(255) default NULL,
  "is_restricted" tinyint NOT NULL default '1',
  "location" varchar(255) default NULL,
  "location_hash" integer unsigned default NULL,
  "metadata" text,
  "auxdata" text,
  "resources" text,
  "update_id" integer NOT NULL default '0',
  "is_searchable" tinyint NOT NULL default '0',
  "mime_type" varchar(40) default NULL,
  "state" varchar(255) default NULL,
  CONSTRAINT "cds_object_ibfk_1" FOREIGN KEY ("ref_id") REFERENCES "cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT "cds_object_ibfk_2" FOREIGN KEY ("parent_id") REFERENCES "cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);
INSERT INTO "mt_cds_object" VALUES(-1, NULL, -1, 0, NULL, NULL, 1, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL);
INSERT INTO "mt_cds_object" VALUES(0, NULL, -1, 1, 'object.container', 'Root', 1, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL);
INSERT INTO "mt_cds_object" VALUES(1, NULL, 0, 1, 'object.container', 'PC Directory', 1, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL, NULL);
CREATE TABLE "mt_internal_setting" (
  "key" varchar(40) primary key NOT NULL,
  "value" varchar(255) NOT NULL
);
CREATE INDEX mt_cds_object_ref_id ON mt_cds_object(ref_id);
CREATE INDEX mt_cds_object_parent_id ON mt_cds_object(parent_id);
CREATE INDEX mt_location_parent ON mt_cds_object(location_hash,parent_id);
CREATE INDEX mt_internal_setting_key ON mt_internal_setting(key);
COMMIT;
