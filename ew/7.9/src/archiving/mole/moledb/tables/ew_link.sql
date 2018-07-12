
DROP TABLE IF EXISTS `ew_link`;
CREATE TABLE `ew_link` (
    `id`                    BIGINT NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`               BIGINT NOT NULL COMMENT 'Foreign key to ew_module',
    `fk_sqkseq`               BIGINT NOT NULL COMMENT 'Foreign key to ew_sqkseq',
    `fk_spkseq`               BIGINT NOT NULL COMMENT 'Foreign key to ew_spkseq',
    `iphs`                  INT             COMMENT 'Phase number',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',            
    PRIMARY KEY (`id`),
    FOREIGN KEY (`fk_module`)  REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_spkseq`) REFERENCES ew_spkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`fk_module`, `fk_sqkseq`, `fk_spkseq`),
    INDEX (`fk_module`, `fk_sqkseq`),
    INDEX (`fk_module`, `fk_spkseq`),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Automatic link messages from EW';

