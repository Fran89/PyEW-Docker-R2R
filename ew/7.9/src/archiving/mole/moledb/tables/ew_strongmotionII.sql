
DROP TABLE IF EXISTS `ew_strongmotionII`;
CREATE TABLE `ew_strongmotionII` (
    `id`                   BIGINT   NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`              BIGINT   NOT NULL  COMMENT 'Foreign key to ew_module',
    `fk_sqkseq`              BIGINT   NOT NULL  COMMENT 'Foreign key to ew_sqkseq',
    `version`              BIGINT   DEFAULT NULL  COMMENT 'version number of the origin',
    `fk_scnl`               BIGINT   NOT NULL   COMMENT 'Foreign key to ew_scnl',
    `qauthor`              VARCHAR(50)            COMMENT 'OPTIONAL: NTS, author of the eventid',
    `t_dt`                 DATETIME            COMMENT 'time: trigger reported by SM box - datetime part',
    `t_usec`               INT                 COMMENT 'time: trigger reported by SM box -usec', 
    `t_alt_dt`             DATETIME            COMMENT 'alternate time: trigger reported by SM box - datetime part',
    `t_alt_usec`           INT                 COMMENT 'alternate time: trigger reported by SM box -usec',
    `altcode`              INT                 COMMENT 'code specifying the source of the alternate time field',
    `pga`                  DOUBLE              COMMENT 'REQUIRED: peak ground acceleration (cm/s/s)',
    `tpga_dt`              DATETIME            COMMENT 'OPTIONAL: time of pga - datetime part',
    `tpga_usec`            INT                 COMMENT 'OPTIONAL: time of pga - usec',
    `pgv`                  DOUBLE              COMMENT 'REQUIRED: peak ground velocity (cm/s)',
    `tpgv_dt`              DATETIME            COMMENT 'OPTIONAL: time of pgv - datetime part',
    `tpgv_usec`            INT                 COMMENT 'OPTIONAL: time of pgv - usec',
    `pgd`                  DOUBLE              COMMENT 'REQUIRED: peak ground displacement (cm)',
    `tpgd_dt`              DATETIME            COMMENT 'OPTIONAL: time of pgd - datetime part',
    `tpgd_usec`            INT                 COMMENT 'OPTIONAL: time of pgd - usec',
    `rsa`                  VARCHAR(255)           COMMENT 'RSA(response spectrum accel)  string format: NRSA/periodRSA valueRSA/periodRSA valueRSA',
    `modified`             TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',
    PRIMARY KEY (`id`),                
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`)   ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`fk_module`, `fk_sqkseq`, `version`),
    INDEX (`modified`)
)   
ENGINE = InnoDB COMMENT 'Automatic strong motion messages from EW';

