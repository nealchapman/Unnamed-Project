/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  SRV1Console.java - base station console for SRV-1 robot
 *    Copyright (C) 2005-2008  Surveyor Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details (www.gnu.org/licenses)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 
import java.awt.*;
import java.util.*;
import java.io.*;
import java.net.*;
import java.awt.image.*;
import java.awt.color.*;
import java.awt.event.*;
import java.applet.*;
//import javax.comm.*;
import gnu.io.*;

import com.surveyor.wstreamd.*;

public class SRV1Console extends Canvas implements ActionListener, KeyListener
{
    private static final boolean DEBUG = true;
    private OutputStream debug = null;

    private static Map props = new HashMap();

    private static boolean connectOnStartup = false;
    private MediaTracker tracker = null;

    private String comPort = null;

    private Socket s = null;
    private SerialPort serialPort = null;
    private OutputStream out = null;
    private InputStream is = null;

    private SRVEventLoop eventLoop = null;
    private FrameDecoder frameDecoder = null;
    private LogService logService = null;
    private NotifyService notifyService = null;
    private Map notifyTokens = new HashMap();

    private WCSPush wcsPush = null;
    private boolean shouldRun = true;
    private boolean idle = true;

    private java.util.List commandQ = Collections.synchronizedList(new ArrayList());

    static int width = 80;
    static int height = 192;
    static int size = width * height;
    Image image;
    int pixels[] = new int[size];
    MemoryImageSource source;

    private static Frame f = new Frame ("SRV-1 Console");

    private static final String CONFIG_FILE = "srv.config";
    private static final String NOTIFY_CONFIG_FILE = "srv_notify.config";

    private static String LINESEP = System.getProperty ("line.separator");
    private static TextArea log =
    new TextArea ("[Surveyor SRV-1 Console]" + LINESEP + LINESEP, 10, 25, 
              TextArea.SCROLLBARS_VERTICAL_ONLY);
    private static Dialog logWindow = new Dialog(f, "SRV-1 Console - Event Log");;
    private static SRV1Console bv = new SRV1Console();

    private Map cmdButtons = new HashMap();
    private Map commands = new HashMap();

    private int frameX = 0;   // coordinates of top left corner of frame
    private int frameY = 0;

    private Cursor target;

    private int[] scanHits = new int[80];
    private static final Color SCAN_COLOR = new Color(255, 0, 0, 120); // R,G,B,A
    private static boolean scanEnabled = false;

    static
    {
    log.setEditable (false);

    logWindow.setSize(500, 300);
    logWindow.add(log);
    logWindow.hide();

    logWindow.addWindowListener (new WindowAdapter () {
        public void windowClosing (WindowEvent evt)
        {
            logWindow.hide();
        }
        });
    } 

    public SRV1Console()
    {
    if (DEBUG) {
        try {
        debug = new FileOutputStream("debug.bin");
        } catch (Exception e) { e.printStackTrace(); }
    }
    }

    public static SRV1Console getInstance() { 
    return bv;
    }



    public static void main (String[]args)
    {
    bv.loadConfig();

    // read command line parameters
    if (args.length > 0) {
        for (int i = 0; i < args.length ; i++) {
        if ("-c".equalsIgnoreCase(args[i])) {
            connectOnStartup = true;
        } else if ("-s".equalsIgnoreCase(args[i])) {
            scanEnabled = true;
        } else if ("-p".equalsIgnoreCase(args[i])) {
            int port = -1;
            try { port = Integer.parseInt(args[i + 1]); } catch (Exception e) {}
            if (port > 0)
            props.put("wcs.port", String.valueOf(port));
        }
        }
    }

    f.setBackground(Color.WHITE);
    f.setLayout (new BorderLayout(3, 3));
    f.add("Center", bv);
    bv.init();
    f.pack();
    f.show();
    f.repaint();
    } 

    public void repaint ()
    {
    paint(this.getGraphics());
    }

    public void paint (Graphics g)
    {
    int cW = getWidth();
    int cH = getHeight();

    if (g != null && image != null) {
        int iW = image.getWidth(null);
        int iH = image.getHeight(null);
        
        if (iW != width || iH != height) {

        g.clearRect(0, 0, cW, cH);
        width = iW;
        height = iH;
        setSize(iW, iH);
        f.pack();
        target.recenter(width, height);
        }

        frameX = (cW - iW) / 2;
        frameY = (cH - iH) / 2;

        g.drawImage(image, frameX, frameY, null);
        g.setXORMode(Color.WHITE);
        target.draw(frameX, frameY, g);
        g.setPaintMode();

        paintScanOverlay(g);

    } else {
        //trace("paint() - null, g: " + g + ", image: " + image);
    }
    }

    private void paintScanOverlay(Graphics g)
    {
    if (!scanEnabled) return;

    for (int i = 0; i < scanHits.length; i++) {
        g.setColor(SCAN_COLOR);
        int x = frameX + (2 * i);
        int y = height - 2 - (2 * scanHits[i]);
        
        g.drawLine(x, y, x, y + 2);
        g.drawLine(x + 1, y, x + 1, y + 2);
    }
    }

    private void initRawBuffer()
    {
    source = new MemoryImageSource(width, height, pixels, 0, width);
    source.setAnimated(true);
    image = createImage(source);
    }

