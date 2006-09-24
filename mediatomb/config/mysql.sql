-- MySQL dump 10.10
--
-- Host: localhost    Database: mediatomb
-- ------------------------------------------------------
-- Server version	5.0.22-Debian_0ubuntu6.06.2-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `cds_object`
--

DROP TABLE IF EXISTS `cds_object`;
CREATE TABLE `cds_object` (
  `id` int(11) NOT NULL auto_increment,
  `ref_id` int(11) default NULL,
  `parent_id` int(11) NOT NULL default '0',
  `object_type` tinyint(4) NOT NULL,
  `upnp_class` varchar(80) default NULL,
  `dc_title` varchar(255) default NULL,
  `is_restricted` tinyint(4) NOT NULL default '1',
  `location` varchar(255) default NULL,
  `location_hash` int(11) unsigned default NULL,
  `metadata` text,
  `auxdata` text,
  `resources` text,
  `update_id` int(11) NOT NULL default '0',
  `is_searchable` tinyint(4) NOT NULL default '0',
  `mime_type` varchar(40) default NULL,
  `state` varchar(255) default NULL,
  PRIMARY KEY  (`id`),
  KEY `cds_object_ref_id` (`ref_id`),
  KEY `cds_object_parent_id` (`parent_id`),
  KEY `location_parent` (`location_hash`,`parent_id`),
  CONSTRAINT `cds_object_ibfk_1` FOREIGN KEY (`ref_id`) REFERENCES `cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `cds_object_ibfk_2` FOREIGN KEY (`parent_id`) REFERENCES `cds_object` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Dumping data for table `cds_object`
--


/*!40000 ALTER TABLE `cds_object` DISABLE KEYS */;
LOCK TABLES `cds_object` WRITE;
INSERT INTO `cds_object` VALUES (-1,NULL,-1,0,NULL,NULL,1,NULL,NULL,NULL,NULL,NULL,0,0,NULL,NULL),(0,NULL,-1,1,'object.container','Root',1,NULL,NULL,NULL,NULL,NULL,0,0,NULL,NULL),(1,NULL,0,1,'object.container','PC Directory',1,NULL,NULL,NULL,NULL,NULL,0,0,NULL,NULL);
UNLOCK TABLES;
/*!40000 ALTER TABLE `cds_object` ENABLE KEYS */;

--
-- Table structure for table `mt_config`
--

DROP TABLE IF EXISTS `mt_config`;
CREATE TABLE `mt_config` (
  `key` varchar(40) NOT NULL,
  `value` varchar(255) NOT NULL,
  PRIMARY KEY  (`key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Dumping data for table `mt_config`
--


/*!40000 ALTER TABLE `mt_config` DISABLE KEYS */;
LOCK TABLES `mt_config` WRITE;
UNLOCK TABLES;
/*!40000 ALTER TABLE `mt_config` ENABLE KEYS */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

