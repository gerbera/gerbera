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
  `dc_title` varchar(GRBMAX) default NULL COLLATE GRBCOLLATION,
  `sort_key` varchar(GRBMAX) default NULL COLLATE GRBCOLLATION,
  `location` blob,
  `location_hash` int(11) unsigned default NULL,
  `auxdata` blob,
  `update_id` int(11) NOT NULL default '0',
  `mime_type` varchar(40) default NULL,
  `flags` int(11) unsigned NOT NULL default '1',
  `part_number` int(11) default NULL,
  `track_number` int(11) default NULL,
  `source` int(11) NOT NULL default '0',
  `service_id` varchar(GRBMAX) default NULL,
  `last_modified` bigint(20) unsigned default NULL,
  `last_updated` bigint(20) unsigned default '0',
  PRIMARY KEY  (`id`),
  KEY `cds_object_ref_id` (`ref_id`),
  KEY `cds_object_parent_id` (`parent_id`,`object_type`,`dc_title`),
  KEY `cds_object_object_type` (`object_type`),
  KEY `location_parent` (`location_hash`,`parent_id`),
  KEY `cds_object_track_number` (`part_number`,`track_number`),
  KEY `cds_object_service_id` (`service_id`),
  KEY `cds_object_title` (`dc_title`),
  KEY `cds_object_sort_key` (`sort_key`),
  CONSTRAINT `mt_cds_object_ibfk_1` FOREIGN KEY (`ref_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `mt_cds_object_ibfk_2` FOREIGN KEY (`parent_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=GRBENGINE CHARSET=GRBCHARSET;
CREATE TABLE `mt_internal_setting` (
  `key` varchar(40) NOT NULL,
  `value` varchar(255) NOT NULL,
  PRIMARY KEY  (`key`)
) ENGINE=GRBENGINE CHARSET=GRBCHARSET;
CREATE TABLE `mt_autoscan` (
  `id` int(11) NOT NULL auto_increment,
  `obj_id` int(11) default NULL,
  `scan_mode` enum('timed','inotify','manual') NOT NULL,
  `recursive` tinyint(4) unsigned NOT NULL,
  `media_type` int(11) NOT NULL,
  `hidden` tinyint(4) unsigned NOT NULL,
  `follow_symlinks` tinyint(4) unsigned NOT NULL,
  `interval` int(11) unsigned default NULL,
  `ct_audio` varchar(255) NOT NULL,
  `ct_image` varchar(255) NOT NULL,
  `ct_video` varchar(255) NOT NULL,
  `last_modified` bigint(20) unsigned default NULL,
  `persistent` tinyint(4) unsigned NOT NULL default '0',
  `location` blob,
  `path_ids` blob,
  `touched` tinyint(4) unsigned NOT NULL default '1',
  `retry_count` int(11) unsigned NOT NULL default '0',
  `dir_types` tinyint(4) unsigned NOT NULL default '0',
  `force_rescan` tinyint(4) unsigned NOT NULL default '0',
  PRIMARY KEY `id` (`id`),
  UNIQUE KEY `mt_autoscan_obj_id` (`obj_id`),
  CONSTRAINT `mt_autoscan_ibfk_1` FOREIGN KEY (`obj_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=GRBENGINE CHARSET=GRBCHARSET;
CREATE TABLE `mt_metadata` (
  `id` int(11) NOT NULL auto_increment,
  `item_id` int(11) NOT NULL,
  `property_name` varchar(255) NOT NULL,
  `property_value` text NOT NULL,
  PRIMARY KEY `id` (`id`),
  KEY `metadata_item_id` (`item_id`),
  CONSTRAINT `mt_metadata_idfk1` FOREIGN KEY (`item_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=GRBENGINE CHARSET=GRBCHARSET;
CREATE TABLE `grb_config_value` (
  `item` varchar(255) primary key,
  `key` varchar(255) NOT NULL,
  `item_value` varchar(255) NOT NULL,
  `status` varchar(20) NOT NULL)
  ENGINE=GRBENGINE CHARSET=GRBCHARSET;
CREATE TABLE `grb_cds_resource` (
  `item_id` int(11) NOT NULL,
  `res_id` int(11) NOT NULL,
  `handlerType` int(11) NOT NULL,
  `purpose` int(11) NOT NULL,
  `options` text default NULL,
  `parameters` text default NULL,
  PRIMARY KEY (`item_id`, `res_id`),
  CONSTRAINT `grb_cds_resource_fk` FOREIGN KEY (`item_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=GRBENGINE CHARSET=GRBCHARSET;
CREATE TABLE `grb_client` (
  `addr` varchar(32) NOT NULL,
  `port` int(11) NOT NULL,
  `addrFamily` int(11) NOT NULL,
  `userAgent` varchar(255) NOT NULL,
  `last` int(11) NOT NULL,
  `age` int(11) NOT NULL,
  PRIMARY KEY (`addr`, `port`)
) ENGINE=GRBENGINE CHARSET=GRBCHARSET;
CREATE TABLE `grb_playstatus` (
  `group` varchar(GRBMAX) NOT NULL,
  `item_id` int(11) NOT NULL,
  `playCount` int(11) NOT NULL default '0',
  `lastPlayed` int(11) NOT NULL default '0',
  `lastPlayedPosition` int(11) NOT NULL default '0',
  `bookMarkPos` int(11) NOT NULL default '0',
  PRIMARY KEY (`group`, `item_id`),
  CONSTRAINT `grb_played_item` FOREIGN KEY (`item_id`) REFERENCES `mt_cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=GRBENGINE CHARSET=GRBCHARSET;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
