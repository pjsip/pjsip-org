import java.awt.*;
import java.applet.*;
import java.io.*;
import java.net.*;
import java.util.*;

class NetData
{
    int txSeq = 0;

    int rxTot = 0;
    int rxTotLost = 0;
    int rxSeq = -1;
    int rxInterval = -1;
    long rxLastTime = -1;

    public void txPkt(byte[] buf, int size)
    {
	int pos = 0;
	int dlsr;
	long now;

	txSeq++;

	buf[pos++] = (byte)((txSeq & 0x0000FF00) >>  8);
	buf[pos++] = (byte)((txSeq & 0x000000FF) >>  0);

	buf[pos++] = (byte)((rxTot & 0x0000FF00) >> 8);
	buf[pos++] = (byte)((rxTot & 0x000000FF) >> 0);

	buf[pos++] = (byte)((rxTotLost & 0x0000FF00) >> 8);
	buf[pos++] = (byte)((rxTotLost & 0x000000FF) >> 0);

	buf[pos++] = (byte)((rxInterval & 0x0000FF00) >> 8);
	buf[pos++] = (byte)((rxInterval & 0x000000FF) >> 0);

	if (rxSeq != -1) {
	    buf[pos++] = (byte)((rxSeq & 0x0000FF00) >> 8);
	    buf[pos++] = (byte)((rxSeq & 0x000000FF) >> 0);
	} else {
	    buf[pos++] = 0;
	    buf[pos++] = 0;
	}

	if (rxLastTime == -1)
	    dlsr = 0;
	else
	    dlsr = (int)(System.currentTimeMillis() - rxLastTime);

	buf[pos++] = (byte)((dlsr & 0x0000FF00) >> 8);
	buf[pos++] = (byte)((dlsr & 0x000000FF) >> 0);

	while (pos < size)
	    buf[pos++] = (byte)0x0A;

	rxInterval = 0;
    }

    public void rxPkt(byte[] buf)
    {
	int pos = 0;
	int seq;
	long now;

	seq  = ((buf[pos++] & 0xFF) <<  8);
	seq += ((buf[pos++] & 0xFF) <<  0);

	if (rxSeq == -1) {
	    rxSeq = seq;
	} else {
	    rxTotLost += (seq - rxSeq - 1);
	}

	rxTot++;

	now = System.currentTimeMillis();
	if (seq - rxSeq == 1) {
	    if (rxLastTime == -1)
		rxInterval = 0;
	    else
		rxInterval = (int)(now - rxLastTime);
	} else {
	    rxInterval = 0;
	}

	rxLastTime = now;
	rxSeq = seq;
    }
};

class WorkerThread implements Runnable
{
    DatagramSocket sock;
    DatagramPacket pkt;
    NetData netData;
    boolean isSending;
    int sendInterval;
    int pktSize;
    byte[] buf;
    Thread thread;
    boolean running;

    public WorkerThread(DatagramSocket s, boolean sending, 
		        InetAddress addr, int port, int psz, int itv,
			NetData ndat)
    throws SocketException    
    {
	sock = s;
	isSending = sending;
	netData = ndat;
	pktSize = psz + 12;
	buf = new byte[pktSize];
	pkt = new DatagramPacket(buf, pktSize, addr, port);
	sendInterval = itv;
    }

    public void start()
    {
        running = true;
	thread = new Thread(this);
	thread.setPriority(Thread.MAX_PRIORITY);
	thread.start();
    }

    public void stop()
    {
	running = false;
	thread.stop();
	thread = null;
    }

    public void run()
    {
	if (isSending)
	    send_loop();
	else
	    recv_loop();
    }

    void send_loop()
    {
	System.err.println("Send thread running..");
	while (running) {
	    try {	
		thread.sleep(sendInterval);
		netData.txPkt(buf, pktSize);
		pkt.setData(buf, 0, pktSize);
		sock.send(pkt);
	    } catch (InterruptedException e) {
		System.err.println("InterruptedException");
		break;	    
	    } catch (IOException e) {
	    }
	}
    }

    void recv_loop()
    {
	System.err.println("Recv thread running..");
	while (running) {
	    try {	
		sock.receive(pkt);
		netData.rxPkt(pkt.getData());
	    } catch (IOException e) {
		System.err.println("IOException");
		break;	    
	    }
	}
    }
}

public class NetTest extends Applet implements Runnable
{
    DatagramSocket sock;
    NetData netData;
    WorkerThread sender;
    WorkerThread receiver;
    Thread uiThread;
    int progress = 0;
    
    void do_init(String dst, int dstPort, 
		 int payloadSize, int itv) 
    {
  	try {
	    netData = new NetData();
	    sock = new DatagramSocket();
	    InetAddress addr = InetAddress.getByName(dst);
    
	    sender = new WorkerThread(sock, true, addr, dstPort, payloadSize, itv, netData);
	    receiver = new WorkerThread(sock, false, addr, dstPort, payloadSize, itv, netData);
	} catch (SocketException e) {
	    System.out.println("Error: socket exception");
	} catch (UnknownHostException e) {
	    System.out.println("Error: unknown host exception");
	}
    }
    
    void do_start()
    {
	sender.start();
	receiver.start();
    }

    void do_stop()
    {
	sender.stop();
	receiver.stop();
    }

    void do_run()
    {
	int duration;
	int to_send;
	
	duration = Integer.parseInt(getParameter("duration"));
	to_send = duration * 1000 / Integer.parseInt(getParameter("interval"));

	System.out.println("Starting..");
    	do_start();

	System.out.println("Running..");

	while (netData.txSeq < to_send) {
	    String msg;

	    try {
		Thread.sleep(1000);
		progress = netData.txSeq * 100 / to_send;
		repaint();

	    } catch (InterruptedException e) {
		System.out.println("InterruptedException");
		break;
	    }
	}

	System.out.println("Stopping..");
	do_stop();

	System.out.println("Stopped");
    }

    public void run()
    {
	do_run();
    }

    public void paint(Graphics g)
    {
	if (progress < 100) {
	    g.setColor(Color.black);
	    g.draw3DRect(0, 0, 200, 40, true);
	    g.fill3DRect(0, 0, 200 * progress / 100, 40, true);
	    g.setColor(Color.white);
	    g.drawString(String.valueOf(progress) + "%", 1, 14);
	} else {
	    g.setColor(Color.black);
	    g.drawString("Done (" + String.valueOf(netData.txSeq) +
			 " pkts sent)", 0, 14);
	}
    }

    public void init()
    {
	String dstIp = getParameter("dst-ip");
	int dstPort = Integer.parseInt(getParameter("dst-port"));
	int payloadSize = Integer.parseInt(getParameter("payload-size"));
	int interval = Integer.parseInt(getParameter("interval"));

	System.out.println("Initializing..");

	do_init(dstIp, dstPort, payloadSize, interval);

	System.out.println("init complete");

	uiThread = new Thread(this);
	uiThread.setPriority(Thread.MIN_PRIORITY);
	uiThread.start();
    }

    public void stop()
    {
	do_stop();
    }

}

