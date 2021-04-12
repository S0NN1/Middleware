package it.polimi.middlewaretechfordistsys;

import it.polimi.middlewaretechfordistsys.model.Schema;
import it.polimi.middlewaretechfordistsys.utils.LogUtils;
import org.apache.spark.sql.SparkSession;

/**
 * Hello world!
 *
 */
public class Covid19CasesCount {
    private static final boolean useCache = true;

    public static void main( String[] args )
    {
        LogUtils.setLogLevel();

        final String master = args.length > 0 ? args[0] : "local[4]";
        final String filePath = args.length > 1 ? args[1] : "./";
        final String appName = "Covid19 Data Analysis";

        final SparkSession sparkSession = SparkSession
                .builder()
                .master(master)
                .appName("Covid19")
                .getOrCreate();

        final Schema schema = new Schema(sparkSession, filePath + "resources/csv/data.csv");


    }
}
