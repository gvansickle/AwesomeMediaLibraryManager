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

(: Determine the set of distinct file types. :)
let $extension_items :=
    for $item in $media_file_list
    let $item_lwr := fn:lower-case($item)
    return fn:replace($item_lwr,'.*\.([a-zA-Z0-9]*)$','$1','i')

let $distinct_categories := fn:distinct-values($extension_items)

return
    <html>
        <body>
            <ol>
                {
                    for $x at $count in fn:doc($input_file_path)/amlm_database/playlist//exturl_media/href
                    where (matches($x, '.*\.flac$'))
                    return
                        <li id="{$count}">
                            {data($x)}
                        </li>
                }
            </ol>
            <p>Count = {fn:count($media_file_list)}</p>
            <tbody>
                {
                    for $file_type in $distinct_categories
                    return
                        <tr>
                            <td>{$file_type}</td>
                            <td>000</td>
                        </tr>
                }
            </tbody>
        </body>
    </html>
