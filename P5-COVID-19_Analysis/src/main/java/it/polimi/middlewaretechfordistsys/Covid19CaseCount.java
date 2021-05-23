package it.polimi.middlewaretechfordistsys;

import java.util.*;

import it.polimi.middlewaretechfordistsys.model.Top10Countries;
import org.apache.spark.sql.Dataset;
import org.apache.spark.sql.RelationalGroupedDataset;
import org.apache.spark.sql.Row;
import org.apache.spark.sql.SparkSession;
import org.apache.spark.sql.types.DataTypes;
import org.apache.spark.sql.types.StructField;
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
    private static final boolean useCache = true;

    public static void main(String[] args) {
        LogUtils.setLogLevel();

        final String master = args.length > 0 ? args[0] : "local[4]";
        final String filePath = args.length > 1 ? args[1] : "./";
        final String appName = useCache ? "BankWithCache" : "BankNoCache";

        final SparkSession spark = SparkSession
                .builder()
                .master(master)
                .appName("Bank")
                .getOrCreate();


        final List<StructField> mySchemaFields = new ArrayList<>();
        mySchemaFields.add(DataTypes.createStructField("day", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("rank", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("infectedValue", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("saneValue", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("infectedIncrement", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("saneIncrement", DataTypes.IntegerType, false));
        final StructType mySchema = DataTypes.createStructType(mySchemaFields);

        final Dataset<Row> covidData = spark
                .read()
                .option("header", "false")
                .option("delimiter", ",")
                .schema(mySchema)
                .csv(filePath + "resources/csv/data.csv");



        int maxDay = covidData.select(max("day")).first().getInt(0);
        int maxCountries = covidData.select(max("rank")).first().getInt(0);


        System.out.println(maxDay);


        //Query1 & Query 2 & Query 3
        HashMap<Integer, Top10Countries> highscore = new HashMap<>();

        for (int i=0; i<maxDay; i++)
        {
            highscore.put(i, new Top10Countries(i));
        }

        for (int rankId = 0; rankId<maxCountries; rankId++) {
            System.out.println("Country:" + rankId);
            int[] newReportedCases = new int[7];

            Dataset<Row> rank= covidData.where("rank="+rankId);

            Double previousMa = 0d;

            for (int i = 0; i < 7; i++) {
                newReportedCases[i] = rank.where("day=" + i).select("infectedIncrement").first().getInt(0);
                System.out.println(newReportedCases[i]);

                int[] newReportedCases2 = new int[i+1];
                System.arraycopy(newReportedCases, 0, newReportedCases2, 0, i + 1);

                Double movingAverage = Arrays.stream(newReportedCases2).average().getAsDouble();
                System.out.println("rank: " + rankId + " day: " + i + " ma:" + movingAverage);

                Top10Countries top10Countries = highscore.get(i);
                if (top10Countries != null) {
                    top10Countries.Update(rankId, movingAverage);
                }

                Double maPercentageIncrease = (movingAverage / previousMa) * 100;
                System.out.println("rank: " + rankId + " day: " + i + " perc_ma_inc:" + maPercentageIncrease + "%");
                previousMa = movingAverage;
            }

            for (int k = 7; k<maxDay; k++){

                System.arraycopy(newReportedCases, 1, newReportedCases, 0, 7-1);

                newReportedCases[7-1] =  rank.where("day=" + k).select("infectedIncrement").first().getInt(0);

                Double movingAverage = Arrays.stream(newReportedCases).average().getAsDouble();
                System.out.println("rank: " + rankId + "day: " + k + " ma:" + movingAverage);

                highscore.get(k).Update(rankId, movingAverage);

                Double maPercentageIncrease = (movingAverage / previousMa) * 100;
                System.out.println("rank: " + rankId + "day: " + k + " perc_ma_inc:" + maPercentageIncrease + "%");
                previousMa = movingAverage;
            }
        }

        //Print Query 3

        for (int i=0; i<maxDay; i++)
        {
            System.out.println("Day " + i);
            highscore.get(i).print();
            System.out.println("");
        }


/*
        RelationalGroupedDataset groupBy = covidData.groupBy("rank", "day");

        Dataset<Row> sum = groupBy.sum("infected");
        sum.show();
        
 */

        spark.close();

    }
}