    public void init()
    {
    tracker = new MediaTracker(this);

    // The window has become active. The item
    // that wishes to have the initial focus must request it here.
    // Note, requestFocusInWindow is prefered to requestFocus in Java 1.4.
        f.addWindowListener(new WindowAdapter() {

            private void setupKeyListener() {
        String enableKB = (String) props.get("enable_keyboard");
        if (enableKB == null)
            enableKB = (String) props.get("ENABLE_KEYBOARD");
        
        if (enableKB == null || "1".equals(enableKB)) {
            // clear any old listeners, to prevent duplicate events
            KeyListener[] kl = bv.getKeyListeners();
            for (int i = 0; i < kl.length; i++) {
            bv.removeKeyListener(kl[i]);
            }

            bv.addKeyListener(bv);
        }
            
        }

            public void windowOpened(WindowEvent e) {
        setupKeyListener();
        bv.requestFocusInWindow();
            }

            public void windowActivated(WindowEvent e) {
        bv.requestFocusInWindow();
            }

        }) ;


    // handle window close
    f.addWindowListener (new WindowAdapter () {
        public void windowClosing (WindowEvent evt) {
            shutdown();
            System.exit(0);
        }
        });


    eventLoop = new SRVEventLoop();
    eventLoop.start();

    frameDecoder = new FrameDecoder();
    frameDecoder.start();

    logService = new LogService();
    logService.start();

    notifyService = new NotifyService();
    notifyService.start();

    Thread wcs = new WCSServer();
    wcs.start();

    wcsPush = new WCSPush ((String) props.get("wcs.server"),
                   (String) props.get("wcs.port"),
                   (String) props.get("wcs.camID"),
                   (String) props.get("wcs.pass"));
    wcsPush.start();

    while (StreamServer.getInstance() == null) 
        try { Thread.sleep(1); } catch (InterruptedException ie) { }

    StreamServer.getInstance().setContextObj(bv);

    setSize(width, height);

    pixels = new int[width * height];

    int index = 0;
    for (int y = 0; y < height; y++) {
        int red = (y * 255) / (height - 1);
        for (int x = 0; x < width; x++) {
        int blue = (x * 255) / (width - 1);
        pixels[index++] = (255 << 24) | (red << 16) | blue;
        }
    }

    initRawBuffer();

    Panel p = new Panel(new GridLayout (0, 1));
    Panel bp = getButtonPanel();
    p.add(bp);
    f.add("South", p);

    Panel cp = getConfigPanel();
    f.add ("North", cp);

    target = new Cursor();

    addMouseListener(target);
    addMouseMotionListener(target);

    // Connect to SRV if requested on command line
    if (connectOnStartup && comPort != null) {
        connectButton.setLabel("Disconnect");
        trace("connecting...");
        openSRVConnection();

        if (wcsPush != null)
        wcsPush.connect();
    }
    }

    private Button connectButton = new Button ("    Connect   ");
    private Choice comPorts = null;

    private Button consoleButton = new Button("Event Log");
    private Button cursorButton = new Button("+");

    private Panel getConfigPanel ()
    {
    Panel p = new Panel (new FlowLayout ());

    comPorts = new Choice();
    comPorts.add("-- Ports --");
    comPorts.select(0);    // no selection by default

    comPorts.add("Network");
    if ("Network".equalsIgnoreCase(comPort))
        comPorts.select(1);

    //if (DEBUG) comPorts.add("Test");

    java.util.List l = getSerialPorts ();
    for (int i = 0; i < l.size (); i++) {
        String cp = (String) l.get (i);
        comPorts.add(cp);
        if (cp.equals(comPort))
        comPorts.select(i + 2);
    }

    connectButton.addActionListener(this);
    p.add(comPorts);
    p.add(connectButton);

    consoleButton.addActionListener(this);
    p.add(consoleButton);

    cursorButton.addActionListener(this);
    p.add(cursorButton);

    return p;
    }

    public void sendSRVCommand(String buttonID)
    {
    //_sendSRVCommand((String) commands.get(buttonID));
    String cmd = (String) commands.get(buttonID);
    commandQ.add(cmd);
    }

    private void _sendSRVCommand(String cmd)
    {
    if (cmd == null || "".equals(cmd)) return;

    trace("### Command ###");
    trace("# Sending: 0x" + cmd);
    trace("###############");

    try {
        int length = cmd.length() / 2;
        if (cmd.indexOf("$x") != -1) length += 1;  // we need two bytes for coordinates
        if (cmd.indexOf("$y") != -1) length += 1;
        if (cmd.indexOf("$r") != -1) length += 1;
        
        byte[] cmdBytes = new byte[length];
        int idx = 0;
        for (int j = 0; j < cmd.length() / 2; j++) {
        String cb = cmd.substring(2*j, 2*j + 2);

        // handle special tokens
        if ("$x".equals(cb)) {
            short x = target.getCenterX();
            cmdBytes[idx++] = (byte) (x & 0xFF);          // FIXME: byte order / masking 
            cmdBytes[idx++] = (byte) ((x / 256) & 0xFF);
        } else if ("$y".equalsIgnoreCase(cb)) {
            short y = target.getCenterY();
            cmdBytes[idx++] = (byte) (y & 0xFF);          // FIXME: byte order / masking 
            cmdBytes[idx++] = (byte) ((y / 256) & 0xFF);
        } else if ("$r".equalsIgnoreCase(cb)) {
            short r = target.getRadius();
            cmdBytes[idx++] = (byte) (r & 0xFF);          // FIXME: byte order / masking 
            cmdBytes[idx++] = (byte) ((r / 256) & 0xFF);
        } else {
            // convert hex string to byte
            cmdBytes[idx++] = (byte) Integer.parseInt(cb, 16);
        }
            
        //System.out.println(cb + " -> " + (idx>1?cmdBytes[idx - 2]:0) + "-" + cmdBytes[idx-1]);

        }
        out.write(cmdBytes);
        if ("Network".equals(comPort)) out.flush();
    } catch (Exception ex) {
        error("command: " + cmd + ", " + ex);
        ex.printStackTrace ();
    }
    }

