CREATE TABLE media_files 
(
    id              INTEGER PRIMARY KEY DEFAULT 0,
    ref_id          INTEGER,
    parent_id       INTEGER NOT NULL,
    object_type     INTEGER NOT NULL,
    upnp_class      VARCHAR(80),
    dc_title        VARCHAR(256),
    is_restricted   INTEGER NOT NULL DEFAULT 0,
    is_virtual      INTEGER NOT NULL DEFAULT 0,
    metadata        TEXT,

    update_id       INTEGER NOT NULL DEFAULT 0,
    is_searchable   INTEGER NOT NULL DEFAULT 0,

    location        VARCHAR(256),
    mime_type       VARCHAR(80),

    action          VARCHAR(256),
    state           VARCHAR(256)
);

CREATE INDEX media_files_ref_id ON media_files(ref_id);
CREATE INDEX media_files_parent_id ON media_files(parent_id);
CREATE INDEX media_files_object_type ON media_files(object_type);
CREATE INDEX media_files_is_virtual ON media_files(is_virtual);
CREATE INDEX media_files_mime_type ON media_files(mime_type);


INSERT INTO media_files(id, parent_id, object_type, dc_title, upnp_class) 
    VALUES (0, -1, 1, 'Root', 'object.container');

INSERT INTO media_files(id, parent_id, object_type, dc_title, upnp_class) 
    VALUES (1, 0, 1, 'PC-Directory', 'object.container');
