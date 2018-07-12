
DROP TABLE IF EXISTS `ew_params_picks_vs_phases`;
CREATE TABLE IF NOT EXISTS `ew_params_picks_vs_phases` (
    `fk_instance`                  BIGINT    NOT NULL COMMENT 'Id of the EW instance running',
    `starttime`             DATETIME  NOT NULL COMMENT 'Start time ',
    `endtime`               DATETIME  NOT NULL COMMENT 'End time ',
    `fk_scnl`                BIGINT    NOT NULL COMMENT 'Foreign key to ew_scnl',
    `fk_modulepick`           BIGINT    NOT NULL COMMENT 'Foreign key to ew_module for picker',
    `npicks`                BIGINT    NOT NULL COMMENT 'number of picks',
    `fk_moduleloc`            BIGINT             COMMENT 'Foreign key to ew_module for locator',
    `nphases`               BIGINT    NOT NULL COMMENT 'number of phases',
    `ndiff`                 BIGINT    NOT NULL COMMENT 'difference between number of picks and phases',
    `modified`              TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',        
    PRIMARY KEY (`starttime`, `endtime`, `fk_scnl`),
    FOREIGN KEY (`fk_modulepick`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_moduleloc`) REFERENCES ew_module(`id`) ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_scnl`) REFERENCES ew_scnl(`id`)   ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (`fk_instance`) REFERENCES ew_instance(`id`) ON DELETE CASCADE ON UPDATE CASCADE ,
    INDEX (`starttime`),
    INDEX (`endtime`),
    INDEX (`modified`)
)
ENGINE = InnoDB COMMENT 'Table for picks vs phases';

