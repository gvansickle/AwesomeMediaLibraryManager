(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";

(: Path to the AMLM database, will be passed in as a bound variable. :)
declare variable $input_file_path as xs:anyURI external;
(: Regex to filter hrefs.  Intended for extensions, but not enforced. :)
declare variable $extension_regex external;

<ul>
{
(: Return URLs (as strings) to all media files in the database :)
for $x in fn:doc($input_file_path)//href
(:where fn:matches($x, '.*\.flac$'):)
where fn:matches($x, $extension_regex)
(: Return list of <href>s. :)
return
<href>
	{
	fn:string($x)
	}
</href>
}
</ul>
