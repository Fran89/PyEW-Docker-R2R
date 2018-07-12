
DROP TABLE IF EXISTS `ew_region_kind`;
CREATE TABLE IF NOT EXISTS `ew_region_kind` (
    `kind`               INT               NOT NULL DEFAULT 0 COMMENT 'identifier of kind of the regions',
    `name`               VARCHAR(255)      NOT NULL COMMENT 'Name of the kind of the regions',
    PRIMARY KEY (`kind`)
)
ENGINE = InnoDB COMMENT 'Kind of the regions';

