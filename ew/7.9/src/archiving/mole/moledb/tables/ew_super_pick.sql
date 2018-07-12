
DROP TABLE IF EXISTS `ew_super_pick`;
CREATE TABLE `ew_super_pick` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`             BIGINT    NOT NULL COMMENT 'Foreign key to ew_module',
    `pick_id`               BIGINT    NOT NULL COMMENT 'Pick sequence number from ew_module',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    UNIQUE (`pick_id`, `fk_module`),
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'EW super pick sequence numbers';

