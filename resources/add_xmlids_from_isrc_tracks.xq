(: declare variable $where as xs:string := doc($fileTree)/@filePath; :)
(: declare variable $where as xs:string := string(doc($fileTree)/@filePath); :)
(: declare variable $x as xs:integer := 0; :)
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(:
Goal here is to take an un-xml:id'ed XML database, and:
- Find all tracks with an extracted ISRC number.
- Add that ISRC number as the xml:id of the track (i.e. in the <track01 ...> tag).
:)

(: Path to the AMLM database, will be passed in. :)
declare variable $input_file_path external;

(: Load all media files. :)
(:let $media_file_list := fn:doc($input_file_path)/amlm_database/playlist//exturl_media/href:)

(: Determine the set of distinct file types. :)
(:let $extension_items :=:)
(:    for $item in $media_file_list:)
(:    let $item_lwr := fn:lower-case($item):)
(:    return fn:replace($item_lwr,'.*\.([a-zA-Z0-9]*)$','$1','i'):)

(:let $distinct_categories := fn:distinct-values($extension_items):)
let $doc := fn:doc($input_file_path)/amlm_database
return
    <html>
        <body>
            <ol>
                {
                    for $x at $count in $doc//m_library_entry
                    let $tracknums := $x//m_tracks/*[fn:starts-with(fn:local-name(), "track")]
                    where exists($tracknums)
                    return
                        for $tracknum in $tracknums
                        return
                        <li title="{$tracknum/PTI_TITLE/text()}" xml:id="{$count}">
                            {$tracknum}
                        </li>
                }
            </ol>
        </body>
    </html>
