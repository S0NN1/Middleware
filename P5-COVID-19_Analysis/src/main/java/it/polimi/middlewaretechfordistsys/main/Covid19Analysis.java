package it.polimi.middlewaretechfordistsys.main;

import org.apache.log4j.Level;
import org.apache.log4j.LogManager;
import org.apache.spark.SparkConf;
import org.apache.spark.api.java.JavaSparkContext;
import org.apache.spark.sql.*;
import org.apache.spark.sql.expressions.Window;
import org.apache.spark.sql.expressions.WindowSpec;
import org.slf4j.LoggerFactory;

import org.slf4j.Logger;

import static org.apache.spark.sql.functions.rank;

public final class Covid19Analysis {

    public static void main(final String[] args) {

        Logger logger = LoggerFactory.getLogger("Logger");
        LogManager.getRootLogger().setLevel(Level.OFF);

        java.util.logging.Logger.getLogger("org.apache.spark").setLevel(java.util.logging.Level.WARNING);
        java.util.logging.Logger.getLogger("org.spark-project").setLevel(java.util.logging.Level.WARNING);

        final int maxRankCountries = 10;

        final String filePath = args.length > 1 ? args[1] : "./";
        SparkConf conf = new SparkConf().setAppName("Covid-19").setMaster(args.length > 0 ? args[0] : "local[4]");
        JavaSparkContext sc = new JavaSparkContext(conf);
        final SparkSession spark = SparkSession
                .builder()
                .appName("Covid-19 Data")
                .getOrCreate();

        // TASK 1
        final Dataset<Row> df = spark.read().format("csv").option("header", "true").option("inferSchema", true).load(filePath + "resources/csv/ecdc/data.csv");
        final WindowSpec ws1 = Window.partitionBy("countriesAndTerritories").orderBy("date", "geoId").rowsBetween(-6,0);
        final Column col1 = functions.avg("cases").over(ws1);
        final Dataset<Row> df1 = df.withColumn("date", functions.to_timestamp(df.col("dateRep"), "dd/MM/yyyy")).withColumn("movingAverage", functions.round(col1, 2));

        // TASK 2
        final WindowSpec ws2 = Window.partitionBy("countriesAndTerritories").orderBy("date", "geoId");
        final Dataset<Row> df2 = df1.withColumn("prevValue", functions.round(functions.lag("movingAverage", 1).over(ws2), 2));
        final Column col3 = df2.col("movingAverage").minus(df2.col("prevValue"));
        final Column col2 = col3.divide(df2.col("prevValue")).multiply(100).cast("float");
        Dataset<Row> df3 = df2.withColumn("perc_increase", functions.when(functions.isnull(col3),0)
                .otherwise(functions.round(col2, 2))).drop("prevValue");
        df3 = df3.filter(df3.col("perc_increase").isNotNull());

        //TASK 3
        final WindowSpec ws3 = Window.partitionBy("date").orderBy(functions.desc("perc_increase"));
        final Dataset<Row> df4 = df3.withColumn("rankingPercIncrease", rank().over(ws3));
        final Dataset<Row> df5 = df4.where("rankingPercIncrease<=" + maxRankCountries).orderBy("date", "rankingPercIncrease");


        //Print results
        df1.show(1000); // Query 1
        df3.show(1000); // Query 2
        df5.show(1000); // Query 3

    }

}
