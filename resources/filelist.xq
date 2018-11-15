(: declare variable $where as xs:string := doc($fileTree)/@filePath; :)
(: declare variable $where as xs:string := string(doc($fileTree)/@filePath); :)
(: declare variable $x as xs:integer := 0; :)
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Extract all *.flac files. :)
declare variable $dirpath as xs:string := '/home/gary/DeleteMe.xspf';
for $x in fn:doc($dirpath)/playlist//exturl[@id="exturl_media"]
where (matches($x/href, '.*\.flac$'))
return <location>{data($x/href)}</location>
