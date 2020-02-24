
<TABLE class="news" cellSpacing="0" cellPadding="2" border="1">
<TR bgColor="#000099">
<TH>
<P align="center">
<STRONG>
<FONT color="#ffffff">
<A style="color: white;" HREF="http://trac.pjsip.org/repos/timeline?changeset=on&max=50&daysback=90&format=rss"><IMG border="0" SRC="/images/feed-icon-16x16.png" ALT="rss">&nbsp; Repository Checkins</a>
</FONT></STRONG></P>
</TH>
</TR>

<!--
<TR bgColor="#000099"> <TD> <P align="center"> <A HREF="http://blog.pjsip.org/feed"> <IMG SRC="/images/add-rss20.png" ALT="rss"> </A> </P> </TD> </TR>
-->

<?php

$url = 'http://trac.pjsip.org/repos/timeline?changeset=on&max=50&daysback=90&format=rss';
$maxitems = 15;

define('MAGPIE_DIR', './');
require_once(MAGPIE_DIR.'rss_fetch.inc');

if ( $url ) {
	$rss = fetch_rss( $url );
	//echo "Channel: " . $rss->channel['title'] . "<p>";
	$i = 0;
	foreach ($rss->items as $item) {
		echo "<TR><TD>";
		$href = $item['link'];
		$title = $item['title'];
		$pubdate = $item['pubdate'];

		echo "<i>";
		echo substr($pubdate, 0, 22);
		echo "</i>: ";
		echo "<a href=\"$href\">$title</a>";
		//echo "<BR>";
		//echo substr($item['description'], 0, 80);
		//echo "... <BR>\n";

		echo "</TD></TR>\n";

		$i = $i + 1;
		if ($i >= $maxitems) break;
	}
}
?>


</TABLE>
