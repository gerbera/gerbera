/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
CREATE TABLE `mt_cds_object` (
  `id` int(11) NOT NULL auto_increment,
  `ref_id` int(11) default NULL,
  `parent_id` int(11) NOT NULL default '0',
  `object_type` tinyint(4) unsigned NOT NULL,
  `upnp_class` varchar(80) default NULL,
  `dc_title` varchar(255) default NULL,
  `location` blob,
  `location_hash` int(11) unsigned default NULL,
  `auxdata` blob,
  `update_id` int(11) NOT NULL default '0',
  `mime_type` varchar(40) default NULL,
  `flags` int(11) unsigned NOT NULL default '1',
  `part_number` int(11) default NULL,
  `track_number` int(11) default NULL,
  `service_id` varchar(255) default NULL,
  `bookmark_pos` int(11) unsigned NOT NULL default '0',
  `last_modified` bigint(20) unsigned default NULL,
  `last_updated` bigint(20) unsigned default '0',
  PRIMARY KEY  (`id`),
  KEY `cds_object_ref_id` (`ref_id`),
  KEY `cds_object_parent_id` (`parent_id`,`object_type`,`dc_title`),
  KEY `cds_object_object_type` (`object_type`),
  KEY `location_parent` (`location_hash`,`parent_id`),
  KEY `cds_object_track_number` (`part_number`,`track_number`),
  KEY `cds_object_service_id` (`service_id`),
  CONSTRAINT `mt_cds_object_ibfk_1` FOREIGN KEY (`ref_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `mt_cds_object_ibfk_2` FOREIGN KEY (`parent_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=MyISAM CHARSET=utf8;
INSERT INTO `mt_cds_object` (
    `id`, `parent_id`, `object_type`, `flags`
) VALUES (-1,-1,0,9);
INSERT INTO `mt_cds_object` (
    `id`, `parent_id`, `object_type`, `upnp_class`, `dc_title`, `flags`
) VALUES (0,-1,1,'object.container','Root',9);
UPDATE `mt_cds_object` SET `id`='0' WHERE `id`='1';
INSERT INTO `mt_cds_object` (
    `id`,  `parent_id`, `object_type`, `upnp_class`, `dc_title`, `flags`
) VALUES (1,0,1,'object.container','PC Directory',9);
CREATE TABLE `mt_internal_setting` (
  `key` varchar(40) NOT NULL,
  `value` varchar(255) NOT NULL,
  PRIMARY KEY  (`key`)
) ENGINE=MyISAM CHARSET=utf8;
CREATE TABLE `mt_autoscan` (
  `id` int(11) NOT NULL auto_increment,
  `obj_id` int(11) default NULL,
  `scan_level` enum('basic','full') NOT NULL,
  `scan_mode` enum('timed','inotify') NOT NULL,
  `recursive` tinyint(4) unsigned NOT NULL,
  `hidden` tinyint(4) unsigned NOT NULL,
  `interval` int(11) unsigned default NULL,
  `last_modified` bigint(20) unsigned default NULL,
  `persistent` tinyint(4) unsigned NOT NULL default '0',
  `location` blob,
  `path_ids` blob,
  `touched` tinyint(4) unsigned NOT NULL default '1',
  PRIMARY KEY `id` (`id`),
  UNIQUE KEY `mt_autoscan_obj_id` (`obj_id`),
  CONSTRAINT `mt_autoscan_ibfk_1` FOREIGN KEY (`obj_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=MyISAM CHARSET=utf8;
CREATE TABLE `mt_metadata` (
  `id` int(11) NOT NULL auto_increment,
  `item_id` int(11) NOT NULL,
  `property_name` varchar(255) NOT NULL,
  `property_value` text NOT NULL,
  PRIMARY KEY `id` (`id`),
  KEY `metadata_item_id` (`item_id`),
  CONSTRAINT `mt_metadata_idfk1` FOREIGN KEY (`item_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=MyISAM CHARSET=utf8;
CREATE TABLE `grb_config_value` (
  `item` varchar(255) primary key,
  `key` varchar(255) NOT NULL,
  `item_value` varchar(255) NOT NULL,
  `status` varchar(20) NOT NULL)
  ENGINE=MyISAM CHARSET=utf8;
CREATE INDEX grb_config_value_item ON grb_config_value(item);
CREATE TABLE `grb_cds_resource` (
    `id` int(11) primary key auto_increment,
    `item_id` int(11) NOT NULL,
    `res_id` int(11) NOT NULL,
    `handlerType` int(11) NOT NULL,
    `options` text default NULL,
    `parameters` text default NULL,
    CONSTRAINT `grb_cds_resource_fk` FOREIGN KEY (`item_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=MyISAM CHARSET=utf8;
INSERT INTO `mt_internal_setting` VALUES('resource_attribute', '');
CREATE INDEX `grb_cds_resource_id` ON `grb_cds_resource`(`item_id`,`res_id`);
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
