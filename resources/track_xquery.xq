xquery version "1.0";

(: The FunctX namespace :)
(:declare namespace functx = "http://www.functx.com";:)
(: @todo Need to move away from this. The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: declare namespace xmlns="http://www.w3.org/1999/xhtml/"; :)
(:declare namespace xlink = "http://www.w3.org/1999/xlink";:)
(:declare namespace fn = "http://www.w3.org/2005/xpath-functions";:)

(: Path to the playlist, will be passed in. :)
declare variable $input_playlist as xs:anyURI external;
declare variable $doc_node_playlist as document-node() := doc($input_playlist);

(: Path to the AMLM database, will be passed in as a bound variable. :)
declare variable $input_file_path as xs:anyURI external;
declare variable $db_doc_node as document-node() := doc($input_file_path);

let $doc_database := $db_doc_node/amlm_database
let $doc_playlist := $doc_node_playlist/amlm_playlist

(:<body>:)
(:	{:)
(:	:)(:for $genre in //genre/choice:)
(:	:)(:for $video in //video:)
(:	for $trackRefs in $doc_playlist//track_ref:)
(:	return:)
(:	<list>:)
(:	{$trackRefs/text()}:)
(:	</list>:)
(:}:)


(:</body>:)

(::)
(:for $trackRefs in $doc_playlist//track_ref:)
	for $track in $doc_database//track[@xml:id="xmlid_02991e09-4058-43ab-8dfd-6c9911438923"]
(:	for $libentry in $doc_database//m_library_entry:)
	return
	<list>
	{$track//entry}
	</list>

(:	:)(: order by $x/title :)
(:	:)(:return <list>:)
(:	:)(:		<a>{$x/books/book[fn:id($x//book//@xml:idref)]/description[text()]}</a>:)
(:	:)(:		<b>{$x//title[text()]}</b>:)
(:	:)(:		<c>{$x//title[text()]}</c>:)
(:	:)(:	</list>:)
(:	:)
