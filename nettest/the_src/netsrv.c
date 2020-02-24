#include <pjlib.h>
#include <time.h>
#include <pj/compat/socket.h>
#include "nettest.h"

#define VAR_PSZ		"%d"
#define VAR_PTIME	"%d"
#define VAR_DUR		"%d"
#define VAR_REPORTER	"%s"
#define VAR_ISP		"%s"
#define VAR_CITY	"%s"
#define VAR_COUNTRY	"%s"
#define VAR_CONN	"%s"

#define VAR_DST_IP	"%s"
#define VAR_DST_PORT	"%d"
#define VAR_PAYLOAD_SZ	"%d"
#define VAR_INTERVAL	"%d"
#define VAR_DURATION	"%d"

const char *html =
"Content-Type: text/html\r\n"
"\r\n"
"<HTML>\n"
"<BODY>\n"
"  <applet code=NetTest.class width=\"200\" height=\"20\">\n"
"    <PARAM NAME=\"cache_option\" VALUE=\"No\">\n"
"    <PARAM NAME=\"cache_archive\" VALUE=\"\">\n"
"    <param name=\"dst-ip\" value=\"" VAR_DST_IP "\">\n"
"    <param name=\"dst-port\" value=\"" VAR_DST_PORT "\">\n"
"    <param name=\"payload-size\" value=\"" VAR_PAYLOAD_SZ "\">\n"
"    <param name=\"interval\" value=\"" VAR_INTERVAL "\">\n"
"    <param name=\"duration\" value=\"" VAR_DURATION "\">\n"
"  </applet>\n"
"\n"
"<p>Test is in progress, please do not use computer or internet..</p>\n\n\n"
;

#define MAX_PKT		600
#define MIN_DURATION	5
#define MAX_DURATION	600

//#define DEB(msg,param)	printf("<p>" msg "</p>\n", param); fflush(stdout)
#define DEB(msg,param)

typedef struct packet
{
    int	tx_seq;
    int	rx_tot;
    int	rx_tot_lost;
    int	rx_interval;
    int	rx_seq;
    int	dlsr;
} packet;

static void test_data_to_pkt(packet *t, pj_uint8_t *pkt, int size)
{
    int pos = 0;
    int dlsr = 0;

    pkt[pos++] = (pj_uint8_t)((t->tx_seq & 0x0000FF00) >>  8);
    pkt[pos++] = (pj_uint8_t)((t->tx_seq & 0x000000FF) >>  0);

    pkt[pos++] = (pj_uint8_t)((t->rx_tot & 0x0000FF00) >> 8);
    pkt[pos++] = (pj_uint8_t)((t->rx_tot & 0x000000FF) >> 0);

    pkt[pos++] = (pj_uint8_t)((t->rx_tot_lost & 0x0000FF00) >> 8);
    pkt[pos++] = (pj_uint8_t)((t->rx_tot_lost & 0x000000FF) >> 0);

    pkt[pos++] = (pj_uint8_t)((t->rx_interval & 0x0000FF00) >> 8);
    pkt[pos++] = (pj_uint8_t)((t->rx_interval & 0x000000FF) >> 0);

    pkt[pos++] = (pj_uint8_t)((t->rx_seq & 0x0000FF00) >> 8);
    pkt[pos++] = (pj_uint8_t)((t->rx_seq & 0x000000FF) >> 0);

    pkt[pos++] = (pj_uint8_t)((dlsr & 0x0000FF00) >> 8);
    pkt[pos++] = (pj_uint8_t)((dlsr & 0x000000FF) >> 0);

    while (pos < size)
	pkt[pos++] = (pj_uint8_t)0x0A;
}

