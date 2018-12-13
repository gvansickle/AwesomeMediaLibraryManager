(: declare variable $where as xs:string := doc($fileTree)/@filePath; :)
(: declare variable $where as xs:string := string(doc($fileTree)/@filePath); :)
(: declare variable $x as xs:integer := 0; :)
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Extract all *.flac files. :)
(: Path to the AMLM database, will be passed in. :)
declare variable $input_file_path external;

(: Load all media files. :)
let $media_file_list := fn:doc($input_file_path)/amlm_database/playlist//exturl_media/href
return $media_file_list

(: for $x in fn:doc($input_file_path)/amlm_database/playlist//exturl_media/href :)
(: where (matches($x, '.*\.flac$')) :)
(: return ($x) :)