    // Key listener methods
    public void keyTyped(KeyEvent e) { }
    public void keyReleased(KeyEvent e) { }

    // See http://java.sun.com/j2se/1.4.2/docs/api/constant-values.html#java.awt.event.KeyEvent.CHAR_UNDEFINED
    // for Java's "virtual key codes"
    public void keyPressed(KeyEvent e) {
    byte keyCode = (byte) (e.getKeyCode() & 0xff);  // the interesting keys have codes < 256

    if (out == null) {
        System.out.println("* Key (" + keyCode + ") pressed, but SRV-1 is disconnected");
        error("not connected to SRV");
        return;
    }

    System.out.println("* Sending key code: " + keyCode);
    try {
        out.write(keyCode);
        if ("Network".equals(comPort)) out.flush();
    } catch (IOException ioe) {
        error("key event, keycode: " + keyCode + " - " + ioe);
    }
    }


    public synchronized void actionPerformed(ActionEvent e)
    {
    bv.requestFocusInWindow(); // immediately regain keyboard focus

    Object s = e.getSource();
    if (s instanceof ImageButton) {
        if (out == null) {
        error("not connected to SRV");
        return;
        }

        String cmd = (String) ((ImageButton) s).getSVCommand();
        //_sendSRVCommand(cmd);
        commandQ.add(cmd);

    } else if (s == connectButton) {
        String cmd = connectButton.getLabel();
        if ("Connect".equals(cmd.trim())) {
        
        if (comPorts.getSelectedIndex () != 0) {
            comPort = comPorts.getSelectedItem();
            props.put("comport", comPort);
            saveConfig();

            connectButton.setLabel("Disconnect");
            trace("connecting...");
            openSRVConnection();

            if (wcsPush != null)
            wcsPush.connect();
        } else {
            error("no COM port selected");
        }

        } else {
        if (wcsPush != null)
            wcsPush.disconnect();

        connectButton.setLabel("    Connect    ");
        closeSRVConnection();
        trace("disconnected");
        }
    } else if (s == consoleButton) {
        if (logWindow.isShowing())  logWindow.hide();
        else                        logWindow.show();
    } else if (s == cursorButton) {
        target = new Cursor();
        addMouseListener(target);
        addMouseMotionListener(target);
        repaint();
    }
    }

    private Panel getButtonPanel()
    {
    URL base = null;

    try {
        base = new URL ("file://");
    } catch (IOException e) {
    } catch (Exception e) {    }

    LayoutManager lm = new FlowLayout ();
    int cols = 8;
    try {
        cols = Integer.parseInt((String) props.get("button.columns"));
    } catch (Exception e) { }

    lm = new GridLayout(0, cols);
    Panel pan = new Panel(lm);
    int i = 0;

    while (true) {
        i++;
        String gif = (String) props.get("button." + i);
        String cmd = (String) props.get("command." + i);

        if (gif == null || "".equals(gif)) break;

        try {
        ImageButton b = new ImageButton((new File (gif)).toURL (),
                        (new File (gif)).toURL (),
                        cmd);
        b.addActionListener(this);
        pan.add(b);

        trace("added: " + gif + ", cmd: " + cmd);
                
        cmdButtons.put(cmd, b);
        commands.put("" + i, cmd);

        } catch (MalformedURLException e) {
        trace("unable to add: " + gif + ", cmd: " + cmd);
        }
    }

    return pan;
    }

    /*
       JPEG -
       SVFRIMJ1xxxx xxxx = binary length, low to high
       IMJ1 - 80x64, IMJ3 - 160x128 IMJ5 - 320x240 IMJ7 - 640x480

       RAW (Y pixels only) -
       SVFRIMR1xxxx xxxx = binary length, low to high
       IMJ1 - 80x60, IMJ3 - 160x120 IMJ5 - 320x240 IMJ7 - 640x480
     */

    private boolean newFrame(Image i)
    {
    image = i;
    repaint();
    return true;
    }


    /**
     * Main SRV event loop: issues periodic requests for
     * frames and handles deferred sending of commands.
     * 
     */
    private class SRVEventLoop extends Thread
    {
    private int count = 0;
    private long lastFrameRequest = 0;

    public void run() 
    {
        while(shouldRun) {
        try { Thread.sleep(10); } catch (InterruptedException ie) { }
        if (!idle && count++ < 50) continue;
        count = 0;
        if (out == null) continue;

        try {
            // don't send "heartbeat" request more than twice a second
            if (frameSize == 0 && System.currentTimeMillis() - lastFrameRequest > 500) {
            lastFrameRequest = System.currentTimeMillis();
            requestFrame();
            }

            //try { Thread.sleep(20); } catch (InterruptedException ie) { }
        
            if (is.available() > 0)
            readLoop();

        } catch (Exception e) { error(e.toString()); }
        }
    }
    }

