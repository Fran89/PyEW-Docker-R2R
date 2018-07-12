
DROP TABLE IF EXISTS `ew_magnitude_phase`;
CREATE TABLE `ew_magnitude_phase` (
    `id`                    BIGINT   NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`               BIGINT   NOT NULL COMMENT 'Foreign key to ew_module',
    `fk_sqkseq`               BIGINT   NOT NULL COMMENT 'Foreign key to ew_sqkseq',
    `version`               BIGINT   NOT NULL COMMENT 'version number of the origin',
    `fk_scnl`                BIGINT   NOT NULL COMMENT 'Foreign key to ew_scnl',
    `mag`                   DOUBLE            COMMENT 'local magnitude for this channel',
    `dist`                  DOUBLE            COMMENT 'station-event distance used for local magnitude',
    `mag_correction`        DOUBLE            COMMENT 'correction that was added to get this local mag',
    `Time1_dt`              DATETIME          COMMENT 'time of the first pick - datetime part',
    `Time1_usec`            INT               COMMENT 'time of the first pick - usec',    
    `Amp1`                  DOUBLE            COMMENT 'amplitude of the first pick',
    `Period1`               DOUBLE            COMMENT 'period associated with the first pick',
    `Time2_dt`              DATETIME          COMMENT 'time of the second pick (if used) - datetime part',
    `Time2_usec`            INT               COMMENT 'time of the second pick (if used) - usec',
    `Amp2`                  DOUBLE            COMMENT 'amplitude of the second pick (if used)',
    `Period2`               DOUBLE            COMMENT 'period of the second pick (if used)',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',            
    PRIMARY KEY (`id`),
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`)   ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`fk_module`, `fk_sqkseq`, `version`),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Automatic magnitude messages from EW, phase line';

