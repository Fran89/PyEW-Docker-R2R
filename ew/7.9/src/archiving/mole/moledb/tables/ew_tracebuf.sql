
DROP TABLE IF EXISTS `ew_tracebuf`;
CREATE TABLE `ew_tracebuf` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`               BIGINT    NOT NULL COMMENT 'Foreign key to ew_module',
    `pinno`                 BIGINT             COMMENT 'Pin number',
    `nsamp`                 BIGINT    NOT NULL COMMENT 'Number of samples in packet',
    `starttime_dt`          DATETIME  NOT NULL COMMENT 'datetime part of time of first sample in epoch seconds',
    `starttime_usec`        INT       NOT NULL COMMENT 'useconds part of time of first sample in epoch seconds',
    `endtime_dt`            DATETIME  NOT NULL COMMENT 'datetime part of time of last sample in epoch seconds',
    `endtime_usec`          INT       NOT NULL COMMENT 'useconds part of time of last sample in epoch seconds',
    `samprate`              DOUBLE    NOT NULL COMMENT 'Sample rate; nominal',
    `fk_scnl`                BIGINT    NOT NULL COMMENT 'Foreign key to ew_scnl',
    `version`               CHAR(10)           COMMENT 'numeric version fields (99-99)',
    `datatype`              CHAR(3)   NOT NULL COMMENT 'Data format code',
    `quality`               CHAR(10)           COMMENT 'Data-quality fields (99-99)',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`)   ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (starttime_dt),
    INDEX (endtime_dt),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Tracebuf messages from EW';

