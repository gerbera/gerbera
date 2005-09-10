CREATE TABLE cds_objects
(
    id              INTEGER PRIMARY KEY AUTO_INCREMENT,
    ref_id          INTEGER NOT NULL DEFAULT 0,
    parent_id       INTEGER NOT NULL DEFAULT 0,
    object_type     INTEGER NOT NULL,
    upnp_class      VARCHAR(80),
    dc_title        VARCHAR(255),
    is_restricted   INTEGER NOT NULL DEFAULT 0,
    is_virtual      INTEGER NOT NULL DEFAULT 0,
    location        VARCHAR(255),
    metadata        TEXT,
    auxdata         TEXT,
    resources       TEXT,

    update_id       INTEGER NOT NULL DEFAULT 0,
    is_searchable   INTEGER NOT NULL DEFAULT 0,

    mime_type       VARCHAR(40),

    action          VARCHAR(255),
    state           VARCHAR(255),

    flags           INTEGER NOT NULL DEFAULT 0,
    display_prio    INTEGER NOT NULL DEFAULT 0
);


CREATE INDEX cds_objects_ref_id ON cds_objects(ref_id);
CREATE INDEX cds_objects_parent_id ON cds_objects(parent_id);
CREATE INDEX cds_objects_object_type ON cds_objects(object_type);
CREATE INDEX cds_objects_is_virtual ON cds_objects(is_virtual);
CREATE INDEX cds_objects_mime_type ON cds_objects(mime_type);


INSERT INTO cds_objects(id, parent_id, object_type, dc_title, upnp_class) 
    VALUES (0, -1, 1, 'Root', 'object.container');
    
UPDATE cds_objects SET id = 0 WHERE id = 1;

INSERT INTO cds_objects(id, parent_id, object_type, dc_title, upnp_class) 
    VALUES (1, 0, 1, 'PC Directory', 'object.container');

