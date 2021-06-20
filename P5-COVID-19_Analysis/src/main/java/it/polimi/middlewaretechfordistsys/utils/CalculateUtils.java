package it.polimi.middlewaretechfordistsys.utils;

import it.polimi.middlewaretechfordistsys.model.Country;
import it.polimi.middlewaretechfordistsys.model.Top10Countries;
import org.apache.spark.sql.Dataset;
import org.apache.spark.sql.Row;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.OptionalDouble;

/**
 * Utility class for calculating things.
 */
public final class CalculateUtils {


    /***
     * This method calculates the values needed for the 3 queries and update the objects query1and2Result and highscore accordingly
     * @param query1and2Result hash map to store into query 1 and 2 results
     * @param highscore hash map to store into query 3 results
     * @param maxDay last day of our simulation
     * @param maxCountries number of countries
     * @param covidData input data
     */
    public static void calculate(Map<Integer, ? extends HashMap<Integer, Country>> query1and2Result,
                                 HashMap<Integer, ? extends Top10Countries> highscore, int maxDay, int maxCountries,
                                 Dataset<Row> covidData) {

        final int sevenDays = 7;


        //for each country
        for (int rankId = 0; rankId < maxCountries; rankId++) {

            int[] newReportedCases = new int[sevenDays];

            Dataset<Row> rank = covidData.where("rank=" + rankId);

            Double previousMa = 0.0d;

            //first seven days
            for (int dayIndex = 0; dayIndex < sevenDays; dayIndex++) {
                newReportedCases[dayIndex] = rank.where("day=" + dayIndex).select("infectedIncrement").first().getInt(0);

                int[] newReportedCases2 = new int[dayIndex + 1];
                System.arraycopy(newReportedCases, 0, newReportedCases2, 0, dayIndex + 1);

                previousMa = calculateValuesAndReturnMovingAverage(rankId, previousMa, dayIndex, newReportedCases2, highscore, query1and2Result);
            }

            //remaining days
            for (int dayIndex = sevenDays; dayIndex < maxDay; dayIndex++) {

                System.arraycopy(newReportedCases, 1, newReportedCases, 0, sevenDays - 1);

                newReportedCases[sevenDays - 1] = rank.where("day=" + dayIndex).select("infectedIncrement").first().getInt(0);

                previousMa = calculateValuesAndReturnMovingAverage(rankId, previousMa, dayIndex,
                        newReportedCases, highscore, query1and2Result);
            }
        }

    }

    /***
     * Calculates the values for the 3 queries and returns the new moving average
     * @param rankId the rank id
     * @param previousMa previous moving average
     * @param currentDay current day
     * @param newReportedCases last 7 days (or less if we didn't analyze enough days) of new reported cases
     * @param highscore object to store into query 3 results
     * @param query1and2Result object to store into query 1 and 2 results
     * @return the new moving average
     */
    private static Double calculateValuesAndReturnMovingAverage(int rankId, Double previousMa, int currentDay,
                                                                int[] newReportedCases,
                                                                HashMap<Integer, ? extends Top10Countries> highscore,
                                                                Map<Integer, ? extends HashMap<Integer, Country>> query1and2Result) {
        OptionalDouble movingAverageOptional = Arrays.stream(newReportedCases).average();
        if (movingAverageOptional.isPresent()) {

            Double movingAverage = movingAverageOptional.getAsDouble();

            Double maPercentageIncrease = (movingAverage / previousMa) * 100.0;

            Country country = new Country(rankId, movingAverage, maPercentageIncrease, rankId, currentDay);

            Top10Countries top10Countries = highscore.get(currentDay);
            if (top10Countries != null) {
                top10Countries.update(country);
            }

            query1and2Result.get(currentDay).put(rankId, country);

            return movingAverage;
        }

        return null;
    }
}
