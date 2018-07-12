
DROP TABLE IF EXISTS `ew_spkseq`;
CREATE TABLE `ew_spkseq` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `pkseq`                 BIGINT    NOT NULL COMMENT 'Pick sequence number from picker',
    `fk_instance`                  BIGINT    NOT NULL COMMENT 'Id of the EW instance running',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    UNIQUE (`pkseq`, `fk_instance`),
    FOREIGN KEY (`fk_instance`) REFERENCES ew_instance(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'EW Super Pick Sequence';

