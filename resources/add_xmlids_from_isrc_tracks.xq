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

(: Identity transform :)
(: Recursively return a deep copy of the element and all sub elements :)
declare function local:copy($element as element()) as element() {
    element {node-name($element)}
    {$element/@*,
    for $child in $element/node()
    return
        if ($child instance of element())
        then local:copy($child)
        else $child
    }
};
(: Same, typeswitch :)
(: Doesn't work with Qt5 :)
(:declare function local:copy_ts($n as node()) as node() {:)
(:    typeswitch($n):)
(:        case $e as element():)
(:            return:)
(:                element {name($e)}:)
(:                {$e/@*,:)
(:                for $c in $e/(* | text()):)
(:                return local:copy($c) }:)
(:        default return $n:)
(:};:)

declare function local:change-attributes($node as node(), $new-name as xs:string, $new-content as item(), $action as xs:string, $target-element-names as xs:string+, $target-attribute-names as xs:string+) as node()+ {

    if ($node instance of element())
    then
        element {node-name($node)}
        {
            if ($action = 'remove-all-empty-attributes')
            then $node/@*[string-length(.) ne 0]
            else

                if ($action = 'remove-all-named-attributes')
                then $node/@*[name(.) != $target-attribute-names]
                else

                    if ($action = 'change-all-values-of-named-attributes')
                    then element {node-name($node)}
                    {for $att in $node/@*
                    return
                        if (name($att) = $target-attribute-names)
                        then attribute {name($att)} {$new-content}
                        else attribute {name($att)} {$att}
                    }
                    else

                        if ($action = 'attach-attribute-to-element' and name($node) = $target-element-names)
                        then ($node/@*, attribute {$new-name} {$new-content})
                        else

                            if ($action = 'remove-attribute-from-element' and name($node) = $target-element-names)
                            then $node/@*[name(.) != $target-attribute-names]
                            else

                                if ($action = 'change-attribute-name-on-element' and name($node) = $target-element-names)
                                then
                                    for $att in $node/@*
                                    return
                                        if (name($att) = $target-attribute-names)
                                        then attribute {$new-name} {$att}
                                        else attribute {name($att)} {$att}
                                else

                                    if ($action = 'change-attribute-value-on-element' and name($node) = $target-element-names)
                                    then
                                        for $att in $node/@*
                                        return
                                            if (name($att) = $target-attribute-names)
                                            then attribute {name($att)} {$new-content}
                                            else attribute {name($att)} {$att}
                                    else

                                        $node/@*
            ,
            for $child in $node/node()
            return
                local:change-attributes($child, $new-name, $new-content, $action, $target-element-names, $target-attribute-names)
        }
    else $node
};

(::)
declare function local:find_and_add_track_isrc_as_id($nodes as node()*) as item()* {
    for $node in $nodes/node()
    let $isrc := $node/m_isrc
    return
        if (exists($isrc))
        then
            functx:add-attributes($node, "xml:id", $isrc/text())
        else
            local:dispatch($node/node())
(:        trace($isrc, 'm_isrc == $isrc');:)
(:            if (fn:string-length($isrc) > 0):)
(:            then:)
(:                ($node/@*, attribute {"xml:id"} {$isrc}):)
(:            else:)
(:                local:dispatch($node):)
};

(: Passthru which preserves attributes. :)
declare function local:passthru($node as node()*) as item()* {
    element {name($node)} {($node/@*, local:dispatch($node/node()))}
};

declare function local:dispatch($node as node()) as item()* {
    typeswitch($node)
        case text() return $node
        case comment() return $node
        case element() return $node
        case element(track) return local:find_and_add_track_isrc_as_id($node)
(:        case element(bill) return local:bill($node):)
(:        case element(btitle) return local:btitle($node):)
(:        case element(section-id) return local:section-id($node):)
(:        case element(bill-text) return local:bill-text($node):)
(:        case element(strike) return local:strike($node):)
        default return local:passthru($node)
};

(::)

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

(:title="{$tracknum/PTI_TITLE/text()}":)

(:let $distinct_categories := fn:distinct-values($extension_items):)

let $doc := fn:doc($input_file_path)/amlm_database
return
    local:dispatch($doc)
(:    local:change-attributes($doc, 'xml:id', 'fn:name()', 'attach-attribute-to-element', 'track', 'xml:id'):)

(:for $x in $doc//m_library_entry:)
(:let $tracknums := $x//m_tracks/*[fn:starts-with(fn:local-name(), "track")]:)
(:where exists($tracknums):)
(:return:)
(:    for $tracknum in $tracknums:)
(:    return:)
(:        functx:add-attributes($tracknum, xs:QName('xml:id'), fn:concat("trackid_", $tracknum/m_isrc/text())):)

(:return:)
(:    <html>:)
(:        <header>:)
(:        </header>:)
(:        <body>:)
(:            <ol>:)
(:                {:)
(:                    for $x in $doc//m_library_entry:)
(:                    let $tracknums := $x//m_tracks/*[fn:starts-with(fn:local-name(), "track")]:)
(:                    where exists($tracknums):)
(:                    return:)

(:                        for $tracknum in $tracknums:)
(:                        return:)
(:                            functx:add-attributes($tracknum, xs:QName('xml:id'), fn:concat("trackid_", $tracknum/m_isrc/text())):)
(:                            <li>:)
(:                                {functx:add-attributes($tracknum, xs:QName('xml:id'), fn:concat("trackid_", $tracknum/m_isrc/text()))}:)
(:                            </li>:)
(:                }:)

(:            </ol>:)
(:        </body>:)
(:    </html>:)

