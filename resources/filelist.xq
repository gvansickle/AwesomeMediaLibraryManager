(: declare variable $where as xs:string := doc($fileTree)/@filePath; :)
(: declare variable $where as xs:string := string(doc($fileTree)/@filePath); :)
(: declare variable $x as xs:integer := 0; :)
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Extract all *.flac files. :)
(:declare variable $dirpath as xs:string := '/home/gary/DeleteMe.xspf';:)
declare variable $in_filepath external;

for $x in fn:doc($in_filepath)/amlm_database/playlist//exturl_media
where (matches($x/href, '.*\.flac$'))
return <location>{data($x/href)}</location>
