
DROP TABLE IF EXISTS `ew_quake2k`;
CREATE TABLE `ew_quake2k` (
    `id`                    BIGINT   NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`               BIGINT   NOT NULL COMMENT 'Foreign key to ew_module',
    `fk_sqkseq`               BIGINT   NOT NULL COMMENT 'Foreign key to ew_sqkseq',
    `quake_ot_dt`           DATETIME NOT NULL COMMENT 'Quake Origin Time - datetime part',
    `quake_ot_usec`         INT      NOT NULL COMMENT 'Quake Origin Time - useconds',   
    `lat`                   DOUBLE            COMMENT 'Latitude',
    `lon`                   DOUBLE            COMMENT 'Longitude',
    `depth`                 DOUBLE            COMMENT 'Depht => QUAKE2K->z',
    `rms`                   DOUBLE            COMMENT 'Rms Residual',
    `dmin`                  DOUBLE            COMMENT 'Distance to closest station',
    `ravg`                  DOUBLE            COMMENT 'Average or median station distance',
    `gap`                   DOUBLE            COMMENT 'Largest azimuth without picks',
    `nph`                   INT               COMMENT 'Linked phases',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',            
    PRIMARY KEY (`id`),
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`fk_module`, `fk_sqkseq`),
    INDEX (`quake_ot_dt`),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Automatic link messages from EW';

