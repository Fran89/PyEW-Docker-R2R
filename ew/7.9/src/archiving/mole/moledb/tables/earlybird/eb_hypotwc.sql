
DROP TABLE IF EXISTS `eb_hypotwc`;
CREATE TABLE `eb_hypotwc` (
  `id`               BIGINT   NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
  `fk_module`        BIGINT   NOT NULL COMMENT 'Foreign key to ew_module',

  `dLat`             DOUBLE   DEFAULT NULL COMMENT 'Geocentric epicentral latitude',
  `dLon`             DOUBLE   DEFAULT NULL COMMENT 'Geocentric epicentral longitude', 

  `dOriginTime_dt`   DATETIME DEFAULT NULL COMMENT 'Origin time - DATETIME from dOriginTime',
  `dOriginTime_usec` INT      DEFAULT NULL COMMENT 'Origin time - usec from dOriginTime',

  `dDepth`           DOUBLE   DEFAULT NULL COMMENT 'Hypocenter depth (in km)',
  `dPreferredMag`    DOUBLE   DEFAULT NULL COMMENT 'Magnitude to use for this event',
  `szPMagType`       CHAR(3)  DEFAULT NULL COMMENT 'Magnitude type of dPreferredMag (l, b, etc.)',

  `iNumPMags`        BIGINT   DEFAULT NULL COMMENT 'Number of stations used in dPreferredMag',
  `szQuakeID`        CHAR(33) DEFAULT NULL COMMENT 'Event ID',
  `iVersion`         BIGINT   DEFAULT NULL COMMENT 'Version of location (1, 2, ...)',
  `iNumPs`           BIGINT   DEFAULT NULL COMMENT 'Number of P s used in quake solution',
  `dAvgRes`          DOUBLE   DEFAULT NULL COMMENT 'Residual average',
  `iAzm`             BIGINT   DEFAULT NULL COMMENT 'Azimuthal coverage in degrees',
  `iGoodSoln`        BIGINT   DEFAULT NULL COMMENT '0-bad, 1-soso, 2-good, 3-very good',
  `dMbAvg`           DOUBLE   DEFAULT NULL COMMENT 'Modified average Mb',
  `iNumMb`           BIGINT   DEFAULT NULL COMMENT 'Number of Mb s in modified average',
  `dMlAvg`           DOUBLE   DEFAULT NULL COMMENT 'Modified average Ml',
  `iNumMl`           BIGINT   DEFAULT NULL COMMENT 'Number of Ml s in modified average',
  `dMSAvg`           DOUBLE   DEFAULT NULL COMMENT 'Modified average MS',
  `iNumMS`           BIGINT   DEFAULT NULL COMMENT 'Number of MS s in modified average',
  `dMwpAvg`          DOUBLE   DEFAULT NULL COMMENT 'Modified average Mwp',
  `iNumMwp`          BIGINT   DEFAULT NULL COMMENT 'Number of Mwp s in modified average',
  `dMwAvg`           DOUBLE   DEFAULT NULL COMMENT 'Modified average Mw',
  `iNumMw`           BIGINT   DEFAULT NULL COMMENT 'Number of Mw s in modified average',
  `dTheta`           DOUBLE   DEFAULT NULL COMMENT 'Theta (energy/moment) ratio',
  `iNumTheta`        BIGINT   DEFAULT NULL COMMENT 'Number of stations used in theta computation',
  `iMagOnly`         BIGINT   DEFAULT NULL COMMENT '0->New location, 1->updated magnitudes only',

  `modified`        TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',            
  PRIMARY KEY (`id`),
  FOREIGN KEY (`fk_module`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'TYPE_HYPOTWC';

