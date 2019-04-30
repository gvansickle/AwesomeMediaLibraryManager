xquery version "1.0";

(: declare variable $where as xs:string := doc($fileTree)/@filePath; :)
(: declare variable $where as xs:string := string(doc($fileTree)/@filePath); :)
(: declare variable $x as xs:integer := 0; :)
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";
(: QXmlQuery as of Qt 5.12 still doesn't support module imports. :)
(:import module namespace functx = "http://www.functx.com" at "functx-1.0-doc-2007-01.xq";:)
declare namespace functx = "http://www.functx.com";
declare function functx:add-attributes
( $elements as element()* ,
        $attrNames as xs:QName* ,
        $attrValues as xs:anyAtomicType* )  as element()? {

    for $element in $elements
    return element { node-name($element)}
    { for $attrName at $seq in $attrNames
    return if ($element/@*[node-name(.) = $attrName])
    then ()
    else attribute {$attrName}
        {$attrValues[$seq]},
    $element/@*,
    $element/node() }
} ;

(:
Goal here is to take an un-xml:id'ed XML database, and:
- Find all tracks with an extracted ISRC number.
- Add that ISRC number as the xml:id of the track (i.e. in the <track01 ...> tag).
:)

(: Path to the AMLM database, will be passed in. :)
declare variable $input_file_path external;

(:declare function local:change($node):)
(:{:)
(:    typeswitch($node):)
(:        case element(add) return:)
(:            functx:add-attributes($node, xs:QName('att1'), 1):)
(:        case element() return:)
(:            element { fn:node-name($node) } {:)
(:                $node/@*,:)
(:                $node/node() ! local:change(.):)
(:            }:)
(:        default return $node:)
(:};:)

(: Load all media files. :)
(:let $media_file_list := fn:doc($input_file_path)/amlm_database/playlist//exturl_media/href:)

(: Determine the set of distinct file types. :)
(:let $extension_items :=:)
(:    for $item in $media_file_list:)
(:    let $item_lwr := fn:lower-case($item):)
(:    return fn:replace($item_lwr,'.*\.([a-zA-Z0-9]*)$','$1','i'):)

(:title="{$tracknum/PTI_TITLE/text()}":)

(:let $distinct_categories := fn:distinct-values($extension_items):)
let $doc := fn:doc($input_file_path)/amlm_database
return
    <html>
        <header>
        </header>
        <body>
            <ol>
                {
                    for $x in $doc//m_library_entry
                    let $tracknums := $x//m_tracks/*[fn:starts-with(fn:local-name(), "track")]
                    where exists($tracknums)
                    return

                        for $tracknum in $tracknums
                        return
                            <li>
                                {functx:add-attributes($tracknum, xs:QName('xml:id'), fn:concat("trackid_", $tracknum/m_isrc/text()))}
                            </li>
                }

            </ol>
        </body>
    </html>