    String comLock = "LOCK";

    private static final byte[] frameHeader = 
    { '#', '#', 'I', 'M', 'J' };

    int pos = 0;
    int frameSize = 0;
    byte[] frame = null;
    char frameDim = '5';

    private void requestFrame() throws IOException
    {
    processCommandQueue();

    out.write("I".getBytes()); // grab frame
    if (DEBUG) System.out.println("wrote 'I'");
    if ("Network".equals(comPort)) out.flush();
    }

    private void getScanHits() throws IOException
    {
    if (!scanEnabled) return;

    out.write("S".getBytes());
    if (DEBUG) System.out.println("wrote 'S'");
    if ("Network".equals(comPort)) out.flush();

    try { Thread.sleep(500); } catch (InterruptedException ie) { }

    if (DEBUG) System.out.println("'S' ACK - " + is.available());

    int r = 0;
    StringBuffer sb = new StringBuffer();
    while (is.available() > 0 && r < 168) {
        sb.append((char) is.read());
        r++;
    }

    String scan = sb.toString();
    if (scan.startsWith("##Scan")) {
        if (DEBUG) System.out.println(scan);

        int ish = 0;
        for (int i = 9; i < sb.length(); i += 2) {
        try {
            scanHits[ish++] = Integer.parseInt(sb.substring(i, i + 2).trim());
        } catch  (Exception e) { }
        }

        // print out the resulting scan hits array 
        if (DEBUG) {
        StringBuffer after = new StringBuffer("         ");
        for (int i = 0; i < scanHits.length; i++)
            after.append(scanHits[i]).append(",");
        System.out.println(after);
        }
    }
    }


    // send queued commands
    private void processCommandQueue() 
    {
    int retries = 0;
    while (!commandQ.isEmpty()) {
        String cmd = (String) commandQ.remove(0);
        if (DEBUG) System.out.println("## sending command: " + cmd);
        _sendSRVCommand(cmd);

        // Command ACK / retry 
        /*
        try { Thread.sleep(100); } catch (InterruptedException ie) { }
        if ((is.available() > 2 &&
         (char) is.read() == '#' &&
         (char) is.read() == cmd.charAt(0)) ||
        retries++ >= 2) {
        System.out.println("## got ACK for " + cmd + " (" + retries + ")");
        commandQ.remove(0);
        retries = 0;
        }
        */
    }
    }

    private void readLoop()
    {
    int mark = 0;

    try {
        //if (DEBUG) System.out.println("readLoop() - available() =  " + is.available());

        if (frameSize == 0) {
        while (is.available() > 0 && mark < frameHeader.length && shouldRun) {
            byte b = (byte) is.read();
            if (debug != null) debug.write(b);

            if (frameHeader[mark] == (char) b)  mark++;
            else mark = 0;
        }

        if (mark == frameHeader.length) {
            idle = false;
            byte b = (byte) is.read();
            if (debug != null) debug.write(b);
            frameDim = (char) b;

            for (int i = 0; i < 4; i++) {
            int in = is.read();
            if (debug != null) debug.write(in);
            frameSize += in * Math.pow(256, i);
            }
        }
        } 

        if (frameSize > 0 && frameSize < (150 * 1024)) { // limit frames to 150k

        if (DEBUG) System.out.println("reading frame: " + pos + " / " + frameSize);

        if (frame == null)
            frame = new byte[frameSize];

        int timeout = 0;

        while (pos < frameSize && timeout < 600 && shouldRun) {
            if (is.available() <= 0) {
            timeout++;
            try { Thread.sleep(50); } catch (InterruptedException ie) {}
            continue;
            }
            
            int thisRead = is.read(frame, pos, frameSize - pos);

            /*
            if (DEBUG && debug != null) { 
            debug.write(frame, pos, thisRead); 
            debug.flush(); 
            }
            */

            pos += thisRead;
            //System.out.println("  current pos - " + pos);
        }

        if (pos == frameSize) {
            if (DEBUG) System.out.println("finished frame - " + frameSize + " - " + System.currentTimeMillis());
            frameDecoder.newRawFrame("IMJ" + frameDim, frame);
        } else {
            if (DEBUG) System.out.println("**short read: " + pos + " of " + frameSize);
            //frameDecoder.newRawFrame("IMJ" + frameDim, frame);
        }

        getScanHits();
        requestFrame();

        pos = 0;
        frameSize = 0;
        frame = null;
        }
        idle = true;

    } catch (Exception e) {
        if (DEBUG) e.printStackTrace();
    }
    }

    public void serialEvent(SerialPortEvent ev)
    {
    /*
    //synchronized (comLock) {

        //long start = System.currentTimeMillis();
        
        if (ev.getEventType() != SerialPortEvent.DATA_AVAILABLE)
        return;


        try {
        System.out.println("serialEvent() - " +is.available() + ", " + System.currentTimeMillis());
        } catch (IOException e) { }

        readLoop();
        //}
        */
    }



