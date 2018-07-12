
DROP TABLE IF EXISTS `version`;
CREATE TABLE IF NOT EXISTS `version` (
    `id`                BIGINT NOT NULL                 COMMENT 'Unique incremental id',
    `version`           VARCHAR(255) NOT NULL           COMMENT 'moledb version (i.e. 1.2.5, 1.2.1-dev)',
    `release_date`      DATE NOT NULL                   COMMENT 'date of release',
    `modified`          TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last Review',
    PRIMARY KEY (`id`)
)
ENGINE = InnoDB COMMENT 'One single row table for storing moledb version and info';

