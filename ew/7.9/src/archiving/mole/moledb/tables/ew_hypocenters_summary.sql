
DROP TABLE IF EXISTS `ew_hypocenters_summary`;
CREATE TABLE IF NOT EXISTS `ew_hypocenters_summary` (
    `id`                    BIGINT    NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_sqkseq`             BIGINT    NOT NULL     COMMENT 'Foreign key to ew_sqkseq',
    `qkseq`                 BIGINT    NOT NULL     COMMENT 'Quake sequence number from binder',
    `version`               BIGINT    NOT NULL     COMMENT 'version number of the origin',
    `ewname`                CHAR(80)  DEFAULT NULL COMMENT 'Name of the EW instance running',
    -- from ew_arc_summary
    `arc_fk_module`         BIGINT    DEFAULT NULL COMMENT 'Foreign key to ew_module',
    `ot_dt`                 DATETIME  DEFAULT NULL COMMENT 'Origin Time - datetime part',
    `ot_usec`               INT       DEFAULT NULL COMMENT 'Origin Time - useconds',
    `lat`                   DOUBLE    DEFAULT NULL COMMENT 'latitude (North=positive)',
    `lon`                   DOUBLE    DEFAULT NULL COMMENT 'longitude(East=positive)',
    `z`                     DOUBLE    DEFAULT NULL COMMENT 'depth (down=positive)',
    `arc_quality`           CHAR(2)   DEFAULT NULL COMMENT 'INGV quality code of the location computed by ew2moledb',
    `erh`                   DOUBLE    DEFAULT NULL COMMENT 'horizontal error (km)',
    `erz`                   DOUBLE    DEFAULT NULL COMMENT 'vertical error (km)',
    `nphtot`                INT       DEFAULT NULL COMMENT '# phases (P&S) w/ weight >0.0',
    `gap`                   INT       DEFAULT NULL COMMENT 'maximum azimuthal gap',
    `dmin`                  INT       DEFAULT NULL COMMENT 'distance (km) to nearest station',
    `rms`                   DOUBLE    DEFAULT NULL COMMENT 'RMS travel time residual',
    `arc_modified`          DATETIME  DEFAULT NULL COMMENT 'Last Review',        
    -- from ew_magnitude_summary
    `mag_fk_module`         BIGINT    DEFAULT NULL COMMENT 'Foreign key to ew_module',
    `mag`                   DOUBLE    DEFAULT NULL COMMENT 'REQUIRED: magnitude value',
    `mag_error`             DOUBLE    DEFAULT NULL COMMENT 'OPTIONAL: Error estimate (std deviation for Ml/AVG)',
    `mag_quality`           DOUBLE    DEFAULT NULL COMMENT 'OPTIONAL: [0.0 - 1.0], -1.0 for NULL',
    `mag_type`              CHAR(6)   DEFAULT NULL COMMENT 'Magnitude type string',
    `nstations`             INT       DEFAULT NULL COMMENT 'OPTIONAL: Number of stations used to compute magnitude',
    `nchannels`             INT       DEFAULT NULL COMMENT 'OPTIONAL: Number of data channels used to compute magnitude.',
    `mag_modified`          DATETIME  DEFAULT NULL COMMENT 'Last Review',        
    `region`                VARCHAR(255)  DEFAULT NULL COMMENT 'Seismic region',        
    -- other
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`id`, `ot_dt`),
    INDEX (`ot_dt`),
    UNIQUE (`arc_fk_module`, `mag_fk_module`, `fk_sqkseq`, `version`),
    FOREIGN KEY (`arc_fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`mag_fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`version`),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Event summary report from EW arc and mag messages';

