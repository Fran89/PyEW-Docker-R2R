
DROP TABLE IF EXISTS `ew_error`;
CREATE TABLE `ew_error` (
    `id`                    BIGINT          NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`               BIGINT          NOT NULL COMMENT 'Foreign key to ew_module',
    `time_dt`               DATETIME        NOT NULL  COMMENT 'datetime part of time of first sample in epoch seconds',
    `message`               VARCHAR(256) DEFAULT NULL COMMENT 'Error message',
    `modified`              TIMESTAMP    DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',
    PRIMARY KEY (`id`),
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (time_dt),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Error messages from EW';

