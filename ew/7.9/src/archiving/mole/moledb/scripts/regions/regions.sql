SELECT
    -- polygon.id,
    -- polygon.id_name_reg, polygon.num_point,
    CAST(GROUP_CONCAT( CONCAT(point.lat, ' ', point.lon) ORDER BY point.id SEPARATOR ',') AS CHAR),
    REPLACE(polygon.name, '\'', '\\\'')
FROM polygon
    JOIN point ON (point.fk_poly = polygon.id)
GROUP BY polygon.id


