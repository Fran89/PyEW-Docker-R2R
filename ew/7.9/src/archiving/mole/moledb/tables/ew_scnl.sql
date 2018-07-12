
DROP TABLE IF EXISTS `ew_scnl`;
CREATE TABLE `ew_scnl` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `sta`                   CHAR(5)   NOT NULL     COMMENT 'Station code',
    `cha`                   CHAR(3)   NOT NULL     COMMENT 'Channel/Component code',
    `net`                   CHAR(2)   NOT NULL     COMMENT 'Network code',
    `loc`                   CHAR(2)   NOT NULL     COMMENT 'Location code',
    `lat`                   DOUBLE    DEFAULT NULL COMMENT 'Latitude  (North=positive)',
    `lon`                   DOUBLE    DEFAULT NULL COMMENT 'Longitude (East=positive)',
    `ele`                   DOUBLE    DEFAULT NULL COMMENT 'Elevation',
    `name`                  CHAR(80)  DEFAULT NULL COMMENT 'Site name',
    `ext_id`                BIGINT    DEFAULT NULL COMMENT 'External table id',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    UNIQUE (`sta`, `cha`, `net`, `loc`),
    INDEX (`ext_id`),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'SCNL information';

