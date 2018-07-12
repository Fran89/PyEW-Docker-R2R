SELECT
    CONCAT(
	CONCAT(lat,     ' ', lon),
	',' ,
	CONCAT(lat+1.0,     ' ', lon),
	',' ,
	CONCAT(lat+1.0, ' ', lon+1.0),
	',' ,
	CONCAT(lat, ' ', lon+1.0)
    ),
    REPLACE(reg_name, '\'', '\\\'')
FROM reg_neic;

