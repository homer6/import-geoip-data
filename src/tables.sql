


Lookups:




set @ip_address = inet_aton( "211.38.137.33" );

set @location_id = ( SELECT location_id FROM geo_ip_block
WHERE
    Intersects( ip_address, POINT(1, @ip_address) ) = 1;
);

SELECT SQL_NO_CACHE * FROM geo_ip_location
WHERE
    id = @location_id
;



