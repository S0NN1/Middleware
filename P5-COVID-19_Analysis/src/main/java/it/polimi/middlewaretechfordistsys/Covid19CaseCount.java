package it.polimi.middlewaretechfordistsys;

import java.util.*;

import it.polimi.middlewaretechfordistsys.model.Country;
import it.polimi.middlewaretechfordistsys.model.Top10Countries;
import org.apache.spark.sql.Dataset;
import org.apache.spark.sql.Row;
import org.apache.spark.sql.SparkSession;
import org.apache.spark.sql.types.StructType;

import it.polimi.middlewaretechfordistsys.utils.LogUtils;

import static org.apache.spark.sql.functions.*;

/**
 * Bank example
 * <p>
 * Input: csv files with list of deposits and withdrawals, having the following
 * schema ("person: String, account: String, amount: Int)
 * <p>
 * Queries
 * Q1. Print the total amount of withdrawals for each person.
 * Q2. Print the person with the maximum total amount of withdrawals
 * Q3. Print all the accounts with a negative balance
 */
public class Covid19CaseCount {

    static final HashMap<Integer, Top10Countries> highscore = new HashMap<>();
    static final HashMap<Integer, HashMap<Integer, Country>> query1and2Result = new HashMap<>(); //indexed for days, then for countries

    public static void main(String[] args) {
        LogUtils.setLogLevel();

        final String master = args.length > 0 ? args[0] : "local[4]";
        final String filePath = args.length > 1 ? args[1] : "./";

        final SparkSession spark = SparkSession
                .builder()
                .master(master)
                .appName("Covid19")
                .getOrCreate();

        StructType mySchema = it.polimi.middlewaretechfordistsys.model.Schema.getSchema();

        final Dataset<Row> covidData = spark
                .read()
                .option("header", "false")
                .option("delimiter", ",")
                .schema(mySchema)
                .csv(filePath + "resources/csv/data.csv");


        final int maxDay = covidData.select(max("day")).first().getInt(0);
        final int maxCountries = covidData.select(max("rank")).first().getInt(0);
        final int sevenDays = 7;

        for (int i=0; i<maxDay; i++)
        {
            highscore.put(i, new Top10Countries(i));
            query1and2Result.put(i, new HashMap<>());
        }

        //Calculation for Query1 & Query2 & Query3
        for (int rankId = 0; rankId<maxCountries; rankId++) {

            int[] newReportedCases = new int[sevenDays];

            Dataset<Row> rank= covidData.where("rank="+rankId);

            Double previousMa = 0d;

            for (int i = 0; i < sevenDays; i++) {
                newReportedCases[i] = rank.where("day=" + i).select("infectedIncrement").first().getInt(0);

                int[] newReportedCases2 = new int[i+1];
                System.arraycopy(newReportedCases, 0, newReportedCases2, 0, i + 1);

                previousMa = CalculateValuesAndReturnMovingAverage(rankId, previousMa, i, newReportedCases2);
            }

            for (int k = sevenDays; k<maxDay; k++){
                //noinspection SuspiciousSystemArraycopy
                System.arraycopy(newReportedCases, 1, newReportedCases, 0, sevenDays-1);

                newReportedCases[sevenDays-1] =  rank.where("day=" + k).select("infectedIncrement").first().getInt(0);

                previousMa = CalculateValuesAndReturnMovingAverage(rankId, previousMa, k, newReportedCases);
            }
        }

        //Print Query1
        System.out.println("Query 1");
        for (int i=0; i<maxDay; i++) {
            HashMap<Integer, Country> h1 = query1and2Result.get(i);
            for (int j=0; j<maxCountries; j++) {
                Country country = h1.get(j);
                System.out.println("rank: " + country.countryRank + " day: " + i + " ma: " + country.movingAverageValue);
            }
        }
        System.out.println();

        //Print Query2
        System.out.println("Query 2");
        for (int i=0; i<maxDay; i++) {
            HashMap<Integer, Country> h1 = query1and2Result.get(i);
            for (int j=0; j<maxCountries; j++) {
                Country country = h1.get(j);
                System.out.println("rank: " + country.countryRank + " day: " + i + " perc_ma_inc: " + country.movingAverageIncrease + "%");
            }
        }
        System.out.println();

        //Print Query3
        System.out.println("Query 3");
        for (int i=0; i<maxDay; i++)
        {
            System.out.println("Day " + i);
            highscore.get(i).print();
            System.out.println();
        }
        System.out.println();


        spark.close();

    }

    private static Double CalculateValuesAndReturnMovingAverage(int rankId, Double previousMa, int currentDay, int[] newReportedCases) {
        OptionalDouble movingAverageOptional = Arrays.stream(newReportedCases).average();
        if (movingAverageOptional.isPresent()) {

            Double movingAverage = movingAverageOptional.getAsDouble();

            Double maPercentageIncrease = (movingAverage / previousMa) * 100;

            Country country = new Country(rankId, movingAverage, maPercentageIncrease);

            Top10Countries top10Countries = highscore.get(currentDay);
            if (top10Countries != null) {
                top10Countries.Update(country);
            }

            query1and2Result.get(currentDay).put(rankId, country);

            return movingAverage;
        }

        return null;
    }
}