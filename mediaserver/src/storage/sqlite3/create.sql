
CREATE TABLE media_files
(
    id              INTEGER PRIMARY KEY DEFAULT 0,
    parent_id       INTEGER NOT NULL,
    upnp_class      VARCHAR(80),
    dc_title        VARCHAR(256),
    restricted      INTEGER NOT NULL DEFAULT 0,
    location        VARCHAR(256),
    
    update_id       INTEGER NOT NULL DEFAULT 0,
    searchable      INTEGER NOT NULL DEFAULT 0,

    mime_type       VARCHAR(80),

    action          VARCHAR(256),
    state           VARCHAR(256),

    object_type     INTEGER NOT NULL
);

CREATE INDEX media_files_parent_id ON media_files(parent_id);
CREATE INDEX media_files_object_type ON media_files(object_type);

INSERT INTO media_files(id, parent_id, object_type, dc_title, upnp_class)
    VALUES (0, -1, 1, 'Root', 'object.container');

INSERT INTO media_files(id, parent_id, object_type, dc_title, upnp_class)
    VALUES (100, 0, 1, 'Music', 'object.container');
    
INSERT INTO media_files(id, parent_id, object_type, dc_title, location, upnp_class, mime_type)
    VALUES (101, 100, 2, 'Hammer smashed face', '/Music/Hsf.mp3', 'object.item', 'audio/mpeg');
INSERT INTO media_files(id, parent_id, object_type, dc_title, location, upnp_class, mime_type)
    VALUES (102, 100, 2, 'Addicted to vaginal skin', '/Music/atvs.mp3', 'object.item',
    'audio/mpeg');


INSERT INTO media_files(id, parent_id, object_type, dc_title, upnp_class)
    VALUES (200, 0, 1, 'Video', 'object.container');

INSERT INTO media_files(id, parent_id, object_type, dc_title, upnp_class)
    VALUES (300, 0, 1, 'Toggles n stuff', 'object.container');

INSERT INTO media_files(id, parent_id, object_type, dc_title, location, upnp_class, mime_type,
    action, state)
    VALUES (301, 300, 6, 'Demo toggle', '/Music/fuck.mp3', 'object.item', 'audio/mpeg',
    '/tmp/toggle', '');
    
INSERT INTO media_files(id, parent_id, object_type, dc_title, location, upnp_class, mime_type,
    action, state)
    VALUES (302, 300, 6, 'Demo HTTP toggle', '/Music/suck.mp3', 'object.item', 'audio/mpeg',
    'http://localhost/upnp/toggle.php', '');