static void test_data_from_pkt(packet *t, pj_uint8_t *buf)
{
    int pos = 0;

    t->tx_seq = (buf[pos++] <<  8);
    t->tx_seq += (buf[pos++] <<  0);

    t->rx_tot = (buf[pos++] <<  8);
    t->rx_tot += (buf[pos++] <<  0);

    t->rx_tot_lost = (buf[pos++] <<  8);
    t->rx_tot_lost += (buf[pos++] <<  0);

    t->rx_interval = (buf[pos++] <<  8);
    t->rx_interval += (buf[pos++] <<  0);

    t->rx_seq = (buf[pos++] <<  8);
    t->rx_seq += (buf[pos++] <<  0);

    t->dlsr = (buf[pos++] <<  8);
    t->dlsr += (buf[pos++] <<  0);
}


static struct test_param_t
{
    char    *dst_ip;
    int	     dst_port;
    int	     payload_size;
    int	     interval;
    int	     duration;

    char     filename[64];
} test_param;

struct test_session
{
    pj_pool_t	    *pool;
    pj_sock_t	    sock;
    pj_sockaddr_in  client_addr;
    pj_bool_t	    running;
    pj_thread_t	    *send_thread;
    pj_thread_t	    *recv_thread;
    
    FILE	    *out;
    struct file_hdr hdr;

    int		    tx_seq;
    int		    tx_tot_rx;
    int		    tx_tot_lost;
    int		    tx_interval;

    int		    rx_tot;
    int		    rx_lost;
    int		    rx_interval;
    int		    rx_seq_heard;
};

#define C(st,op)	status = op; \
			if (status != PJ_SUCCESS) err(st, #op, status)

static void err(int st, const char *title, pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];
    pj_strerror(status, errmsg, sizeof(errmsg));

    if (st==0) {
	printf("Content-Type: text/html\r\n"
	     "\r\n"
	     "<HTML><HEAD><TITLE>Error</TITLE></HEAD>\r\n"
	     "<BODY>%s: %s", title, errmsg);
    } else {
	printf("%s: %s", title, errmsg);	
    }
    puts("</BODY></HTML>");
    exit(0);
}

static int send_thread(void *p)
{
    struct test_session *sess = (struct test_session*)p;
    pj_status_t status;
    
    DEB("Sender thread started", 0);

    while (sess->running) {
	pj_ssize_t len;
	pj_uint8_t pkt[MAX_PKT];
	packet data;

	C( 1, pj_thread_sleep(test_param.interval) );

	++sess->tx_seq;

	pj_bzero(&data, sizeof(data));
	data.tx_seq = sess->tx_seq;
	data.rx_tot = sess->rx_tot;
	data.rx_tot_lost = sess->rx_lost;
	data.rx_interval = sess->rx_interval;
	data.rx_seq = sess->rx_seq_heard;
	data.dlsr = 0;

	len = test_param.payload_size + 12;
	pj_bzero(pkt, len);
	test_data_to_pkt(&data, pkt, len);

	pj_sock_sendto(sess->sock, pkt, &len, 0,
		       &sess->client_addr, sizeof(sess->client_addr));
    }

    DEB("Sender thread exited", 0);
    return 0;
}

