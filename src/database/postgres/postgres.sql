CREATE TABLE "mt_cds_object"(
    "id" serial NOT NULL PRIMARY KEY,
    "ref_id" integer default NULL,
    "parent_id" integer NOT NULL default 0,
    "object_type" smallint NOT NULL,
    "upnp_class" varchar(80) default NULL,
    "dc_title" varchar(GRBMAX) default NULL,
    "sort_key" varchar(GRBMAX) default NULL,
    "location" text,
    "location_hash" bigint default NULL,
    "auxdata" text,
    "update_id" integer NOT NULL default 0,
    "mime_type" varchar(40) default NULL,
    "flags" integer NOT NULL default 1,
    "part_number" integer default NULL,
    "track_number" integer default NULL,
    "source" integer NOT NULL default 0,
    "entry_type" integer NOT NULL default -1,
    "service_id" varchar(GRBMAX) default NULL,
    "last_modified" bigint default NULL,
    "last_updated" bigint default 0,
    CONSTRAINT "mt_cds_object_ibfk_1" FOREIGN KEY("ref_id") REFERENCES "mt_cds_object"("id")ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT "mt_cds_object_ibfk_2" FOREIGN KEY("parent_id") REFERENCES "mt_cds_object"("id")ON DELETE CASCADE ON UPDATE CASCADE);

ALTER SEQUENCE mt_cds_object_id_seq RESTART WITH 2;

CREATE INDEX "cds_object_ref_id" ON "mt_cds_object"("ref_id");
CREATE INDEX "cds_object_parent_id" ON "mt_cds_object"("parent_id", "object_type", "dc_title");
CREATE INDEX "cds_object_object_type" ON "mt_cds_object"("object_type");
CREATE INDEX "location_parent" ON "mt_cds_object"("location_hash", "parent_id");
CREATE INDEX "cds_object_track_number" ON "mt_cds_object"("part_number", "track_number");
CREATE INDEX "cds_object_service_id" ON "mt_cds_object"("service_id");
CREATE INDEX "cds_object_title" ON "mt_cds_object"("dc_title");
CREATE INDEX "cds_object_sort_key" ON "mt_cds_object"("sort_key");

CREATE TABLE "mt_internal_setting"(
    "key" varchar(40) NOT NULL PRIMARY KEY,
    "value" varchar(255) NOT NULL);

CREATE TABLE mt_autoscan(
    "id" SERIAL PRIMARY KEY,
    "obj_id" INTEGER,
    "scan_mode" VARCHAR(16) NOT NULL CHECK(scan_mode IN('timed', 'inotify', 'manual')),
    "recursive" SMALLINT NOT NULL,
    "media_type" INTEGER NOT NULL,
    "hidden" SMALLINT NOT NULL,
    "follow_symlinks" SMALLINT NOT NULL,
    "interval" INTEGER,
    "ct_audio" VARCHAR(255) NOT NULL,
    "ct_image" VARCHAR(255) NOT NULL,
    "ct_video" VARCHAR(255) NOT NULL,
    "last_modified" BIGINT,
    "persistent" SMALLINT NOT NULL DEFAULT 0,
    "location" TEXT,
    "path_ids" TEXT,
    "touched" SMALLINT NOT NULL DEFAULT 1,
    "retry_count" INTEGER NOT NULL DEFAULT 0,
    "dir_types" SMALLINT NOT NULL DEFAULT 0,
    "force_rescan" SMALLINT NOT NULL DEFAULT 0,
    CONSTRAINT "mt_autoscan_obj_id" UNIQUE("obj_id"),
    CONSTRAINT "mt_autoscan_ibfk_1" FOREIGN KEY("obj_id") REFERENCES "mt_cds_object"("id")ON DELETE CASCADE ON UPDATE CASCADE);

CREATE TABLE mt_metadata(
    "id" SERIAL PRIMARY KEY,
    "item_id" INTEGER NOT NULL,
    "property_name" VARCHAR(255) NOT NULL,
    "property_value" TEXT NOT NULL,
    CONSTRAINT "mt_metadata_idfk1" FOREIGN KEY("item_id") REFERENCES "mt_cds_object"("id")ON DELETE CASCADE ON UPDATE CASCADE);
CREATE INDEX "metadata_item_id" ON "mt_metadata"("item_id");

CREATE TABLE "grb_config_value"(
    "item" varchar(255) PRIMARY KEY,
    "key" varchar(255) NOT NULL,
    "item_value" text NOT NULL,
    "status" varchar(20) NOT NULL);

CREATE TABLE "grb_cds_resource"(
    "item_id" integer NOT NULL,
    "res_id" integer NOT NULL,
    "handlerType" integer NOT NULL,
    "purpose" integer NOT NULL,
    "options" text default NULL,
    "parameters" text default NULL,
    PRIMARY KEY("item_id", "res_id"),
    CONSTRAINT "grb_cds_resource_fk" FOREIGN KEY("item_id") REFERENCES "mt_cds_object"("id")ON DELETE CASCADE ON UPDATE CASCADE);

CREATE TABLE "grb_client"(
    "addr" varchar(32) NOT NULL,
    "port" integer NOT NULL,
    "addrFamily" integer NOT NULL,
    "userAgent" text NOT NULL,
    "last" integer NOT NULL,
    "age" integer NOT NULL,
    PRIMARY KEY("addr", "port"));

CREATE TABLE "grb_playstatus"(
    "group" varchar(GRBMAX) NOT NULL,
    "item_id" integer NOT NULL,
    "playCount" integer NOT NULL default 0,
    "lastPlayed" integer NOT NULL default 0,
    "lastPlayedPosition" integer NOT NULL default 0,
    "bookMarkPos" integer NOT NULL default 0,
    PRIMARY KEY("group", "item_id"),
    CONSTRAINT "grb_played_item" FOREIGN KEY("item_id") REFERENCES "mt_cds_object"("id")ON DELETE CASCADE ON UPDATE CASCADE);