    /**
     * Shutdown the SRV console and clean up resources
     */
    private void shutdown()
    {
    shouldRun = false;
    idle = true;
    closeSRVConnection();

    try {
        if (debug != null) debug.close();
    } catch (Exception e) { }
    }


    private void trace(String msg)
    {
    logService.log(msg + LINESEP);
    }

    private void traceChar(String msg)
    {
    logService.log(msg);
    }

    private void error(String msg)
    {
    logService.log("[ERROR] " + msg + LINESEP);
    }

    private void openSRVConnection()
    {
    if ("Network".equalsIgnoreCase(comPort)) {
        openNetworkSRV();
    } else if ("Test".equalsIgnoreCase(comPort)) {
        try {
        is = new FileInputStream("/tmp/srv/testing.bin");
        out = new FileOutputStream("/tmp/srv/testing.out");
        } catch (Exception e) { }

    } else {
        openSerialPort();
    }
    }

    private void closeSRVConnection()
    {
    if ("Network".equalsIgnoreCase(comPort)) {
        /*
        if (networkSRV != null) {
        networkSRV.disconnect();
        networkSRV = null;
        }
        */
    } else {
        closeSerialPort();
    }

    try { out.close(); } catch(Exception e) { }
    out = null;
    try { is.close(); } catch(Exception e) { }
    is = null;
    try { s.close(); } catch(Exception e) { }
    s = null;
    
    //try { serialPort.close(); } catch(Exception e) { }
    //serialPort = null;
    }


    /**
     * Open connection to networked SRV
     */
    private void openNetworkSRV()
    {
    String host = (String) props.get("network.srv.host");
    String port = (String) props.get("network.srv.port");
    if (host != null && port != null) {
        try {
        s = new Socket(host, Integer.parseInt(port));
        is = new BufferedInputStream(s.getInputStream());
        out = new BufferedOutputStream(s.getOutputStream());
        } catch (Exception e) {
        e.printStackTrace();
        }

        trace (LINESEP + "[NetworkSRV] - opened connection to " + host + ":" + port);

        /*
        networkSRV = new NetworkSRV(host, port);
        networkSRV.start();
        networkSRV.connect();
        */
    } else {
        error("network.srv.host and network.srv.port missing from config file");
    }
    }


    private boolean setBps(int bps)
    {
    boolean r = false;

    try {
        serialPort.setSerialPortParams (bps,
                        SerialPort.DATABITS_8,
                        SerialPort.STOPBITS_1,
                        SerialPort.PARITY_NONE);

        serialPort.setFlowControlMode(SerialPort.FLOWCONTROL_NONE);
        r = true;
    } catch (UnsupportedCommOperationException e3) {
        error ("error configuring RS232 port");
        e3.printStackTrace ();
    }

    return r;
    }
    

    private java.util.List getSerialPorts()
    {
    java.util.List l = new ArrayList ();
    Enumeration portList;
    CommPortIdentifier portId;

    portList = CommPortIdentifier.getPortIdentifiers();
    
    while (portList.hasMoreElements()) {

        portId = (CommPortIdentifier) portList.nextElement ();
        if (portId.getPortType () == CommPortIdentifier.PORT_SERIAL) {
        l.add(portId.getName ());
        }
    }

    return l;
    }

    private void openSerialPort()
    {
    Enumeration portList;
    CommPortIdentifier portId;

    if (comPort == null || "".equals (comPort)) {
        error("no COM port specified");
    }

    portList = CommPortIdentifier.getPortIdentifiers();

    while (portList.hasMoreElements()) {

        portId = (CommPortIdentifier) portList.nextElement ();
        if (portId.getPortType() == CommPortIdentifier.PORT_SERIAL) {
                
        if (comPort.equals(portId.getName ())) {

            trace("opening " + portId.getName () + "...");

            try {
            serialPort =
                (SerialPort) portId.open("SRV Console", 0);
            }
            catch (PortInUseException e) {
            trace("RS232 port in use");
            return;
            }

            setBps (115200);

            try {
            out = serialPort.getOutputStream();
            is = new BufferedInputStream(serialPort.
                             getInputStream(), 16384);
            } catch (IOException e2) {
            error(e2.toString());
            }

            serialPort.setDTR(false);
            serialPort.setRTS(false);

            /*
            try {
            serialPort.addEventListener(this);
            } catch (TooManyListenersException e) {    }
            serialPort.notifyOnDataAvailable(true);
            */

            trace("connected to " + portId.getName ());
            return;
        }
        }
    }
    }

    private void closeSerialPort()
    {
    try {
        if (out != null)    out.close ();
        if (is != null)    is.close ();
        if (serialPort != null)    serialPort.close();
            
        out = null;
        is = null;
        serialPort = null;
    } catch (IOException ioe) {
        trace ("Error closing SRV connection: " + ioe);
    }
    }


    private class LogService extends Thread
    {
    private StringBuffer logBuf = new StringBuffer(512);

    public void log(String msg) {
        synchronized (logBuf) {
        logBuf.append(msg);
        }
    }

    public void run() 
    {
        while(shouldRun) {
        try { Thread.sleep(250); } catch (InterruptedException ie) { }

        String l = null;
        synchronized (logBuf) {
            l = logBuf.toString();
            logBuf.delete(0, logBuf.length());
        }

        if (l != null && l.length() > 0) {
            log.append(l);
            try {
            log.setCaretPosition(Integer.MAX_VALUE);
            } catch (IllegalComponentStateException ise) { }
        }

        }
    }
    }

