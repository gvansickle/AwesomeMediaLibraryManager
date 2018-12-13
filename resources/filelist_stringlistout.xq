(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Path to the AMLM database, will be passed in as a bound variable. :)
declare variable $input_file_path external;

(: Return URLs (as strings) to all media files in the database :)
for $x in fn:doc($input_file_path)/amlm_database/playlist//exturl_media/href
(: let/where (matches($x, '.*\.flac$')) :)
return fn:string($x)

