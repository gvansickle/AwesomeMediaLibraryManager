(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Path to the AMLM database, will be passed in as a bound variable. :)
declare variable $input_file_path external;
declare variable $extension_regex external;

(: Return URLs (as strings) to all media files in the database :)
for $x in fn:doc($input_file_path)/amlm_database/playlist//exturl_media/href
(:where fn:matches($x, '.*\.flac$'):)
where fn:matches($x, $extension_regex)
return fn:string($x)

