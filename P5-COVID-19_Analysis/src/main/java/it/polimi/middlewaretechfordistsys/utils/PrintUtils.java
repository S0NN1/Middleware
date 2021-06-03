package it.polimi.middlewaretechfordistsys.utils;

import it.polimi.middlewaretechfordistsys.model.Country;
import it.polimi.middlewaretechfordistsys.model.Top10Countries;

import java.util.HashMap;
import java.util.Map;

public final class PrintUtils {

    /***
     * Print the query results
     * @param query1and2Result object where query 1 and 2 results are stored
     * @param highscore object where query 3 result is stored
     * @param maxDay last day
     * @param maxCountries number of countries
     */
    public static void print(HashMap<Integer, ? extends HashMap<Integer, Country>> query1and2Result, Map<Integer, ? extends Top10Countries> highscore, int maxDay, int maxCountries) {
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
    }
}
