package ws2json;

import gov.usgs.earthworm.WaveServer;
import gov.usgs.vdx.data.wave.Wave;
import java.text.SimpleDateFormat;
import java.util.Locale;
import java.util.TimeZone;

//waveserver=10.19.6.62_16022&station=LFA.EHZ.CP.--&start=20120315160000&duration=30&encoding=gext&average=reset&rate=40 -debug
public class Ws2json {

    public static final double version = 1.5;
    public static final SimpleDateFormat tf = new SimpleDateFormat("yyyyMMddHHmmss");
    public static final String usageStr = String.format(""
            + "WS2JSON version %3.1f\n\nUsage: java -jar ws2json [OPTIONS] <query string>\n\n"
            + "\t<query string> : Waveserver request options separated by & for the\n"
            + "\t\tstation and time interval.\n"
            + "\t\tNote that this is purposely similar to a typical GET query string\n"
            + "\t\twaveserver=\t: IP and port address of the WaveServer\n"
            + "\t\t\t\t   with the format aaa.bbb.ccc.ddd_ppppp\n"
            + "\t\tstation=\t: Required channel in S.C.N.L format\n"
            + "\t\tstart=\t\t: Required start time in format yyyymmddHHMMSS\n"
            + "\t\tduration=\t: Duration of the time interval in seconds\n"
            + "\t\tencoding=\t: Optional type of encoding. Available types:\n"
            + "\t\t\t\t   - int   : An array of integers (default)\n"
            + "\t\t\t\t   - gsim  : Simple encoding (60 levels)\n"
            + "\t\t\t\t   - gext  : Extended encoding (4095 levels)\n"
            + "\t\taverage=\t: Optional average handling\n"
            + "\t\t\t\t   - reset : Set average to zero (default)\n"
            + "\t\t\t\t   - ok    : Maintain original average\n"
            + "\t\trate=\t\t: Optional output data rate. Changes the\n"
            + "\t\t\t\t  output sampling rate using nearest neighbour\n"
            + "\t\t\t\t  approximation. Note: Will introduce alias.\n"
            + "\t\t\t\t  Default value is 40.\n\n"
            + "Example Usage:\n"
            + "\tjava -jar ws2json waveserver=10.19.6.62_16022&station=LFA.EHZ.CP.--&start=20111203135500&duration=30&encoding=int&average=reset&rate=40"
            + "\n\n", version);
    public static boolean Debug = false;

