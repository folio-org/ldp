LDP User Guide
==============

##### Contents  
1\. Data model  
2\. JSON queries  
3\. Relational attributes vs. JSON  
4\. Local schemas  
5\. Important note on database views


1\. Data model
--------------

The LDP data model is a hybrid of relational and JSON schemas.  Each table
contains JSON data in a relational attribute called "data", and a subset of
the JSON fields is also stored in individual relational attributes:

```sql
SELECT * FROM loans LIMIT 1;
```
```
id            | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
user_id       | ab579dc3-219b-4f5b-8068-ab1c7a55c402
proxy_user_id | a13dda6b-51ce-4543-be9c-eff894c0c2d0
item_id       | 459afaba-5b39-468d-9072-eb1685e0ddf4
loan_date     | 2017-02-03 08:43:15+00
due_date      | 2017-02-17 08:43:15+00
return_date   | 2017-03-01 11:35:12+00
action        | checkedin
item_status   | Available
renewal_count | 0
data          | {                                                          
              |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
              |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
              |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
              |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
              |     "loanDate": "2017-02-03T08:43:15Z",                    
              |     "dueDate": "2017-02-17T08:43:15.000+0000",             
              |     "returnDate": "2017-03-01T11:35:12Z",                  
              |     "action": "checkedin",                                 
              |     "itemStatus": "Available",                             
              |     "renewalCount": 0,                                     
              |     "status": {                                            
              |         "name": "Closed"                                   
              |     },                                                     
              | }
```

The relational attributes are provided to simplify writing queries and to
improve query performance.  The JSON fields offer access to the complete
extracted source data.

The data in these tables are extracted from Okapi-based APIs and loaded
into the database by the LDP data loader.  The data loader typically
should be run once per day, and so the LDP database reflects the state
of the source data as of sometime within the past 24 hours.


2\. JSON queries
----------------

To access the JSON fields, it is recommended to use the built in function
`json_extract_path_text()` to retrieve data from a path of up to five nested
JSON fields, for example:

```sql
SELECT data FROM loans LIMIT 1;
```

```
                         data                          
-------------------------------------------------------
 {                                                     
     "id": "87598cd2-4989-4191-aaf0-c8fbb5552a06",     
     "action": "checkedout",                           
     "itemId": "575d5eec-ad28-472f-ba79-09f8cf5539a3", 
     "status": {                                       
         "name": "Open"                                
     },                                                
     "userId": "79dffc9d-be22-462e-b46f-78ca88babe91", 
     "dueDate": "2018-01-15T00:00:00Z",                
     "loanDate": "2018-01-01T00:00:00Z"                
 }                                                     
```

```sql
SELECT json_extract_path_text(data, 'status', 'name') AS status,
       count(*)
    FROM loans
    GROUP BY status;
```

```
 status | count
--------+--------
 Closed | 504407
 Open   |   1294
```

In this example, `json_extract_path_text(data, 'status', 'name')` refers to
the `name` field nested within the `status` field.


3\. Relational attributes vs. JSON
----------------------------------

An example of a query written entirely using JSON data:

```sql
SELECT json_extract_path_text(users.data, 'id') AS user_id,
       json_extract_path_text(groups.data, 'group') AS group
    FROM users
        LEFT JOIN groups
            ON json_extract_path_text(users.data, 'patronGroup') =
               json_extract_path_text(groups.data, 'id')
    LIMIT 5;
```

This can be written in a simpler form by using the relational attributes
rather than JSON fields:

```sql
SELECT users.id AS user_id,
       groups.group
    FROM users
        LEFT JOIN groups
	    ON users.patron_group = groups.id
    LIMIT 5;
```

```
               user_id                |    group
--------------------------------------+---------------
 429510c8-0b50-4ec5-b4ca-462278c0677b | staff
 af91674a-6468-4d5f-b037-28ebbf02f051 | undergraduate
 0736f1a1-6a19-4f13-ab3b-50a4915dcc4a | undergraduate
 37274158-5e38-4852-93c7-359bfdd76892 | undergraduate
 2c2864b1-58cb-4752-ae8e-3ce7bc9a3be4 | faculty
```


<!--

4\. Sample report query
-----------------------

This is an example of a report query that is based on relational attributes
and does not require the use of JSON data.  The query retrieves the number of
loans in subtotals by location and patron group.

```sql
SELECT t.temp_location AS location,
       g.group AS group,
       count(l.id) AS count
    FROM ( SELECT id,
                  user_id
               FROM loans
               WHERE loan_date BETWEEN '2017-01-01' AND '2019-12-31'
         ) AS l
        LEFT JOIN temp_loans AS t
	    ON l.id = t.id
        LEFT JOIN users AS u
	    ON l.user_id = u.id
        LEFT JOIN groups AS g
	    ON u.patron_group = g.id
    GROUP BY t.temp_location, g.group
    ORDER BY t.temp_location, g.group;
```

