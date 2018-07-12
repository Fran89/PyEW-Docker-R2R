
DROP TABLE IF EXISTS `ew_sqkseq`;
CREATE TABLE `ew_sqkseq` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `qkseq`                 BIGINT    NOT NULL COMMENT 'Quake sequence number from binder',
    `qknph`                 TINYINT   NOT NULL DEFAULT -1 COMMENT '-1 undefined, last nph associated phases by binder_ew. 0 means that event has been canceled',
    `fk_instance`           BIGINT    NOT NULL COMMENT 'Id of the EW instance running',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    UNIQUE (`qkseq`, `fk_instance`),
    FOREIGN KEY (`fk_instance`) REFERENCES ew_instance(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'EW Super Quake Sequence';

