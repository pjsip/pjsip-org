<?php

$url = 'http://blog.pjsip.org/feed';
$maxitems = 5;

define('MAGPIE_DIR', './');
require_once(MAGPIE_DIR.'rss_fetch.inc');

if ( $url ) {
	$rss = fetch_rss( $url );
	//echo "Channel: " . $rss->channel['title'] . "<p>";
	$i = 0;
	foreach ($rss->items as $item) {
		$href = $item['link'];
		$title = $item['title'];
		$pubdate = $item['pubdate'];

		$date = substr($pubdate, 5, 11);
		echo "        <li><a href=\"$href\">$title <IMG SRC='images/extlink.gif' alt='link' /></a> -- on $date</li>\n";
		$i = $i + 1;
		if ($i >= $maxitems) break;
	}
}
?>

