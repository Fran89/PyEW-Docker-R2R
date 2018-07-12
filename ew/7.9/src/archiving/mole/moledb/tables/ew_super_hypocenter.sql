
DROP TABLE IF EXISTS `ew_super_hypocenter`;
CREATE TABLE `ew_super_hypocenter` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`             BIGINT    NOT NULL COMMENT 'Foreign key to ew_module',
    `hypo_id`               BIGINT    NOT NULL COMMENT 'Hypocenter sequence number from ew_module',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    UNIQUE (`hypo_id`, `fk_module`),
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'EW super hypocenter sequence numbers';

