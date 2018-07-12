
-- http://en.wikipedia.org/wiki/Point_in_polygon
-- http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
-- http://www.sql-statements.com/point-in-polygon.html
-- int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy)
-- {
--   int i, j, c = 0;
--   for (i = 0, j = nvert-1; i < nvert; j = i++) {
--     if ( ((verty[i]>testy) != (verty[j]>testy)) &&
-- 	 (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
--        c = !c;
--   }
--   return c;
-- }


DELIMITER $$
DROP FUNCTION IF EXISTS `fn_pnpoly`$$
CREATE  DEFINER = CURRENT_USER  FUNCTION `fn_pnpoly`(
in_g    GEOMETRY,
in_X    DOUBLE,
in_Y    DOUBLE
)
RETURNS INT DETERMINISTIC
BEGIN
    DECLARE l_linestring          LineString DEFAULT NULL;
    DECLARE l_num_points          BIGINT DEFAULT NULL;
    DECLARE l_Xj                  DOUBLE DEFAULT 0;
    DECLARE l_Xi                  DOUBLE DEFAULT 0;
    DECLARE l_Yj                  DOUBLE DEFAULT 0;
    DECLARE l_Yi                  DOUBLE DEFAULT 0;
    DECLARE l_in                  INT DEFAULT -1;

    DECLARE i                     INT DEFAULT 0;

    SET l_linestring=ExteriorRing(in_g);
    SET l_num_points=NumPoints(l_linestring);

    SET l_Xj = X(PointN(l_linestring, l_num_points));
    SET l_Yj = Y(PointN(l_linestring, l_num_points));
    SET i = 1;
    WHILE (i <= l_num_points) DO
	SET l_Xi = X(PointN(l_linestring, i));
	SET l_Yi = Y(PointN(l_linestring, i));
	IF( ((l_Yi>in_Y) <> (l_Yj>in_Y)) AND
	    ( in_X <  ( (l_Xj-l_Xi) * ((in_Y-l_Yi) / (l_Yj-l_Yi)) + l_Xi) ) ) THEN
		SET l_in = l_in * (-1);
	END IF;
	SET l_Xj = l_Xi;
	SET l_Yj = l_Yi;
	SET i = i + 1;
    END WHILE;

    RETURN l_in;
END $$
DELIMITER ;

