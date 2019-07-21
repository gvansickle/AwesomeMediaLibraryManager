xquery version "1.0";

declare namespace fn = "http://www.w3.org/2005/xpath-functions";
(: @todo Remove this, XSPF isn't what we're using here. :)
declare default element namespace "http://xspf.org/ns/0/";
(: QXmlQuery as of Qt 5.12 still doesn't support module imports, so we'll have to copy/paste what we need. :)
(:import module namespace functx = "http://www.functx.com" at "functx-1.0-doc-2007-01.xq";:)
declare namespace functx = "http://www.functx.com";

(:
: User: gary
: Date: 7/20/19
: Time: 6:51 PM
: To change this template use File | Settings | File Templates.
:)

(: Cross-file join: :)
(:for $item in doc("order.xml")//item:)
(:let $name := doc("catalog.xml")//product[number = $item/@num]/name:)
(:return<item num="{$item/@num}" name="{$name}" quan="{$item/@quantity}"/>:)


(: Path to the AMLM database, will be passed in. :)
declare variable $input_file_path as xs:anyURI external;
(:declare variable $input_playlist as xs:anyURI external;:)
(:declare variable $input_track_db := "/home/gary/src/AwesomeMediaLibraryManager/resources/dummyplaylist.xml";:)

let $indoc := fn:doc($input_file_path)/amlm_database
(:for $track in fn:doc($input_track_db)//trackref:)
(:	m_library_entry[m_is_populated="true"],:)
(:let $db_track_entry := $indoc//track/id(@refs):)
(:	xmlid_0fb93aff-1f08-4888-bdaa-830e59b6bf1a]:)
(:	$album_pair in $libentry//entry[pair[1] = "ARTIST"]:)
return
	<p>{$indoc//id("xmlid_f837d0ac-a9ea-4d7c-b191-4677a12e4587")}</p>
(:		Track: {$track}:)
(:		Track2: {$db_track_entry}:)

(: Select the first element of each pair, which also matches "ALBUM" :)
(: album="{$v//pair[fn:position() = 1 and fn:string(.) = "ALBUM"]}" :)
(:	<artists>:)
(:		{:)
(:		:)(:            for $a in $doc//actor[@id = $v/actorRef]:)
(:		:)(:            for $a in $album_pair :)(::)(: //entry[pair[2]] :)
(:			let $a := $album_pair/pair[2]:)
(:			return:)
(:				<artist>:)
(:					<name>{$a}</name>:)
(:				</artist>:)
(:		}:)
(:	</artists>:)
