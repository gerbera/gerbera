BEGIN TRANSACTION;
CREATE TABLE "mt_cds_object" (
  "id" integer primary key,
  "ref_id" integer default NULL,
  "parent_id" integer NOT NULL default 0,
  "object_type" tinyint unsigned NOT NULL,
  "upnp_class" varchar(80) default NULL,
  "dc_title" varchar(255) default NULL,
  "location" text default NULL,
  "location_hash" integer unsigned default NULL,
  "auxdata" text default NULL,
  "update_id" integer NOT NULL default 0,
  "mime_type" varchar(40) default NULL,
  "flags" integer unsigned NOT NULL default 1,
  "part_number" integer default NULL,
  "track_number" integer default NULL,
  "service_id" varchar(255) default NULL,
  "bookmark_pos" integer unsigned NOT NULL default 0,
  "last_modified" integer unsigned default NULL,
  "last_updated" integer unsigned default 0,
  CONSTRAINT "cds_object_ibfk_1" FOREIGN KEY ("ref_id") REFERENCES "mt_cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT "cds_object_ibfk_2" FOREIGN KEY ("parent_id") REFERENCES "mt_cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);
INSERT INTO "mt_cds_object" (
    id, ref_id, parent_id, object_type, upnp_class, dc_title, flags
) VALUES (-1, NULL, -1, 0, NULL, NULL, 9);
INSERT INTO "mt_cds_object" (
    id, ref_id, parent_id, object_type, upnp_class, dc_title, flags
) VALUES (0, NULL, -1, 1, 'object.container', 'Root', 9);
INSERT INTO "mt_cds_object" (
    id, ref_id, parent_id, object_type, upnp_class, dc_title, flags
) VALUES (1, NULL, 0, 1, 'object.container', 'PC Directory', 9);
CREATE TABLE "mt_internal_setting" (
  "key" varchar(40) primary key NOT NULL,
  "value" varchar(255) NOT NULL
);
CREATE TABLE "mt_autoscan" (
  "id" integer primary key,
  "obj_id" integer default NULL,
  "scan_level" varchar(10) NOT NULL,
  "scan_mode" varchar(10) NOT NULL,
  "recursive" tinyint unsigned NOT NULL,
  "hidden" tinyint unsigned NOT NULL,
  "interval" integer unsigned default NULL,
  "last_modified" integer unsigned default NULL,
  "persistent" tinyint unsigned NOT NULL default '0',
  "location" text default NULL,
  "path_ids" text default NULL,
  "touched" tinyint unsigned NOT NULL default '1',
  CONSTRAINT "mt_autoscan_id" FOREIGN KEY ("obj_id") REFERENCES "mt_cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE "mt_metadata" (
  "id" integer primary key,
  "item_id" integer NOT NULL,
  "property_name" varchar(255) NOT NULL,
  "property_value" text NOT NULL,
  CONSTRAINT "mt_metadata_idfk1" FOREIGN KEY ("item_id") REFERENCES "mt_cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE TABLE "grb_config_value" (
  "item" varchar(255) primary key,
  "key" varchar(255) NOT NULL,
  "item_value" varchar(255) NOT NULL,
  "status" varchar(20) NOT NULL
);
CREATE TABLE "grb_cds_resource" (
    "id" integer primary key,
    "item_id" integer NOT NULL,
    "res_id" integer NOT NULL,
    "handlerType" integer NOT NULL,
    "options" text default NULL,
    "parameters" text default NULL,
    CONSTRAINT "grb_cds_resource_fk" FOREIGN KEY ("item_id") REFERENCES "mt_cds_object" ("id") ON DELETE CASCADE ON UPDATE CASCADE
);
INSERT INTO "mt_internal_setting" VALUES('resource_attribute', '');
CREATE INDEX grb_cds_resource_id ON grb_cds_resource(item_id,res_id);
CREATE INDEX mt_cds_object_ref_id ON mt_cds_object(ref_id);
CREATE INDEX mt_cds_object_parent_id ON mt_cds_object(parent_id,object_type,dc_title);
CREATE INDEX mt_object_type ON mt_cds_object(object_type);
CREATE INDEX mt_location_parent ON mt_cds_object(location_hash,parent_id);
CREATE INDEX grb_track_number ON mt_cds_object(part_number,track_number);
CREATE INDEX mt_internal_setting_key ON mt_internal_setting(key);
CREATE UNIQUE INDEX mt_autoscan_obj_id ON mt_autoscan(obj_id);
CREATE INDEX mt_cds_object_service_id ON mt_cds_object(service_id);
CREATE INDEX mt_metadata_item_id ON mt_metadata(item_id);
CREATE INDEX grb_config_value_item ON grb_config_value(item);
COMMIT;
