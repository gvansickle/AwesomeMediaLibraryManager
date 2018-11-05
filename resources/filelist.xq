(: declare variable $where as xs:string := doc($fileTree)/@filePath; :)
(: declare variable $where as xs:string := string(doc($fileTree)/@filePath); :)
(: declare variable $x as xs:integer := 0; :)
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

for $x in fn:doc('/home/gary/DeleteMe.xspf')/playlist//exturl[@id="exturl_media"]
where (matches($x/href, '.*\.flac$'))
return <location>{data($x/href)}</location>