    /**
     * This thread serves as the URL notification worker.  Every ASCII 
     * character read from the SRV-1 is sent to this worker.  When the internal 
     * buffer ('collector') has reached SEARCH_INTERVAL characters,
     * NotifyService searches the string for tokens (defined in srv_notify.config).  
     * When a token is encountered, the associated URL is added to the visit queue.
     */
    private class NotifyService extends Thread
    {
    private java.util.List urls = new ArrayList();  // queue that hold URLs to be visted
    private StringBuffer collector = new StringBuffer();

    // run a search every 64 characters by default
    private static final int SEARCH_INTERVAL = 64;

    public void newChar(char c) 
    {
        collector.append(c);
    }

    private int findNotificationTokens(String str)
    {
        int lastIndex = 0;
        
        for (Iterator i = notifyTokens.keySet().iterator(); i.hasNext(); ) {
        String tok = (String) i.next();
        int index = str.indexOf(tok);
        if (index != -1) {
            if (index > lastIndex) lastIndex = index + tok.length();
            String url = (String) notifyTokens.get(tok);
            trace("Found: " + tok + ", visiting: " + url);
            visitUrl(url);
        }
        }

        if (lastIndex == 0) lastIndex  = str.length() - (SEARCH_INTERVAL / 2);
        return lastIndex;
    }


    private void visitUrl(String url) {
        synchronized (urls) {
        urls.add(url);
        }
    }

    public void run() 
    {
        while(true) {
        try { Thread.sleep(250); } catch (InterruptedException ie) { }
        
        if (collector.length() >= SEARCH_INTERVAL) {
            System.out.println("searching: " + collector.toString());
            int lastIndex = findNotificationTokens(collector.toString());
            collector = new StringBuffer(collector.substring(lastIndex));
        }

        while (urls.size() > 0) {
            try {
            String u = null;
            synchronized (urls) {
                u = (String) urls.remove(0);
            }

            if (u != null) {
                URL url = new URL(u);
                InputStream urlis = url.openStream();
                urlis.close();
            }
            } catch (Exception e) { 
            trace("NotifyService: " + e);
            }
        }
        }
    }
    }


    private class FrameDecoder extends Thread
    {
    private byte[] img = null;
    private String type = "IMJ3";

    public void newRawFrame(String type, byte[] f) {
        this.img = f;
        this.type = type;
    }

    public void run() {
        while (shouldRun) {
        if (img == null) {
            try { Thread.sleep(50); } catch (InterruptedException ie) { }
            continue;
        }

        if (type.startsWith ("IMB")) {

            char res = type.charAt (3);
            if (res == '1') {
               width = 40;
               height = 32;
            } else if (res == '3') {
              width = 80;
              height = 64;
            } 

            pixels = new int[img.length * 8];    
            initRawBuffer ();

            for (int i = 0; i < img.length; i++) {
            for (int j = 0; j < 8; j++) {
                if (((int)img[i] & (0x00000080 >> j)) > 0)
                pixels[i*8 + j] = 0xFFFFFFFF;
                            else
                pixels[i*8 + j] = 0xFF000000;
                }
            }

            source.newPixels(0, 0, width, height);

            newFrame(image);

        } else {            // JPEG ("IMJ") is default

            Image i = Toolkit.getDefaultToolkit().createImage(img);

            tracker.addImage (i, 0);
            try {
            tracker.waitForID (0);
            } catch (InterruptedException ie) {
            error ("JPEG decode " + ie);
            }

            if (!tracker.isErrorID (0)) {
            newFrame(i);
            }

            tracker.removeImage(i);

            if (wcsPush != null)
            wcsPush.newFrame(img);
        }

        img = null;
        }
    }
    }


    /**
     * Handle cursor dragging and resizing
     */
    private class Cursor implements MouseListener, MouseMotionListener
    {
    private Image cursor = null;
    private int cursorW = 20;
    private int cursorH = 20;
    private int cursorWOrig;  // original size of cursor bitmap
    private int cursorHOrig;
    private int cursorX = (width / 2) - 10;   // coordinates of top left corner or cursor
    private int cursorY = (height / 2) - 10;

    boolean dragging = false;
    boolean sizing = false;
    private int offsetX = 0;
    private int offsetY = 0;
    private int x1 = cursorX;
    private int y1 = cursorY;
    private int clickX = 0;
    private int clickY = 0;

    private int refX;  // absolute corrds of frame that contains this cursor
    private int refY; 

    public Cursor() {
        load();
    }

    public void draw(int refX, int refY, Graphics g) {
        this.refX = refX;
        this.refY = refY;

        g.drawImage(cursor, refX + cursorX, refY + cursorY, null);
    }

    public void recenter(int width, int height) {
        cursorX = (width / 2) - (cursorW / 2);
        cursorY = (height / 2) - (cursorH / 2);
    }

    // x-coordinate of cursor center (relative to refX,refY)
    public short getCenterX()
    {
        return (short) (cursorX + (cursorW / 2));
    }

    // y-coordinate of cursor center (relative to refX,refY)
    public short getCenterY()
    {
        return (short) (cursorY + (cursorH / 2));
    }