static int recv_thread(void *p)
{
    struct test_session *sess = (struct test_session*)p;
    pj_time_val last_rx;
    struct timeval timeout;
    pj_status_t status;

    last_rx.sec = last_rx.msec = 0;

    DEB("Receiver thread started, sess->sock=%d", sess->sock);

    /* Set recv timeout */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(sess->sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&timeout,
	       sizeof(struct timeval));

    while (sess->running) {
	pj_uint8_t pkt[MAX_PKT];
	int addr_len;
	pj_ssize_t len;
	int rc;

	len = sizeof(pkt);
	addr_len = sizeof(sess->client_addr);
	status = pj_sock_recvfrom(sess->sock, pkt, &len, 0, 
			          &sess->client_addr, &addr_len);
	if (status != PJ_SUCCESS || len < 1)
	    continue;

	if (!sess->running)
	    break;

	if (sess->send_thread == NULL) {
	    /* Start sender thread upon receiving first packet 
	     * (as we need to know client_addr first.
	     */
	    C( 1, pj_thread_create(sess->pool, "sender", &send_thread, sess, 0, 0, 
			        &sess->send_thread) );
	} else {
	    pj_time_val now;
	    struct file_item item;
	    packet p;

	    pj_gettimeofday(&now);

	    test_data_from_pkt(&p, pkt);

	    if (last_rx.sec == 0) {
		pj_gettimeofday(&last_rx);
	    } else {
		pj_time_val dur = now;
		PJ_TIME_VAL_SUB(dur, last_rx);
		sess->rx_interval = (short)PJ_TIME_VAL_MSEC(dur);
		last_rx = now;
	    }

	    if (sess->rx_seq_heard == -1) 
		sess->rx_seq_heard = p.tx_seq - 1;

	    if (p.tx_seq <= sess->rx_seq_heard) {
		sess->rx_seq_heard = p.tx_seq;
		continue;
	    }

	    sess->rx_tot++;
	    item.rx_lost = (p.tx_seq - sess->rx_seq_heard - 1);
	    sess->rx_lost += item.rx_lost;
	    sess->rx_seq_heard = p.tx_seq;
	    
	    sess->tx_tot_rx = p.rx_tot;
	    if (p.rx_tot_lost > sess->tx_tot_lost) {
		item.tx_lost = p.rx_tot_lost - sess->tx_tot_lost;
		sess->tx_tot_lost = p.rx_tot_lost;
	    } else {
		item.tx_lost = 0;
	    }
	    sess->tx_interval = p.rx_interval;
	    
	    /*
	    printf("<p>RX stream tot=%d, last_seq=%d, lost=%d, interval=%d<BR>\n"
		   "   TX stream sent=%d tot=%d, last_seq=%d, lost=%d, interval=%d<BR>\n</p>\n",
		   sess->rx_tot, sess->rx_seq_heard, 
		   sess->rx_lost, sess->rx_interval,
		   sess->tx_seq,
		   p.rx_tot, p.rx_seq, p.rx_tot_lost, p.rx_interval);
	     */

	    if (sess->out) {

		item.rx_interval = sess->rx_interval;
		item.tx_interval = p.rx_interval;

		fwrite(&item, sizeof(item), 1, sess->out);

		sess->hdr.count++;
		sess->hdr.rx_tot = sess->rx_tot;
		sess->hdr.rx_tot_lost = sess->rx_lost;
		sess->hdr.tx_tot = p.rx_seq;
		sess->hdr.tx_tot_lost = p.rx_tot_lost;
	    }
	}
    }

    DEB("Receiver thread exited", 0);
    return 0;
}

