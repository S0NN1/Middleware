package it.polimi.middlewaretechfordistsys;

import it.polimi.middlewaretechfordistsys.model.CovidRecord;
import it.polimi.middlewaretechfordistsys.model.Nation;
import org.apache.spark.SparkConf;
import org.apache.spark.api.java.JavaPairRDD;
import org.apache.spark.api.java.JavaRDD;
import org.apache.spark.api.java.JavaSparkContext;
import org.apache.spark.api.java.function.Function;
import org.apache.spark.api.java.function.Function2;
import org.apache.spark.api.java.function.PairFunction;
import org.apache.spark.internal.config.R;
import org.apache.spark.sql.*;
import org.apache.spark.sql.expressions.Window;
import org.apache.spark.sql.expressions.WindowSpec;
import scala.Tuple2;

import java.io.Serializable;
import java.text.SimpleDateFormat;
import java.util.regex.Pattern;

public class Covid19Analysis {

    public static void main(String[] args) {
        final SparkConf conf = new SparkConf().setAppName("Covid-19").setMaster(args.length > 0 ? args[0] : "local[4]");
        final JavaSparkContext sc = new JavaSparkContext(conf);
        SparkSession spark = SparkSession
                .builder()
                .appName("Covid-19 Data")
                .getOrCreate();
        //JavaRDD<String> data = sc.textFile("resources/csv/ecdc/data.csv");
        SQLContext sqlContext = new SQLContext(sc);

        // TASK 1
        Dataset<Row> df = spark.read().format("csv").option("header", "true").option("inferSchema", true).load("resources/csv/ecdc/data.csv");
        WindowSpec ws1 = Window.partitionBy("countriesAndTerritories").orderBy("date").rowsBetween(-6,0);
        Column col1 = functions.avg("cases").over(ws1);
        Dataset<Row> df1 = df.withColumn("date", functions.to_timestamp(df.col("dateRep"), "dd/MM/yyyy")).withColumn("movingAverage", functions.round(col1, 2));

        // TASK 2
        WindowSpec ws2 = Window.partitionBy("countriesAndTerritories").orderBy("date");
        Dataset<Row> df2 = df1.withColumn("prevValue", functions.round(functions.lag("movingAverage", 1).over(ws2), 2));
        Dataset<Row> df3 = df2.withColumn("perc_increase", functions .when(functions
                        .isnull(df2
                                .col("movingAverage")
                                .minus(df2.col("prevValue"))),0)
                .otherwise(functions.round(df2
                        .col("movingAverage")
                        .minus(df2.col("prevValue")).divide(df2.col("prevValue")).multiply(100).cast("float"), 2))).drop("prevValue");
        df3.show(100);
    }

}
