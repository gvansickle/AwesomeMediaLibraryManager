xquery version "1.0";

(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Path to the AMLM database, will be passed in as a bound variable. :)
declare variable $input_file_path as xs:anyURI external;

let $doc := fn:doc($input_file_path)/amlm_database

for $libentry in $doc//m_library_entry
for $album_names in $libentry//m_tm_generic/entry/key[data(.)="ALBUM"]/../values/value
return $album_names




