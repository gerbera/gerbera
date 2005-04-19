CREATE TABLE media_files 
(
    id              INTEGER PRIMARY KEY DEFAULT 0,
    parent_id       INTEGER NOT NULL,
    upnp_class      VARCHAR(80),
    dc_title        VARCHAR(256),
    dc_description  TEXT,
    restricted      INTEGER NOT NULL DEFAULT 0,

    update_id       INTEGER NOT NULL DEFAULT 0,
    searchable      INTEGER NOT NULL DEFAULT 0,

    is_virtual      INTEGER NOT NULL DEFAULT 0,

    location        VARCHAR(256),
    mime_type       VARCHAR(80),

    action          VARCHAR(256),
    state           VARCHAR(256),

    object_type     INTEGER NOT NULL
);

CREATE INDEX media_files_parent_id ON media_files(parent_id);
CREATE INDEX media_files_object_type ON media_files(object_type);

INSERT INTO media_files(id, parent_id, object_type, dc_title, upnp_class) 
    VALUES (0, -1, 1, 'Root', 'object.container');