static int server(struct test_session *sess)
{
    pj_status_t status;
    time_t t, end_time;
    pj_sockaddr_in addr;
    int addr_len;
    
    sess->running = PJ_TRUE;
    sess->rx_seq_heard = -1;

    /* Prepare socket */
    C( 0, pj_sock_socket(pj_AF_INET(), pj_SOCK_DGRAM(), 0, &sess->sock) );

    test_param.dst_ip = getenv("SERVER_NAME");

    pj_bzero(&addr, sizeof(addr));
    addr.sin_family = pj_AF_INET();
    //addr.sin_addr.s_addr = inet_addr(test_param.dst_ip);
    C( 0, pj_sock_bind(sess->sock, &addr, sizeof(addr)) );

    addr_len = sizeof(addr);
    C( 0, pj_sock_getsockname(sess->sock, &addr, &addr_len) );

    test_param.dst_port = pj_ntohs(addr.sin_port);

    /* We can now write the applet */
    printf(html, 
	   test_param.dst_ip, test_param.dst_port,
	   test_param.payload_size, test_param.interval, 
	   test_param.duration);
    fflush(stdout);

    DEB("Main thread sess->sock=%d", sess->sock);

    /* Prepare report file */
    {
	char path[128];

	DEB("Opening data file", 0);

	snprintf(sess->hdr.ip, sizeof(sess->hdr.ip), "%s", getenv("REMOTE_ADDR"));
	snprintf(sess->hdr.user_agent, sizeof(sess->hdr.user_agent), "%s", 
		 getenv("HTTP_USER_AGENT"));

	snprintf(test_param.filename, sizeof(test_param.filename),
		 "%s-%u", 
		 getenv("REMOTE_ADDR"), time(NULL));

	snprintf(path, sizeof(path), 
		 "reslt/%s.dat",
		 test_param.filename);

	sess->out = fopen(path, "wb");
	if (sess->out== NULL) {
	    status = pj_get_os_error();
	    err(1, "unable to create data file", status);
	}

	sess->hdr.interval = test_param.interval;
	sess->hdr.pkt_size = test_param.payload_size + 12;

	fwrite(&sess->hdr, sizeof(sess->hdr), 1, sess->out);
    }

    /* Start receiver thread */
    C( 1, pj_thread_create(sess->pool, "recv", &recv_thread, sess, 0, 0, 
			&sess->recv_thread) );

    /* Wait until complete */
    t = time(NULL);
    end_time = t + test_param.duration*5/2;

    DEB("Main thread waiting for max %d seconds", end_time - t);

    while (t < end_time) {
	DEB("Received: %d", sess->rx_tot);
	if (sess->rx_tot + 20 >= test_param.duration * 1000 / test_param.interval)
	    break;
	pj_thread_sleep(1000);
	t = time(NULL);
    }

    /* Kill threads */
    DEB("Send/recv done.", 0);
    sess->running = PJ_FALSE;
    DEB("Waiting send_thread to complete", 0);
    if (sess->send_thread) {
	pj_thread_join(sess->send_thread);
    }
    pj_sock_close(sess->sock);
    DEB("Waiting recv_thread complete", 0);
    if (sess->recv_thread) {
	pj_thread_join(sess->recv_thread);
    }

    /* Rewrite header */
    DEB("Rewrite header..", 0);
    if (sess->out) {
	fseek(sess->out, 0, SEEK_SET);
	fwrite(&sess->hdr, sizeof(sess->hdr), 1, sess->out);
	fclose(sess->out);
    }

    if (sess->hdr.count == 0) {
	printf("<p>Error: the server receives no packets from the Java client. There's probably some problem with the Java version installed in the client (hey, it's Java!)</p></BOD></HTML>\n");
	return 0;
    }

    printf("<p>Test complete, %d packets done. Now processing the results..</p>\n",
	   sess->hdr.count);

    /* Create charts */
    {
	char cmdline[128];

	snprintf(cmdline, sizeof(cmdline),
	         "./netchart.cgi reslt/%s.dat charts/%s.gif",
		 test_param.filename, test_param.filename);
	system(cmdline);
	//printf("<p>cmdline: %s</p>\n", cmdline);
    }

    printf("<H3><A HREF=\"charts/%s.gif\">Direct URL to your result below</A></H3>\n",
	  test_param.filename);
    printf("<H3><A HREF=\"mailto:bennylp@pjsip.org?subject=My%%20NetTest%%20"
	    "Result&body=Find%%20below%%20my%%20result:%%0d%%0a"
	    "http://%s/nettest/charts/%s.gif\">"
	    "Mail your result to me</A></H3>", 
	    getenv("SERVER_NAME"),
	    test_param.filename);

    puts("<p>Your result:</p>\n");
    printf("<IMG ALT=\"\" SRC=\"charts/%s.gif\">\n",
	    test_param.filename);

    printf(
	"  <!-- "
	" Payload size=" VAR_PSZ ","
	" Ptime=" VAR_PTIME ","
	" Duration=" VAR_DUR ","
	" Reporter=" VAR_REPORTER ","
	" ISP=" VAR_ISP ","
	" City=" VAR_CITY ","
	" Country=" VAR_COUNTRY ","
	" Connection=" VAR_CONN ""
	" --!>\n",
	   test_param.payload_size,
	   test_param.interval,
	   test_param.duration,
	   sess->hdr.reporter,
	   sess->hdr.isp,
	   sess->hdr.city,
	   sess->hdr.country,
	   sess->hdr.connection);

    printf("<p>Thank you for participating in this survey.</p>\n");

    puts("</BODY></HTML>\n");
    fflush(stdout);
    return 0;
}

