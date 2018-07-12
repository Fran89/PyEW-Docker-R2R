
DROP TABLE IF EXISTS `ew_params_pick_sta`;
CREATE TABLE `ew_params_pick_sta` (
    `fk_module`               BIGINT     NOT NULL COMMENT 'Foreign key to ew_module',
    `fk_scnl`                BIGINT     NOT NULL COMMENT 'Foreign key to ew_scnl',
    `revision`              BIGINT     NOT NULL DEFAULT '0' COMMENT 'Revision number for channel',
    `pick_flag`             INT        DEFAULT NULL COMMENT '0, ew_params_pick will not try to pick P-wave arrivals, 1 yes',
    `pin_numb`              BIGINT     DEFAULT NULL COMMENT 'this field is not used by ew_params_pick, but exists for posterity',
    `itr1`                  BIGINT     DEFAULT NULL COMMENT '(i5) used to calculate the zero-crossing termination count',
    `minsmallzc`            BIGINT     DEFAULT NULL COMMENT '(i6) Defines minimum number of zero-crossings for valid pick',
    `minbigzc`              BIGINT     DEFAULT NULL COMMENT '(i7) Defines min number "big zero-crossings" for valid pick',
    `minpeaksize`           BIGINT     DEFAULT NULL COMMENT '(i8) Defines min amplitude (digital counts) for a valid pick',
    `maxmint`               BIGINT     DEFAULT NULL COMMENT 'Maximum interval (in samples) between zero crossings',
    `i9`                    BIGINT     DEFAULT NULL COMMENT 'Defines the minimum coda length (seconds) for a valid pick',
    `rawdatafilt`           DOUBLE     DEFAULT NULL COMMENT '(c1) Sets filter parameter RawDataFilt applied to raw trace',
    `charfuncfilt`          DOUBLE     DEFAULT NULL COMMENT '(c2) Sets filter parameter CharFuncFilt characteristic fun.',
    `stafilt`               DOUBLE     DEFAULT NULL COMMENT '(c3) Sets filter parameter (time constant) StaFilt (STA)',
    `ltafilt`               DOUBLE     DEFAULT NULL COMMENT '(c4) Sets filter parameter (time constant) LtaFilt (LTA)',
    `eventthresh`           DOUBLE     DEFAULT NULL COMMENT '(c5) Sets STA/LTA event threshold',
    `rmavfilt`              DOUBLE     DEFAULT NULL COMMENT 'Filter parameter (time constant)running mean of the absolute',
    `deadsta`               DOUBLE     DEFAULT NULL COMMENT '(c6) Sets the dead station threshold (counts).',
    `codaterm`              DOUBLE     DEFAULT NULL COMMENT '(c7) Sets the "normal" coda termination threshold (counts).',
    `altcoda`               DOUBLE     DEFAULT NULL COMMENT '(c8) Defines "noisy station level"',
    `preevent`              DOUBLE     DEFAULT NULL COMMENT '(c9) Defines alternate coda termination thresh for noisy sta',
    `erefs`                 DOUBLE     DEFAULT NULL COMMENT 'Used calculating the increment (crtinc) to be added to ecrit',
    `clipcount`             BIGINT     DEFAULT NULL COMMENT 'Specifies maximum absolute amplitude, in counts zero-to-peak',
    `ext_id`                BIGINT     DEFAULT NULL COMMENT 'Link to an external channel id',
    `modified`              TIMESTAMP  DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',
    PRIMARY KEY (`fk_scnl`,`fk_module`,`revision`),
    FOREIGN KEY (`fk_module`)  REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_scnl`)  REFERENCES ew_scnl(`id`)   ON DELETE CASCADE ON UPDATE CASCADE,
          INDEX (`revision`),
          INDEX (`ext_id`)
)
ENGINE = InnoDB COMMENT='Configuration EarthWorm module pick_ew';

