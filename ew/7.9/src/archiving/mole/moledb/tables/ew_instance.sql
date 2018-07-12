
DROP TABLE IF EXISTS `ew_instance`;
CREATE TABLE `ew_instance` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `ewname`                CHAR(80)  NOT NULL COMMENT 'Name of the EW instance running',
    `hostname`              CHAR(80)  DEFAULT NULL COMMENT 'Hostname where the instance is running',
    `ewinstallation`        CHAR(80)  DEFAULT NULL COMMENT 'EW installation of the instance (i.e. INST_INGV)',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    UNIQUE (`ewname` ),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'EW instance name';

