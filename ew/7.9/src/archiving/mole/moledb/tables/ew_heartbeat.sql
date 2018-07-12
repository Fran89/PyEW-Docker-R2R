
DROP TABLE IF EXISTS `ew_heartbeat`;
CREATE TABLE `ew_heartbeat` (
    `id`                    BIGINT          NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`             BIGINT          NOT NULL COMMENT 'Foreign key to ew_module',
    `last_dt`               DATETIME        NOT NULL  COMMENT 'Last heartbeat time',
    `pid`                   BIGINT          DEFAULT NULL COMMENT 'Pid',
    `modified`              TIMESTAMP    DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',
    PRIMARY KEY (`id`),
    UNIQUE (`fk_module`),
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (last_dt),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Last Heartbeat messages from EW modules';

