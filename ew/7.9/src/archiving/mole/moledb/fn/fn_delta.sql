
DELIMITER $$
DROP FUNCTION IF EXISTS `fn_delta` $$
CREATE FUNCTION `fn_delta` (
in_lat1 float,
in_lon1 float,
in_lat2 float,
in_lon2 float)
RETURNS FLOAT
DETERMINISTIC
BEGIN
DECLARE l_g1            FLOAT;
DECLARE l_g2            FLOAT;
DECLARE l_gr            FLOAT;
DECLARE l_a             FLOAT;
DECLARE l_b             FLOAT;
DECLARE l_rlonep        FLOAT;
DECLARE l_gamma2        FLOAT;
DECLARE l_amenb2        FLOAT;
DECLARE l_apiub2        FLOAT;
DECLARE l_tgaamenbb2    FLOAT;
DECLARE l_tgaapiubb2    FLOAT;
DECLARE l_aamenbb2      FLOAT;
DECLARE l_aapiubb2      FLOAT;
DECLARE l_tgc2s         FLOAT;
DECLARE l_cs            FLOAT;
DECLARE l_tgc2p         FLOAT;
DECLARE l_cp            FLOAT;
DECLARE l_azim          FLOAT;
DECLARE l_delta         FLOAT;
DECLARE l_rlonst        FLOAT;

SET l_g1=6.28318531;
SET l_g2=360;
SET l_gr=l_g1/l_g2;
SET l_b=(90-in_lat1)*l_gr;      /*--   angolo tra polo N ed epicentro*/
SET l_rlonep=in_lon1*l_gr;
SET l_a=(90-in_lat2)*l_gr;       /*--   angolo tra polo N e stazione*/
SET l_rlonst=in_lon2*l_gr;
SET l_gamma2=(l_rlonst-l_rlonep)/2;
SET l_amenb2=(l_a-l_b)/2;
SET l_apiub2=(l_a+l_b)/2;
IF (ABS(l_gamma2)<(0.000001*l_gr)) THEN
    SET l_delta=(ABS(in_lat1-in_lat2)*l_gr);
ELSE
    IF (in_lat1=in_lat2) THEN
        SET l_tgaamenbb2=(1/Tan(l_gamma2))*Sin(l_amenb2)/Sin(l_apiub2);
        SET l_tgaapiubb2=(1/Tan(l_gamma2))*Cos(l_amenb2)/Cos(l_apiub2);
        SET l_aamenbb2=Atan(l_tgaamenbb2);
        SET l_aapiubb2=Atan(l_tgaapiubb2);
        SET l_tgc2s=Tan(l_apiub2)*Cos(l_aapiubb2)/Cos(l_aamenbb2);
        SET l_cs=2*Atan(l_tgc2s);
        SET l_delta=l_cs;
    ELSE
        SET l_tgaamenbb2=(1/Tan(l_gamma2))*Sin(l_amenb2)/Sin(l_apiub2);
        SET l_tgaapiubb2=(1/Tan(l_gamma2))*Cos(l_amenb2)/Cos(l_apiub2);
        SET l_aamenbb2=Atan(l_tgaamenbb2);
        SET l_aapiubb2=Atan(l_tgaapiubb2);
        SET l_tgc2p=Tan(l_amenb2)*Sin(l_aapiubb2)/Sin(l_aamenbb2);
        SET l_cp=2*Atan(l_tgc2p);
        SET l_delta=l_cp;
    END IF;
END IF;
RETURN Abs((l_delta/l_gr)*111.195);
END $$
DELIMITER ;

