/* GDCHART 0.94b  1st CHART SAMPLE  12 Nov 1998 */

/* writes gif file to stdout */

/* sample gdchart usage */
/* this will produce a 3D BAR chart */
/* this is suitable for use as a CGI */

/* for CGI use un-comment the "Content-Type" line */

#include <stdio.h>
#include <values.h>
 
#include "gdc.h"
#include "gdchart.h"
 
main()
{
    /* ----- set some data ----- */
    float   a[6]  = { 0.5, 0.09, 0.6, 0.85, 0.0, 0.90 },
            b[6]  = { 1.9, 1.3,  0.6, 0.75, 0.1, 2.0 };
    /* ----- X labels ----- */
    char    *t[6] = { "Chicago", "New York", "L.A.", "Atlanta", "Paris, MD\n(USA) ", "London" };
    /* ----- data set colors (RGB) ----- */
    unsigned long   sc[2]    = { 0xFF8080, 0x8080FF };
 
    GDC_BGColor   = 0xFFFFFFL;                  /* backgound color (white) */
    GDC_LineColor = 0x000000L;                  /* line color      (black) */
    GDC_SetColor  = &(sc[0]);                   /* assign set colors */
    FILE *out;

       GDC_SCATTER_T  scat[2];       // two scatter points
 
    scat[0].val   = 1;          // 1st scatter point Y value
    scat[0].point = 2;            // where along X axis
    scat[0].ind   = GDC_SCATTER_TRIANGLE_DOWN;
    scat[0].color = 0xFF0000L;    // RGB
    scat[0].width = 40;
    scat[1].val   = 2;          // 2nd scatter point Y value
    scat[1].point = 4;            // where along X axis
    scat[1].ind   = GDC_SCATTER_TRIANGLE_UP;
    scat[1].color = 0x00FF00L;    // RGB
    scat[1].width = 30;

    GDC_num_scatter_pts = 2;
    GDC_scatter = scat;

    out = fopen("test1.gif", "wb");

	GDC_stack_type = GDC_STACK_BESIDE;
//    printf( "Content-Type: image/gif\n\n" );    /* tell browser type */

                              /* ----- call the lib ----- */
    out_graph( 250, 200,      /* short       width, height */
               out,        /* FILE*       open FILE pointer */
               GDC_LINE,     /* GDC_CHART_T chart type */
               6,             /* int         number of points per data set */
               t,             /* char*[]     array of X labels */
               2,             /* int         number of data sets */
               a,             /* float[]     data set 1 */
               b );           /*  ...        data set n */

    fclose(out);
    return 0;
}
