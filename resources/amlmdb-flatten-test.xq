xquery version "1.0";

(: The FunctX namespace :)
declare namespace functx = "http://www.functx.com";
(: @todo Need to move away from this. The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";


(: Path to the AMLM database, will be passed in as a bound variable. :)
declare variable $input_file_path as xs:anyURI external;

let $doc := fn:doc($input_file_path)/amlm_database

for $libentry in $doc//m_library_entry
for $summary_tags in $libentry//m_tm_generic
(:let $amlmtagnames := ("ALBUM", "ARTIST"):)
let $album_name := $summary_tags/entry/key[data(.)="ALBUM"]/../values/value/text()
let $artist_name := $summary_tags/entry/key[data(.)="ARTIST"]/../values/value/text()
(:for $album_names in $summary_tags/entry/key[data(.)="ALBUM"]/../values/value:)
(:return $summary_tags/entry/key[data(.)="ALBUM"]/../values/value/text():)
return
	$libentry
(:	<li>"ALBUM: {$album_name} , ARTIST: {$artist_name}"</li>:)
