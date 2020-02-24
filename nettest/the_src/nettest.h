struct file_hdr
{
    unsigned	ver;
    char	ip[32];
    char	reporter[32];
    char	connection[16];
    char	city[16];
    char	country[16];
    char	date_time[32];
    char	user_agent[64];
    char	isp[32];
    char	reserved[256];

    unsigned	count;
    unsigned	interval;
    unsigned	pkt_size;

    unsigned	rx_tot;
    unsigned	rx_tot_lost;

    unsigned	tx_tot;
    unsigned	tx_tot_lost;
};

struct file_item
{
    unsigned short rx_interval;
    unsigned short tx_interval;
    unsigned char  rx_lost;
    unsigned char  tx_lost;
};