/*
 * Mandatory query parameters:
 *
 *  psz	    Payload size (bytes)
 *  itv	    Interval (msec)
 *  dur	    Duration (second)
 *
 * Optional query parameters:
 *  con	    Connection type (string)
 *  nm	    Reporter name (string)
 *  cty	    City (string)
 *  cou	    Country (string)
 *  info    Other info (string)
 */
static void parse_query(struct test_session *sess)
{
    const char *cq;
    char q[512];
    char *name, *value;
    time_t t;
    int len;

    sess->hdr.ver = 1;

    test_param.duration = 10;
    test_param.payload_size = 16;
    test_param.interval = 20;

    cq = getenv("QUERY_STRING");
    if (cq == NULL)
	err(0, "error: invalid QUERY_STRING", PJ_EINVAL);

    strncpy(q, cq, sizeof(q));
    q[sizeof(q)-1] = '\0';

    name = strtok(q, "=");
    if (name)
	value = strtok(NULL, "&");
    else
	value = NULL;

    while (name && value) {
	if (strcmp(name, "psz") == 0) {
	    test_param.payload_size = atoi(value);
	    if (test_param.payload_size < 10 || test_param.payload_size > 640)
		err(0, "INVALID psz VALUE", PJ_EINVAL);
	} else if (strcmp(name, "itv") == 0) {
	    test_param.interval = atoi(value);
	    if (test_param.interval < 10 || test_param.interval > 1000)
		err(0, "INVALID itv VALUE", PJ_EINVAL);
	} else if (strcmp(name, "dur") == 0) {
	    test_param.duration = atoi(value);
	    if (test_param.duration < MIN_DURATION || test_param.duration > MAX_DURATION)
		err(0, "INVALID dur VALUE", PJ_EINVAL);
	} else if (strcmp(name, "con") == 0) {
	    strncpy(sess->hdr.connection, value, sizeof(sess->hdr.connection));
	    sess->hdr.connection[sizeof(sess->hdr.connection)-1] = '\0';
	} else if (strcmp(name, "nm") == 0) {
	    strncpy(sess->hdr.reporter, value, sizeof(sess->hdr.reporter));
	    sess->hdr.reporter[sizeof(sess->hdr.reporter)-1] = '\0';
	} else if (strcmp(name, "isp") == 0) {
	    strncpy(sess->hdr.isp, value, sizeof(sess->hdr.isp));
	    sess->hdr.isp[sizeof(sess->hdr.isp)-1] = '\0';
	} else if (strcmp(name, "cty") == 0) {
	    strncpy(sess->hdr.city, value, sizeof(sess->hdr.city));
	    sess->hdr.city[sizeof(sess->hdr.city)-1] = '\0';
	} else if (strcmp(name, "cou") == 0) {
	    strncpy(sess->hdr.country, value, sizeof(sess->hdr.country));
	    sess->hdr.country[sizeof(sess->hdr.country)-1] = '\0';
	}

	name = strtok(NULL, "=");
	if (name)
	    value = strtok(NULL, "&");
	else
	    value = NULL;
    }

    t = time(NULL);
    strncpy(q, ctime(&t), sizeof(q));
    len = strlen(q);
    while (q[len-1] == '\r' || q[len-1] == '\n') {
	q[--len] = '\0';
    }
    strncpy(sess->hdr.date_time, q, sizeof(sess->hdr.date_time));
}

int main()
{
    struct test_session sess;
    pj_status_t status;
    pj_caching_pool cp;

    pj_log_set_level(0);

    pj_bzero(&sess, sizeof(sess));

    parse_query(&sess);

    C( 0, pj_init() );
    pj_caching_pool_init(&cp, NULL, 0);
    sess.pool = pj_pool_create(&cp.factory, "netsrv", 1000, 1000, NULL);
    
    server(&sess);

    pj_pool_release(sess.pool);
    pj_caching_pool_destroy(&cp);
    pj_shutdown();
    return 0;
}

