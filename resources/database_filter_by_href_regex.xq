xquery version "1.0";
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Path to the AMLM database, will be passed in as a bound variable. :)
declare variable $input_file_path as xs:anyURI external;
declare variable $input_doc_node as document-node() := doc($input_file_path);
(: Regex to filter hrefs.  Intended for extensions, but not enforced. :)
declare variable $extension_regex external;

(: ========================= Count :)
let $href_sequence :=
	for $x in $input_doc_node//m_dsr//m_url
	where fn:matches($x, '.*\.flac$')
	return $x
return
	<h1>{ fn:count($href_sequence) }</h1>
(: ========================= :)

(:<ul>:)
(:{:)
(:	:)(: Return URLs (as strings) to all media files in the database :)
(:	for $x in $input_doc_node//HREF:)
(:	where fn:matches($x, '.*\.flac$'):)
(:	:)(:where fn:matches($x, $extension_regex):)
(:	:)(: Return list of <href>s. :)
(:	return:)
(:	<h1>{ fn:string($x) }</h1>:)

(:<href>:)
(:	{:)
(:		fn:string($x):)
(:	}:)
(:</href>:)
(:let $ct := fn:count($x):)
(:	{:)

(:	fn:string($ct):)
(:}:)
(:}:)
(:</ul>:)