    public static void main(String[] args) {
        // Set UTC time
        System.setProperty("user.timezone", "Etc/Universal");
        System.setProperty("user.region", "UK");
        TimeZone.setDefault(TimeZone.getTimeZone("UTC"));
        Locale.setDefault(Locale.ENGLISH);

        // Checj input argument
        if (args == null || args.length == 0) {
            System.err.println(usageStr);
            return;
        }

        // Parse query string
        Arguments arg = null;
        try {
            arg = new Arguments(args[0]);
        } catch (Exception e) {
            System.err.println("Error parsing input arguments\n" + e.getMessage());
            System.exit(1);
        }

        // Check debug input argument
        if (args.length > 1 && args[1].equalsIgnoreCase("-debug")) {
            Debug = true;
        }

        // For debugging
        if (Debug) {
            System.out.println(arg.toString());
        }



        // Make request to waveserver using swarm library
        Wave wave = null;
        try {
            //Make request to winston/wave server
            WaveServer ws = new WaveServer(arg.wsIP, arg.wsPort);
            ws.connect();
            wave = ws.getRawData(arg.scnl[0], arg.scnl[1], arg.scnl[2], arg.scnl[3],
                    arg.start - 10, arg.start + arg.duration + 10);
        } catch (Exception e) {
            System.err.println("Error acquiring data from waveserver: " + e);
            System.exit(1);
        }
        if (wave == null) {
            System.err.println("No available data from waveserver");
            System.exit(0);
        }




        //Create JSON data object
        JSONData data = new JSONData();
        data.scnl = arg.scnl;
        data.starttime = (long) (arg.start * 1000);
        data.endtime = (long) ((arg.start + arg.duration) * 1000);
        data.samprate = (arg.rate == 0) ? wave.getSamplingRate() : arg.rate;
        data.average = wave.mean();
        data.nsamp = (int) ((double) (data.endtime - data.starttime) / 1000.0 * data.samprate);

        // Produce preliminary samples
        double[] prelimSamp = new double[data.nsamp];
        long[] t = new long[data.nsamp]; //Time array
        for (int i = 0; i < data.nsamp; i++) {
            t[i] = data.starttime + (long) ((double) i * 1000.0 / data.samprate);
        }
        int[] av0samples;
        if (arg.average == 0 || arg.encoding > 0) {
            av0samples = new int[wave.buffer.length];
            double[] aux = wave.removeMean();
            for (int i = 0; i < av0samples.length; i++) {
                av0samples[i] = (int) (aux[i] + 0.5);
            }
        } else {
            av0samples = wave.buffer;
        }
        // Sub/Super sample
        int[] fsamples = new int[data.nsamp];
        for (int i = 0; i < data.nsamp; i++) {
            // Sample original position
            int spos = (int) ((((double) t[i] - (double) data.starttime) / 1000.0) * wave.getSamplingRate());
            if (spos > 0 && spos < av0samples.length) {
                fsamples[i] = av0samples[spos];
            }
        }

        // Encode, if required
        char[] code;
        char[] outdata;
        EncodedSamples es;
        switch (arg.encoding) {
            case 0: // Array of integers
                data.Samples = fsamples; //No encoding
                data.Scale = 1.0;
                data.Encoding = "IntArray";
                break;
            case 1: // Simple google encoding
                // Set data between 0 and 60
                es = resetLimits(fsamples, 0, 60);
                // Encode using google's simple data
                code = ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789").toCharArray();
                outdata = new char[es.Samples.length];
                for (int i = 0; i < es.Samples.length; i++) {
                    outdata[i] = code[es.Samples[i]];
                }
                data.Samples = new String(outdata);
                data.Scale = es.Scale;
                data.Encoding = "GoogleSimple";
                break;
            case 2: // Extended encoding
                // Set data between 0 and 4095
                es = resetLimits(fsamples, 0, 4095);
                // Encode using google's extended data
                code = ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.").toCharArray();
                outdata = new char[es.Samples.length * 2];
                for (int i = 0; i < es.Samples.length; i++) {
                    //System.out.println(i + ": " + es.Samples[i]);
                    // First character
                    outdata[i * 2] = code[(int) Math.floor((double) es.Samples[i] / 64.0)];
                    // Second character
                    outdata[i * 2 + 1] = code[es.Samples[i] % 64];
                }
                data.Samples = new String(outdata);
                data.Scale = es.Scale;
                data.Encoding = "GoogleExtended";
                break;
        }

        // Output JSON data
        System.out.println(data.toJSON());
        
    }

    private static EncodedSamples resetLimits(int[] data, int minlim, int maxlim) {
        double maxval = 0;
        double average = 0;
        double aux;
        for (int i = 0; i < data.length; i++) {
            average += data[i];
            aux = Math.abs(data[i]);
            if (aux > maxval) {
                maxval = aux;
            }
        }
        average /= (double) data.length;
        for (int i = 0; i < data.length; i++) {
            aux = Math.abs(data[i]-average);
            if (aux > maxval) {
                maxval = aux;
            }
        }

        double delta = (maxlim - minlim) / 2-2;
        double mean = (maxlim + minlim) / 2+1;
        EncodedSamples out = new EncodedSamples();
        out.Samples = new int[data.length];
        out.Scale = maxval / delta;
        for (int i = 0; i < data.length; i++) {
            out.Samples[i] = (int) ((data[i] - average) / out.Scale + mean);
        }
        return out;
    }

    // Class to parse the input arguments
    static class Arguments {

        String wsIP;
        int wsPort;
        String[] scnl;
        double start; // In epoch seconds
        double duration;
        int encoding = 0;
        int average = 0;
        double rate = 0;

