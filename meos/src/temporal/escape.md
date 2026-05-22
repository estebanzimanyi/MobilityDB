
#Escaping text and jsonb literals

## Output functions

```sql
test=# SELECT text '{"location": {"geom": "Point(1 1)", "timestamp": "2001-12-07 11:10:00+01"}, "vehicleId": "97"}';
-- {"location": {"geom": "Point(1 1)", "timestamp": "2001-12-07 11:10:00+01"}, "vehicleId": "97"}

test=# SELECT ARRAY[text '{"location": {"geom": "Point(1 1)", "timestamp": "2001-12-07 11:10:00+01"}, "vehicleId": "97"}'];
-- {"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}

test=# SELECT set(text '{"location": {"geom": "Point(1 1)", "timestamp": "2001-12-07 11:10:00+01"}, "vehicleId": "97"}');
-- {"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}

test=# SELECT jsonb '{"location": {"geom": "Point(1 1)", "timestamp": "2001-12-07 11:10:00+01"}, "vehicleId": "97"}';
--  {"location": {"geom": "Point(1 1)", "timestamp": "2001-12-07 11:10:00+01"}, "vehicleId": "97"}

test=# SELECT ARRAY[jsonb '{"location": {"geom": "Point(1 1)", "timestamp": "2001-12-07 11:10:00+01"}, "vehicleId": "97"}'];
-- {"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}

test=# SELECT set(jsonb '{"location": {"geom": "Point(1 1)", "timestamp": "2001-12-07 11:10:00+01"}, "vehicleId": "97"}');
-- {"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}
```

## Input functions

```sql
test=# SELECT textset '{"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}';
-- {"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}

test=# SELECT jsonbset '{"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}';
-- {"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}
```


test=# SELECT '{"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}'::textset;
-- {"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}



'{"{\"location\": {\"geom\": \"Point(1 1)\", \"timestamp\": \"2001-12-07 11:10:00+01\"}, \"vehicleId\": \"97\"}"}'
 {"{\"location\": {\"geom\": \"0101000020E40E0000000CA970C670574034C9C88C5BC74940\", \"timestamp\": \"2001-12-06 21:35:00+01\"}, \"vehicleId\": \"25\"}
