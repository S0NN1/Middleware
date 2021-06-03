package it.polimi.middlewaretechfordistsys.utils;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;

public final class LogUtils {

    /***
     * Set the log level
     */
    public static void setLogLevel() {
        Logger.getLogger("org").setLevel(Level.OFF);
        Logger.getLogger("akka").setLevel(Level.OFF);
    }

    /***
     * Returns a padded string with leading zeros
     * @param inputString input string
     * @param length how long the string must at least be
     * @return the padded string
     */
    public static String padLeftZeros(String inputString, int length) {
        if (inputString.length() >= length) {
            return inputString;
        }
        StringBuilder sb = new StringBuilder();
        while (sb.length() < length - inputString.length()) {
            sb.append('0');
        }
        sb.append(inputString);

        return sb.toString();
    }
}