    // cursor 'radius' in pixels
    // = distance from the center point to corner
    public short getRadius()
    {
        int x1 = cursorX + (cursorW / 2);
        int y1 = cursorY + (cursorH / 2);
        int x2 = cursorX + cursorW;
        int y2 = cursorY + cursorH;

        return (short) Math.sqrt(((x2 - x1) * (x2 - x1)) + 
                     ((y2 - y1) * (y2 - y1)));
    }

    private void render()
    {
        tracker.addImage(cursor, 0);
        try { tracker.waitForID(0); } 
        catch (InterruptedException iec) {
        error("cursor failure " + iec);
        }
        if (!tracker.isErrorID(0)) {
        cursorW = cursor.getWidth(null);
        cursorH = cursor.getHeight(null);
        }
        tracker.removeImage(cursor);
    }

    public void load()
    {
        if (cursor != null)
        cursor.flush();
        cursor = Toolkit.getDefaultToolkit().getImage("cursor.png");
        render();
        cursorWOrig = cursorW;
        cursorHOrig = cursorH;
    }

    public void mousePressed(MouseEvent evt) { 

        if (dragging || sizing)  // Exit if a drag is already in progress.
        return;

        clickX = evt.getX();  // Location where user clicked.
        clickY = evt.getY();

        int boundX = cursorX + refX;
        int boundY = cursorY + refY;

        // check if click was on cursor
        if (clickX >= boundX && clickX < boundX + cursorW && 
        clickY >= boundY && clickY < boundY + cursorH) {

        // size of the resize handle (bottom quarter, minimum 5x5 pixels)
        int handle = Math.max(cursorW / 4, cursorH / 4);
        if (handle < 5) handle = 5;

        // resize if click was in handle zone
        if (clickX >= boundX + cursorW - handle &&
            clickY >= boundY + cursorH - handle) {
            sizing = true;
        } else {
            dragging = true;
            offsetX = clickX - boundX;  // Distance from corner of square to click point.
            offsetY = clickY - boundY;
        }
        }

    }

    // Dragging / sizing stops when user releases the mouse button.
    public void mouseReleased(MouseEvent evt) { 
        dragging = false;
        sizing = false;
    }

    public void mouseDragged(MouseEvent evt) { 

        int x = evt.getX();   // position of mouse
        int y = evt.getY();

        //System.out.println("mouseDragged(): " + x + ", " + y + " : " + offsetX + ", " + offsetY);

        if (dragging) {
        // new top left coords of cursor
        int newX = x - offsetX;
        int newY = y - offsetY;

        if (newX < refX) newX = refX;
        if (newX > refX + width - cursorW)  newX = refX + width - cursorW;
        if (newY < refY) newY = refY;
        if (newY > refY + height - cursorH) newY = refY + height - cursorH;

        cursorX = newX - refX;     // move the cursor
        cursorY = newY - refY;
        repaint();
        } else if (sizing) {
        int add = Math.max(x - clickX, y - clickY);
        
        int newW = cursorW + add;
        int newH = cursorH + add;

        if (Math.abs(newW - cursorW) < 2 ||
            Math.abs(newH - cursorH) < 2) return;


        if (newW <= 10 || newH <= 10) {
            newW = cursorW;
            newH = cursorH;
        }

        //System.out.println("sizing: " + x + ", " + clickX + " : " + newW + ", " + newH);

        if (newW > width - cursorX) {
            newW = width - cursorX;
            newH = cursorH + (newW - cursorW);
        } else if (newH > height - cursorY) {
            newH = height - cursorY;
            newW = cursorW + (newH - cursorH);
        }

        cursorW = newW;
        cursorH = newH;

        // reload original bitmap if we're near original size
        // (repeated sizing tends to distort images)
        if (Math.abs(cursorW - cursorWOrig) < 2 ||
            Math.abs(cursorH - cursorHOrig) < 2) {
            trace("reverting: " + cursorW + ", " + cursorH + 
              " : " + cursorWOrig + ", " + cursorHOrig);
            load();
        } else {
            Image i = cursor.getScaledInstance(cursorW, cursorH, Image.SCALE_DEFAULT);
            cursor.flush();
            cursor = i;
        }
        render();
        repaint();
        } else {
        return;
        }
    }


    public void mouseMoved(MouseEvent evt) { }
    public void mouseClicked(MouseEvent evt) { }
    public void mouseEntered(MouseEvent evt) { }
    public void mouseExited(MouseEvent evt) { }
    }


    private class WCSServer extends Thread
    {
    public void run() {
        StreamServer.main(new String[] { (String) props.get("wcs.port") });
    }
    }


    /**
     * Thread to handle Webcamsat push.
     */
    private class WCSPush extends Thread
    {
    private boolean shouldPush = false;
    private byte[] wcsFrame = null;
    private Object frameLock = new Integer (1);

    private Socket s = null;
    private OutputStream wcs = null;

    private String server, port, camID, pass;

    public WCSPush(String server, String port, String camID, String pass)
    {
        this.server = server;
        this.port = port;
        this.camID = camID;
        this.pass = pass;
    }

    public void connect()
    {
        shouldPush = true;
    }

    public void disconnect()
    {
        shouldPush = false;
        wcsFrame = null;
        closeConnection ();
    }
        
