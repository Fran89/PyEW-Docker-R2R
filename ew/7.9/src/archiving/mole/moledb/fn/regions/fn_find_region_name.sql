
DELIMITER $$
DROP FUNCTION IF EXISTS `fn_find_region_name`$$
CREATE  DEFINER = CURRENT_USER  FUNCTION `fn_find_region_name`(
in_lat                   DOUBLE    /*latitude (North=positive)*/,
in_lon                   DOUBLE    /*longitude (East=positive)*/
) RETURNS VARCHAR(1024) NOT DETERMINISTIC READS SQL DATA
 BEGIN
    DECLARE l_region    VARCHAR(1024);
    DECLARE l_sp        VARCHAR(1024);
    DECLARE l_p         GEOMETRY;

    SET l_region = NULL;
    -- WARNING: bug or problem setting l_p directly using the following instruction
    --          SET l_p = GeomFromText( CONCAT('POINT', '(',CAST(in_lat AS CHAR) ,' ',  CAST(in_lon AS CHAR), ')' ) );
    --  Possible solution: set an intermediate string variable l_sp and successively pass it to GeomFromText()
    --  Calling Contains(g, l_p) is the fastest way I found.
    SET l_sp = CONCAT('POINT', '(',CAST(in_lat AS CHAR) ,' ',  CAST(in_lon AS CHAR), ')' );
    SET l_p = GeomFromText(l_sp);

    SELECT
    region
    /*
    -- For debugging
    GROUP_CONCAT(
	region, '{', kind, '}', IF(fn_pnpoly(g, in_lat, in_lon)>0, '', '[?]')
	ORDER BY fn_pnpoly(g, in_lat, in_lon)  DESC, kind, Area(g)
	SEPARATOR '; '
    )
    */
    INTO l_region
    FROM (
	SELECT *
	FROM ew_region_geom
	WHERE Contains(g, l_p)
    ) tmp_table1
    ORDER BY fn_pnpoly(g, in_lat, in_lon)  DESC, kind, Area(g)
    LIMIT 1
    ;

    if l_region IS NULL then 
	SET l_region = 'Region not found!'; 
    END IF;

    RETURN l_region;
END $$
DELIMITER ;

