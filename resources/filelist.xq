(: declare variable $where as xs:string := doc($fileTree)/@filePath; :)
(: declare variable $where as xs:string := string(doc($fileTree)/@filePath); :)
(: declare variable $x as xs:integer := 0; :)
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Extract all *.flac files. :)
(: Path to the AMLM database, will be passed in. :)
declare variable $input_file_path external;

let $media_file_list_flac := fn:doc($input_file_path)/amlm_database/playlist//exturl_media
return
<html>
<body>
<ol>
{
for $x at $count in fn:doc($input_file_path)/amlm_database/playlist//exturl_media
where (matches($x/href, '.*\.flac$'))
return
<li id="{$count}">
	{data($x/href)}
</li>
}
</ol>
<p>Count = {fn:count($media_file_list_flac)}</p>
</body>
</html>
