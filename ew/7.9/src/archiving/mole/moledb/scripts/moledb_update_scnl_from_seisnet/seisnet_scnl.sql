SELECT DISTINCT
       s.id_inter,
       c.code,
       n.net_code,
       IF(c.location is NULL,'--',c.location),
       s.latitude,
       s.longitude,
       s.altitude,
       s.name,
       c.id
FROM channel_52 c
   JOIN network n ON c.fk_network = n.id
   JOIN station s ON c.fk_station = s.id
WHERE c.end_time > NOW()
   AND c.main=1
   -- AND s.sit=true
   -- AND (c.use4pick=1 OR c.use4mag=1 OR c.use4shake=1 OR c.use4mt=1)
   -- AND DATEDIFF(c.start_time,FROM_DAYS(TO_DAYS(NOW())-30))>0
   -- AND s.latitude >= 35
   -- AND s.latitude <= 48
   -- AND s.longitude >= 5
   -- AND s.longitude <= 23
ORDER BY s.id_inter,c.code,n.net_code;

