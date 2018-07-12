
DROP TABLE IF EXISTS `ew_magnitude_summary`;
CREATE TABLE `ew_magnitude_summary` (
    `id`                    BIGINT NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `fk_module`               BIGINT NOT NULL COMMENT 'Foreign key to ew_module',
    `fk_sqkseq`               BIGINT   NOT NULL COMMENT 'Foreign key to ew_sqkseq',
    `version`               BIGINT NOT NULL COMMENT 'version number of the origin',
    `mag`                   DOUBLE          COMMENT 'REQUIRED: magnitude value',
    `error`                 DOUBLE          COMMENT 'OPTIONAL: Error estimate (std deviation for Ml/AVG)',
    `quality`               DOUBLE          COMMENT 'OPTIONAL: [0.0 - 1.0], -1.0 for NULL',
    `mindist`               DOUBLE          COMMENT 'OPTIONAL: Minumun distance from location to station',
    `azimuth`               INT             COMMENT 'OPTIONAL: Maximum azimuthal gap for stations',
    `nstations`             INT             COMMENT 'OPTIONAL: Number of stations used to compute magnitude',
    `nchannels`             INT             COMMENT 'OPTIONAL: Number of data channels used to compute magnitude.',
    `qauthor`               CHAR(50)        COMMENT 'REQUIRED: NTS, author of the eventid',
    `qdds_version`          INT             COMMENT 'qdds',
    `imagtype`              INT             COMMENT 'REQUIRED: Magnitude type from MagNames table above',
    `szmagtype`             CHAR(6)         COMMENT 'Magnitude type string',
    `algorithm`             CHAR(8)         COMMENT 'OPTIONAL: NTS, AVG for average',
    `mag_quality`           CHAR(2)           COMMENT 'INGV quality code of the magnitude (computed by ew2moledb)',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',            
    PRIMARY KEY (`id`),                  
    UNIQUE (`fk_module`, `fk_sqkseq`, `version`),
    FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_sqkseq`) REFERENCES ew_sqkseq(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Automatic magnitude messages from EW, summary line';

