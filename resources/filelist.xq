(: declare variable $where as xs:string := doc($fileTree)/@filePath; :)
(: declare variable $where as xs:string := string(doc($fileTree)/@filePath); :)
(: declare variable $x as xs:integer := 0; :)
(: http://www.w3.org/2005/xpath-functions :)
declare default element namespace "http://xspf.org/ns/0/";



for $x in fn:doc('/home/gary/DeleteMe.xspf')//exturl
return <p>{$x}</p>


