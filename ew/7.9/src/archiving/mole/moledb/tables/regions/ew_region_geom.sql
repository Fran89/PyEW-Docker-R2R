
-- TODO ALTER TABLE ew_region_geom ADD `id`                 BIGINT            NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id' PRIMARY KEY;

DROP TABLE IF EXISTS `ew_region_geom`;
CREATE TABLE IF NOT EXISTS `ew_region_geom` (
    `id`                 BIGINT            NOT NULL AUTO_INCREMENT  COMMENT 'Unique incremental id',
    `kind`               INT               NOT NULL DEFAULT 0 COMMENT 'identifier of kind of the region',
    `g`                  GEOMETRY          NOT NULL COMMENT 'Region geometry',
    `region`             VARCHAR(255)      NOT NULL COMMENT 'Region name',
    PRIMARY KEY (`id`),
    INDEX (`kind`),
    FOREIGN KEY (`kind`) REFERENCES ew_region_kind(`kind`) ON DELETE CASCADE ON UPDATE CASCADE
)
ENGINE = InnoDB COMMENT 'Region geometry and names';

-- ---------------------------------------------------------------
-- Counterclockwise for polygon definition.
-- Suggestion: first and last point be the same.
-- ---------------------------------------------------------------

-- SET @g = 'POLYGON((44.0 7.0, 47.0 19.0, 47.0 19.0, 47.0 7.0 ,44.0 7.0))';
-- INSERT INTO ew_region_geom VALUES (PolygonFromText(@g),'Nord Italia');
-- 
-- SET @g = 'POLYGON((41.5 7.0, 41.5 19, 44.0 19.0, 44.0 7.0,  41.5 7.0))';
-- INSERT INTO ew_region_geom VALUES (PolygonFromText(@g),'Centro Italia');
-- 
-- SET @g = 'POLYGON((36.0 7.0, 36.0 19.0, 41.5 19.0, 41.5 7.0,  36.0 7.0))';
-- INSERT INTO ew_region_geom VALUES (PolygonFromText(@g),'Sud Italia');

-- SELECT region FROM ew_region_geom WHERE Contains(g, GeomFromText('POINT(9.0 37.0)'));

-- ---------------------------------------------------------------

