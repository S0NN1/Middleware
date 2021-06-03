package it.polimi.middlewaretechfordistsys;

import java.util.*;

import it.polimi.middlewaretechfordistsys.model.Country;
import it.polimi.middlewaretechfordistsys.model.Top10Countries;
import it.polimi.middlewaretechfordistsys.utils.CalculateUtils;
import it.polimi.middlewaretechfordistsys.utils.PrintUtils;
import org.apache.spark.sql.Dataset;
import org.apache.spark.sql.Row;
import org.apache.spark.sql.SparkSession;
import org.apache.spark.sql.types.StructType;

import it.polimi.middlewaretechfordistsys.utils.LogUtils;

import static org.apache.spark.sql.functions.*;

/**
 * Covid19CaseCount
 *
 * Input: csv files with list of deposits and withdrawals, having the following
 * schema:
 *      {
 *          day :Integer,
 *          rank :Integer,
 *          infected :Integer,
 *          sane :Integer,
 *          infected_increment :Double,
 *          sane_increment :Double
 *      }
 *
 * Queries
 * Q1. Print the total amount of withdrawals for each person.
 * Q2. Print the person with the maximum total amount of withdrawals
 * Q3. Print all the accounts with a negative balance
 */
final class Covid19CaseCount {

    private static final HashMap<Integer, Top10Countries> highscore = new HashMap<>(); //query 3 result, indexed by days
    private static final HashMap<Integer, HashMap<Integer, Country>> query1and2Result = new HashMap<>(); //query 1 and 2 result, indexed by days, then for countries

    /***
     * Main method
     * @param args arguments
     */
    public static void main(String[] args) {
        LogUtils.setLogLevel();

        String master = args.length > 0 ? args[0] : "local[4]";
        String filePath = args.length > 1 ? args[1] : "./";

        SparkSession spark = SparkSession
                .builder()
                .master(master)
                .appName("Covid19")
                .getOrCreate();

        StructType mySchema = it.polimi.middlewaretechfordistsys.model.Schema.getSchema();

        Dataset<Row> covidData = spark
                .read()
                .option("header", "false")
                .option("delimiter", ",")
                .schema(mySchema)
                .csv(filePath + "resources/csv/data.csv");


        int maxDay = covidData.select(max("day")).first().getInt(0); //last day, from the input file
        int maxCountries = covidData.select(max("rank")).first().getInt(0); //number of countries, from the input file

        //Initialize result class in order to accomodate result data
        for (int i=0; i<maxDay; i++)
        {
            highscore.put(i, new Top10Countries(i));
            query1and2Result.put(i, new HashMap<>());
        }

        //Calculate the results
        CalculateUtils.calculate(query1and2Result, highscore, maxDay, maxCountries, covidData);

        //Print the results
        PrintUtils.print(query1and2Result, highscore, maxDay, maxCountries);


        spark.close();

    }


}