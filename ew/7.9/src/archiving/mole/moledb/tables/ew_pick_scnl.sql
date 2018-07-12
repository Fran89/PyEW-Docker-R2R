
DROP TABLE IF EXISTS `ew_pick_scnl`;
CREATE TABLE `ew_pick_scnl` (
    `id`                    BIGINT   NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`               BIGINT   NOT NULL COMMENT 'Foreign key to ew_module',
    `fk_spkseq`               BIGINT   NOT NULL COMMENT 'Foreign key to ew_spkseq',
    `fk_scnl`                BIGINT   NOT NULL COMMENT 'Foreign key to ew_scnl',
    `dir`                   CHAR(1)           COMMENT 'First-motion descriptor (U,D,-space-,?) => pick->fm',
    `wt`                    TINYINT           COMMENT 'Pick weight or quality (0-4)',
    `tpick_dt`              DATETIME NOT NULL COMMENT 'Time of pick - datetime part',
    `tpick_usec`            INT      NOT NULL COMMENT 'Time of pick - useconds',
    `pamp_0`                BIGINT            COMMENT 'pick->pamp[0]',
    `pamp_1`                BIGINT            COMMENT 'pick->pamp[1]',    
    `pamp_2`                BIGINT            COMMENT 'pick->pamp[2]',
    `caav_0`                BIGINT DEFAULT NULL COMMENT 'coda->caav[0] coda avg abs value (counts) 2s window',
    `caav_1`                BIGINT DEFAULT NULL COMMENT 'coda->caav[1] coda avg abs value (counts) 2s window',
    `caav_2`                BIGINT DEFAULT NULL COMMENT 'coda->caav[2] coda avg abs value (counts) 2s window',
    `caav_3`                BIGINT DEFAULT NULL COMMENT 'coda->caav[3] coda avg abs value (counts) 2s window',
    `caav_4`                BIGINT DEFAULT NULL COMMENT 'coda->caav[4] coda avg abs value (counts) 2s window',
    `caav_5`                BIGINT DEFAULT NULL COMMENT 'coda->caav[5] coda avg abs value (counts) 2s window',
    `dur`                   BIGINT DEFAULT NULL COMMENT 'coda->dur coda duration (seconds since pick)',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`),
    FOREIGN KEY (`fk_module`)  REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_spkseq`) REFERENCES ew_spkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_scnl`)  REFERENCES ew_scnl(`id`)   ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`fk_module`, `fk_spkseq`),
    INDEX (`tpick_dt`),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Automatic pick_scnl messages from EW';

