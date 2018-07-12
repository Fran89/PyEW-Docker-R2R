DELIMITER $$
DROP FUNCTION IF EXISTS `fn_similarity_ew_arc_summary` $$
CREATE FUNCTION `fn_similarity_ew_arc_summary` (
  in_id1 BIGINT,
  in_id2 BIGINT
)
RETURNS DOUBLE
NOT DETERMINISTIC
BEGIN

  DECLARE ret                     DOUBLE          DEFAULT -1.0;
  DECLARE l_id1                   BIGINT          DEFAULT -1;
  DECLARE l_id2                   BIGINT          DEFAULT -1;
  DECLARE l_ot_dt1                DATETIME        DEFAULT NULL;
  DECLARE l_ot_dt2                DATETIME        DEFAULT NULL;
  DECLARE l_lat1                  FLOAT           DEFAULT NULL;
  DECLARE l_lon1                  FLOAT           DEFAULT NULL;
  DECLARE l_lat2                  FLOAT           DEFAULT NULL;
  DECLARE l_lon2                  FLOAT           DEFAULT NULL;

  IF in_id1 IS NOT NULL THEN
    SET l_id1 = in_id1;
  END IF;

  IF in_id2 IS NOT NULL THEN
    SET l_id2 = in_id2;
  END IF;

  SELECT lat, lon, ot_dt INTO l_lat1, l_lon1, l_ot_dt1 FROM ew_arc_summary WHERE id = l_id1;
  SELECT lat, lon, ot_dt INTO l_lat2, l_lon2, l_ot_dt2 FROM ew_arc_summary WHERE id = l_id2;

  IF l_lat1 IS NOT NULL AND l_lon1 IS NOT NULL AND l_lat2 IS NOT NULL AND l_lon2 IS NOT NULL THEN
    SET ret = fn_similarity_generic_hypocenters(l_ot_dt1, l_ot_dt2, l_lat1, l_lon1, l_lat2, l_lon2);
  END IF;

  RETURN ret;

END $$
DELIMITER ;