Note that this query makes use of a join with the `temp_loans` table and
its attribute `temp_location`, which are a temporary workaround for the
lack of a check-out-time "effective location" field.  We expect that the
needed field will be included later in the `loans` table, and the
temporary join table will no longer be required.

-->


4\. Local schemas
-----------------

The `local` schema is created by the LDP loader as a common area in the
database where reporting users can create or import their own data sets,
including storing the results of queries, e.g.:

```sql
CREATE TABLE local.loan_status AS
SELECT json_extract_path_text(data, 'status', 'name') AS status,
       count(*)
    FROM loans
    GROUP BY status;
```

The `ldp` user is granted database permissions to create tables in the
`local` schema.

If additional local schemas are desired, it is recommended that the new
schema names be prefixed with `local_` or `l_` to avoid future naming
collisions with the LDP.


5\. Important note on database views
------------------------------------

The schema of source data can change over time, and the LDP reflects
these changes when it refreshes its data.  For this reason, the LDP
cannot support the use of database views.  The LDP loader may fail to
run if the database contains views.  Instead of creating a view, use
`CREATE TABLE ... AS SELECT ...` to store a result set, as in the local
schema example above.

Reporting users should be aware of schema changes in advance, in order
to be able to update queries and to prepare to recreate local result
sets if needed.


<!--

6\. Historical data
-------------------

As mentioned earlier, the LDP database reflects the state of the source
data as of the last time the LDP data loader was run.  The loader also
maintains another schema called `history` which stores all data that
have been loaded in the past including data that no longer exist in the
source database.  Each table normally has a corresponding history table,
e.g. the history table for `loans` is `history.loans`.

History tables include three attributes: the record ID (`id`), the JSON data
(`data`), and the date and time when the data were loaded (`updated`):

```sql
SELECT * FROM history.loans LIMIT 1;
```
```
id      | 0bab56e5-1ab6-4ac2-afdf-8b2df0434378
data    | {                                                          
        |     "action": "checkedin",                                 
        |     "dueDate": "2017-02-17T08:43:15.000+0000",             
        |     "id": "0bab56e5-1ab6-4ac2-afdf-8b2df0434378",          
        |     "itemId": "459afaba-5b39-468d-9072-eb1685e0ddf4",      
        |     "itemStatus": "Available",                             
        |     "loanDate": "2017-02-03T08:43:15Z",                    
        |     "proxyUserId": "a13dda6b-51ce-4543-be9c-eff894c0c2d0", 
        |     "renewalCount": 0,                                     
        |     "returnDate": "2017-03-01T11:35:12Z",                  
        |     "status": {                                            
        |         "name": "Closed"                                   
        |     },                                                     
        |     "userId": "ab579dc3-219b-4f5b-8068-ab1c7a55c402"       
        | }
updated | 2019-09-06 03:46:49.362606+00
```

Unlike the main LDP tables in which IDs are unique per table, the
history tables can have many records with the same ID.  Note also that
if a value in the source database changes more than once during the
interval between any two runs of the LDP loader, the LDP history will
only reflect the last of those changes.

Since the source data schemas evolve over time, the `data` attribute in
history tables does not necessarily have a single schema that is
consistent over an entire table.  As a result, reporting on history
tables may require "data cleaning" as preparation before the data can be
used.  A suggested first step could be to select a subset of data within
a time window, pulling out JSON fields of interest into relational
attributes, and storing this result in a local table, e.g.:

```sql
CREATE TABLE local.loan_status_history AS
SELECT id,
       json_extract_path_text(data, 'itemStatus') AS item_status,
       updated
    FROM history.loans
    WHERE updated BETWEEN '2019-01-01' AND '2019-12-31';
```

From there one might examine the data to check for inconsistent or missing
values, update them, etc.  Note that in SQL, `''` and `NULL` may look the same
in the output of a `SELECT`, but they are distinct values.

-->


<!--

6\. Schema documentation
------------------------

The LDP project plans to include documentation on mappings between LDP tables
and the FOLIO module interfaces defined by the [FOLIO API
documentation](https://dev.folio.org/reference/api/).

-->


<!--

6\. Report queries
------------------

Report queries for the LDP are being developed by the FOLIO reporting
community in the [ldp-analytics](https://github.com/folio-org/ldp-analytics)
repository.

-->


Further reading
---------------

[**Learn about installing and administering the LDP in the Admin Guide > > >**](ADMIN_GUIDE.md)
