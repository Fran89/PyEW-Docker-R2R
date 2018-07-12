package ws2json;

import java.util.Locale;

public class JSONData {

    String[] scnl;      // Station identifier
    long starttime;     // Start time of this set of samples in miliseconds
    long endtime;       // End time of this set of samples in miliseconds
    double samprate;    // Sampling rate
    double average;     // Original average value
    double Scale;       // Scale factor in the case of rescaling for encoding
    int nsamp;          // Total number of samples
    String Encoding;    // An identifier for the type of encoding used
    Object Samples;     // An integer array of samples or strings with the encoded samples

    public String toJSON() {
        String out = String.format(Locale.UK, "{"
                + "\"scnl\":[\"%s\",\"%s\",\"%s\",\"%s\"],"
                + "\"starttime\":%d,\"endtime\":%d,"
                + "\"samprate\":%-7.2f,"
                + "\"average\":%-9.3f,"
                + "\"scale\":%-9.3f,"
                + "\"nsamp\":%d,"
                + "\"encoding\":\"%s\",",
                scnl[0], scnl[1], scnl[2], scnl[3],
                starttime, endtime,
                samprate,
                average,
                Scale,
                nsamp,
                Encoding);
        if (Samples.getClass().equals(String.class)) {
            out += String.format(Locale.UK, "\"samples\":\"%s\"", (String) Samples);
        } else {
            out += "\"samples:\":[";
            int[] data = (int[]) Samples;
            for (int i = 0; i < data.length; i++) {
                out += String.format(Locale.UK, "%d%s", data[i], (i == data.length - 1) ? "" : ",");
            }
            out += "]";
        }
        out += "}";
        return out;
    }
}
