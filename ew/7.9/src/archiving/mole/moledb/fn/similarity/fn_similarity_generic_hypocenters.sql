DELIMITER $$
DROP FUNCTION IF EXISTS `fn_similarity_generic_hypocenters` $$
CREATE FUNCTION `fn_similarity_generic_hypocenters` (
  l_ot_dt1                DATETIME,
  l_ot_dt2                DATETIME,
  l_lat1                  FLOAT,
  l_lon1                  FLOAT,
  l_lat2                  FLOAT,
  l_lon2                  FLOAT
)
RETURNS DOUBLE
NOT DETERMINISTIC
BEGIN

  DECLARE ret                     DOUBLE          DEFAULT -1.0;
  DECLARE l_factor_velocity       FLOAT           DEFAULT 8.0;

  IF l_ot_dt1 IS NOT NULL AND l_ot_dt2 IS NOT NULL AND l_lat1 IS NOT NULL AND l_lon1 IS NOT NULL AND l_lat2 IS NOT NULL AND l_lon2 IS NOT NULL THEN
    SET ret = fn_delta(l_lat1, l_lon1, l_lat2, l_lon2)
    + ( ABS(UNIX_TIMESTAMP(l_ot_dt1) - UNIX_TIMESTAMP(l_ot_dt2)) * l_factor_velocity );
  END IF;

  RETURN ret;

END $$
DELIMITER ;
