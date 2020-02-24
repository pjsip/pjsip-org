/* GDCHART 0.94b  1st CHART SAMPLE  12 Nov 1998 */

/* writes gif file to stdout */

/* sample gdchart usage */
/* this will produce a 3D BAR chart */
/* this is suitable for use as a CGI */

/* for CGI use un-comment the "Content-Type" line */

#include <stdio.h>
#include <values.h>
#include <stdlib.h>
 
#include "gdc.h"
#include "gdchart.h"
#include "nettest.h"

#define PIX_PER_SEC	60

const char *err_html =
" "
;

//#define PRINTF(f,p)	printf(f,p);
#define PRINTF(f,p)

static void err(const char *title)
{
    puts("<p>");
    puts(title);
    puts("</p>\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    struct file_hdr hdr;
    struct serie {
	const char *title;
	float	   *data;
    } series[2];
    char **xlabel;
    unsigned i, sec;
    struct file_item item1, item2;
    FILE *in, *out;
    char title[256];
    char *xtext;
    unsigned long sc[2] = { 0x00FF00, 0x0000FF };
    GDC_SCATTER_T *scat;
    unsigned scat_idx = 0;
    unsigned max_yval = 20;
    unsigned xmax, ymax;

    if (argc != 3) {
	err("Argument required");
    }

    in = fopen(argv[1], "rb");
    if (!in)
	err("Unable to open file");

    if (fread(&hdr, sizeof(hdr), 1, in) != 1)
	err("Error reading file");

    series[0].title = "RX jitter";
    series[1].title = "TX jitter";

    if (hdr.count <= 0)
	err("Invalid count");
    if (hdr.count > 50 * 600)
	err("Too many count");

    series[0].data = calloc(hdr.count, sizeof(float));
    series[1].data = calloc(hdr.count, sizeof(float));
    xlabel = calloc(hdr.count, sizeof(char*));

    GDC_num_scatter_pts = hdr.rx_tot_lost + hdr.tx_tot_lost;
    if (GDC_num_scatter_pts  != 0) {
	scat = calloc(GDC_num_scatter_pts, sizeof(GDC_SCATTER_T )); 
	GDC_scatter = scat;
    }

    xtext = malloc(10);
    sec = 0;
    sprintf(xtext, "%d", sec);
    for (i=0; i<hdr.count; ++i) {
	if (i / (1000 / hdr.interval) != sec) {
	    sec = i / (1000 / hdr.interval);
	    xtext = malloc(10);
	    sprintf(xtext, "%d", sec);
	}
	xlabel[i] = xtext;
    }

    /* Skip the first few samples */
    for (i=0; i<10; ++i) {
	if (fread(&item1, sizeof(item1), 1, in) != 1)
	    err("Error reading file");

	if (item1.rx_interval != 0 && item2.tx_interval != 0)
	    break;
    }

    i = 0;
    while (fread(&item2, sizeof(item2), 1, in)==1) {
	if ((item2.rx_lost) && scat_idx<GDC_num_scatter_pts) {
	    scat[scat_idx].val   = hdr.interval;
	    scat[scat_idx].point = i;
	    scat[scat_idx].ind   = GDC_SCATTER_TRIANGLE_DOWN;
	    scat[scat_idx].color = 0x00FF00L;
	    scat[scat_idx].width = 40;
	    scat_idx++;
	}
	if ((item2.rx_lost) && scat_idx<GDC_num_scatter_pts) {
	    scat[scat_idx].val   = hdr.interval;
	    scat[scat_idx].point = i;
	    scat[scat_idx].ind   = GDC_SCATTER_TRIANGLE_UP;
	    scat[scat_idx].color = 0x0000FFL;
	    scat[scat_idx].width = 40;
	    scat_idx++;
	}

	series[0].data[i] = item2.rx_interval - item1.rx_interval;
	//if (series[0].data[i] < 0)
	//    series[0].data[i] = 0 - series[0].data[i];
	series[1].data[i] = item2.tx_interval - item1.tx_interval;

	//if (series[1].data[i] > 0)
	//    series[1].data[i] = 0 - series[1].data[i];
	++i;

	if (series[0].data[i] > max_yval)
	    max_yval = series[0].data[i];
	if (series[1].data[i] > max_yval)
	    max_yval = series[1].data[i];
	if (-series[0].data[i] > max_yval)
	    max_yval = -series[0].data[i];
	if (-series[1].data[i] > max_yval)
	    max_yval = -series[1].data[i];

	item1 = item2;
    }

    hdr.count = i - 5;

    GDC_num_scatter_pts = scat_idx;
    fclose(in);

    PRINTF("Data count=%d\n", hdr.count);
    PRINTF("Interval=%d\n", hdr.interval);
    PRINTF("Packet size=%d\n", hdr.pkt_size);
    PRINTF("Client IP=%s\n", hdr.ip);
    PRINTF("Connection type=%s\n", hdr.connection);
    PRINTF("Reporter=%s\n", hdr.reporter);
    PRINTF("City=%s\n", hdr.city);
    PRINTF("Country=%s\n", hdr.country);
    PRINTF("Browser=%s\n", hdr.user_agent);

    out = fopen(argv[2], "wb");
    if (!out)
	err("Unable to open output file");


    snprintf(title, sizeof(title), 
	     "Jitter report: by %s\n"
	     "%s\n"
	     "Duration: %d seconds\n"
	     "IP:%s, city:%s, country:%s\n"
	     "Conn:%s, ISP:%s\n"
	     "%s\n"
	     "Green: RX, Blue: TX\n"
	     "TX lost=%d, RX lost=%d",
	     hdr.reporter,
	     hdr.date_time,
	     hdr.interval * hdr.count / 1000,
	     hdr.ip, hdr.city, hdr.country,
	     hdr.connection, hdr.isp,
	     hdr.user_agent,
	     hdr.tx_tot_lost, hdr.rx_tot_lost);

    GDC_title	= title;
    GDC_xtitle	= "Time (seconds)";
    GDC_ytitle	= "Jitter (ms)";

    GDC_BGColor   = 0xFFFFFFL;                  /* backgound color (white) */
    GDC_LineColor = 0x000000L;                  /* line color      (black) */
    GDC_SetColor  = sc;                   /* assign set colors */

    GDC_xlabel_spacing = 1000 / hdr.interval;
    GDC_ylabel_density = 50;

    PRINTF("GDC_xlabel_spacing=%d\n", GDC_xlabel_spacing);

    xmax = PIX_PER_SEC*hdr.interval*hdr.count/1000;
    if (xmax > 2000) xmax = 2000;
    if (xmax < 400) xmax = 400;

    ymax = max_yval * 40;
    if (ymax > 600) ymax = 600;
    if (ymax < 400) ymax = 400;

    out_graph( xmax, ymax,      /* short       width, height */
               out,	      /* FILE*       open FILE pointer */
               GDC_LINE,      /* GDC_CHART_T chart type */
               hdr.count,     /* int         number of points per data set */
               xlabel,        /* char*[]     array of X labels */
               2,             /* int         number of data sets */
               series[0].data,/* float[]     data set 1 */
               series[1].data);/*  ...        data set n */


    fclose(out);
    return 0;
}