    private void closeConnection()
    {
        try {
        if (wcs != null) wcs.close();
        if (s != null) s.close();
        wcs = null;
        s = null;
        } catch (Exception e) { }
        
        trace (LINESEP + "[wcs] - closed connection to " + server + ":" + port);
    }

    private void openConnection ()
    {
        try {
        s = new Socket (server, Integer.parseInt (port));
        wcs = new BufferedOutputStream (s.getOutputStream ());
        wcs.write (("SOURCE " + camID + " " + pass +
                " HTTP/1.1\r\n\r\n\r\n").getBytes ());
        
        } catch (Exception e) {
        e.printStackTrace ();
        }

        trace (LINESEP + "[wcs] - opened connection to " + server + ":" + port);
    }

    public void newFrame (byte[]f)
    {
        synchronized (frameLock) {
        wcsFrame = f;
        trace ("[wcs] - new frame");
        }
    }

    public void run ()
    {
        while (true) {
        if (!shouldPush || wcsFrame == null) {
            try {
            Thread.sleep (250);
            } catch (InterruptedException e) { }
            continue;
        }

        if (wcs == null) {
            openConnection ();
        }
                
        synchronized (frameLock) {
            String frameHead = "SVFR            ";
                    
            try {
            wcs.write (frameHead.getBytes ());
            wcs.write (wcsFrame);
            wcs.flush ();
            }
            catch (Exception e) {
            trace ("[wcs] - error: " + e);
            closeConnection ();
            }
            
            wcsFrame = null;
        }
        }
    }
    }
    


    /*******************************
     * Config file utility methods 
     ******************************/

    public void loadConfig ()
    {

    try {
        FileInputStream nf = new FileInputStream(NOTIFY_CONFIG_FILE);
        Properties notifyProps = new Properties();
        notifyProps.load(nf);
        notifyTokens.putAll(notifyProps);
        nf.close();
    } catch (Exception e) {
        //trace ("error loading notify config file: " + e);
    }


    props.clear ();
    
    try {
        BufferedReader br =
        new BufferedReader (new FileReader(CONFIG_FILE));
        String line = null;
        while ((line = br.readLine ()) != null) {
        if (!line.startsWith ("#")) {
            String[]p = line.split ("=");
            if (p.length == 2 && p[0] != null && p[1] != null) {
            props.put (p[0].trim (), p[1].trim ());
            }
        }
        }

        br.close ();
    } catch (Exception e) {
        trace ("error loading config file: " + e);
    }

    String s = (String) props.get ("comport");
    if (s != null && !"".equals (s))
        comPort = s;
    }

    public void saveConfig ()
    {
    java.util.List lines = new ArrayList();
    java.util.List written = new ArrayList();
        
    try {
        BufferedReader br =
        new BufferedReader (new FileReader(CONFIG_FILE));
        String line = null;
        while ((line = br.readLine ()) != null) {
        lines.add (line);
        }
            
        br.close ();

        BufferedWriter bw =
        new BufferedWriter (new FileWriter(CONFIG_FILE));
        for (Iterator i = lines.iterator (); i.hasNext ();) {
        String l = (String) i.next ();
        String[]p = l.split("=");
        if (p.length == 2 && p[0] != null && p[1] != null) {
            String v = (String) props.get (p[0].trim ());
            if (v != null) {
            l = p[0] + "=" + v;
            written.add(p[0]);
            }
        }

        bw.write(l);
        bw.write(LINESEP);
        }

        for (Iterator it = props.keySet ().iterator (); it.hasNext ();) {
        String k = (String) it.next ();
        if (!written.contains (k)) {
            bw.write(k + "=" + props.get (k));
            bw.write(LINESEP);
            written.add(k);
        }
        }
            
        bw.close ();
    } catch (Exception e) {
        trace ("error saving config file: " + e);
    }
    }


    private final char[] HEX_DIGITS =
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D','E', 'F' };

    /**
     * This converts a <code>byte[]</code> to a <code>String</code>
     * in hexidecimal format.
     *
     * @param input <code>byte[]</code> to convert
     * @return <code>String</code> - resulting Hex String
     */
    public String byteArrayToHex(byte[]input)
    {
    StringBuffer result = new StringBuffer();
    byte highNibble, lowNibble;
    if (input != null) {
        for (int i = 0; i < input.length; i++) {
        highNibble = (byte) ((input[i] >>> 4) & 0x0F);
        lowNibble = (byte) (input[i] & 0x0F);
        result.append(HEX_DIGITS[highNibble]);
        result.append(HEX_DIGITS[lowNibble]);
        result.append(" ");
        }
    }
    return result.toString();
    }

}


/**
byte[] rawImage = conx.receiveBytes( imageLength );

if ( ! isJpgValid ( rawImage ) )
{
throw new IOException ( "Invalid JPG image signature: " +
Integer.toHexString( rawImage[0] ) + " " + Integer.toHexString(
rawImage[1] ) );
}
// start process of converting image from jpg to internal
format
// old AWT style:
// Image image = toolkit.createImage( rawImage );

// new ImageIO style, convert raw bytes to BufferedImage
BufferedImage image = ImageIO.read ( new ByteArrayInputStream (
rawImage ) );
------------------------------

public static boolean isJpgValid ( byte image[] )
{
return( ( image[0] & 0xff ) == 0xff ) && ( (image[1] & 0xff )
== 0xd8 );
}
--
*/