        public Arguments(String line) throws Exception {
            // break down in parts
            String[] parts = line.split("[&]+");
            if (parts.length < 5 || parts.length > 7) {
                throw new Exception("Invalid number of arguments in " + line);
            }
            for (int i = 0; i < parts.length; i++) {
                String[] argument = parts[i].split("[=]+");
                if (argument.length != 2) {
                    throw new Exception("Error processing argument: " + parts[i]);
                }
                // Process waveserver argument
                if (argument[0].equalsIgnoreCase("waveserver")) {
                    String[] aux = argument[1].split("[_]+");
                    if (aux.length != 2) {
                        throw new Exception("Invalid waveserver IP and Port: " + parts[i]);
                    }
                    // Check IP
                    String[] splitIP = aux[0].split("[.]+");
                    if (splitIP.length != 4) {
                        throw new Exception("Invalid IP: " + aux[0]);
                    }
                    for (int f = 0; f < 4; f++) {
                        try {
                            Integer.valueOf(splitIP[f]);
                        } catch (Exception e) {
                            throw new Exception("Invalid IP address: " + aux[0]);
                        }
                    }
                    this.wsPort = -1;
                    try {
                        this.wsPort = Integer.valueOf(aux[1]);
                    } catch (Exception e) {
                        throw new Exception("Invalid Port address: " + aux[1]);
                    }
                    if (this.wsPort <= 0 || this.wsPort > 65535) {
                        throw new Exception("Invalid Port address: " + aux[1]);
                    }
                    // Done checking... store it
                    this.wsIP = aux[0];
                    //Process station argument
                } else if (argument[0].equalsIgnoreCase("station")) {
                    this.scnl = argument[1].split("[.]+");
                    if (scnl.length != 4) {
                        throw new Exception("Invalid station name: " + argument[1]);
                    }
                } //Process start argument
                else if (argument[0].equalsIgnoreCase("start")) {
                    try {
                        this.start = (double) tf.parse(argument[1]).getTime() / 1000.0;
                    } catch (Exception e) {
                        throw new Exception("Invalid start time: " + argument[1]);
                    }
                } //Process duration argument
                else if (argument[0].equalsIgnoreCase("duration")) {
                    try {
                        this.duration = Double.valueOf(argument[1]);
                    } catch (Exception e) {
                        throw new Exception("Invalid duration: " + argument[1]);
                    }
                    if (this.duration < 1 || this.duration > 300.0) {
                        throw new Exception("Duration too long (more than 300s) or too short (less than 1s): " + argument[1]);
                    }
                } //Process encoding argument
                else if (argument[0].equalsIgnoreCase("encoding")) {
                    if (argument[1].equalsIgnoreCase("int")) {
                        this.encoding = 0;
                    } else if (argument[1].equalsIgnoreCase("gsim")) {
                        this.encoding = 1;
                    } else if (argument[1].equalsIgnoreCase("gext")) {
                        this.encoding = 2;
                    } else {
                        throw new Exception("Unknown encoding type: " + argument[1]);
                    }
                } //Process average argument
                else if (argument[0].equalsIgnoreCase("average")) {
                    if (argument[1].equalsIgnoreCase("reset")) {
                        this.average = 0;
                    } else if (argument[1].equalsIgnoreCase("ok")) {
                        this.average = 1;
                    } else {
                        throw new Exception("Average setting: " + argument[1]);
                    }
                }//Process duration argument
                else if (argument[0].equalsIgnoreCase("rate")) {
                    try {
                        this.rate = Double.valueOf(argument[1]);
                    } catch (Exception e) {
                        throw new Exception("Invalid output data rate: " + argument[1]);
                    }
                    if (this.rate < 0 || this.rate > 300.0) {
                        throw new Exception("Output data rate outside range: " + argument[1]);
                    }
                }
            }
        }

        @Override
        public String toString() {
            return String.format(""
                    + "ws2json input arguments:\n"
                    + "\twaveserver\t:%s:%d\n"
                    + "\tstation\t\t:%s.%s.%s.%s\n"
                    + "\tstart\t\t:%s\n"
                    + "\tduration\t:%d\n"
                    + "\tencoding\t:%d\n"
                    + "\taverage\t\t:%d\n"
                    + "\trate\t\t:%-6.1f",
                    wsIP, wsPort, scnl[0], scnl[1], scnl[2], scnl[3],
                    tf.format((long) (start * 1000)), (int) duration,
                    encoding, average, rate);
        }
    }
}

class EncodedSamples {

    int[] Samples;
    double Scale;
}