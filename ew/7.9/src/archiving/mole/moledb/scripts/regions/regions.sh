#!/bin/sh

rm -f tmp_regions.sql

echo "INSERT INTO ew_region_kind VALUES (0,  'Italia');" >> tmp_regions.sql
echo "INSERT INTO ew_region_kind VALUES (10, 'NEIC');"   >> tmp_regions.sql

mysql_format -h hdb1 -u root -p -d seis_ev -s regions.sql -f regions.format           >> tmp_regions.sql
mysql_format -h hdb1 -u root -p -d seis_ev -s regions_neic.sql -f regions_neic.format >> tmp_regions.sql

sed -e "s/Polygon((\([^,][^,]*\),\([^)][^)]*\))/Polygon((\1,\2,\1)/" tmp_regions.sql > out_regions.sql

