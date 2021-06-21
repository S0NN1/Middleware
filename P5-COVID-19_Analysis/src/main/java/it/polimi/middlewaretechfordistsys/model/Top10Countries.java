package it.polimi.middlewaretechfordistsys.model;

import java.util.ArrayList;

/**
 * This class contains information about the top 10 countries.
 */
public class Top10Countries {

    // Fields
    public final Integer day;
    private final ArrayList<Country> countryList = new ArrayList<>();

    // Constructor
    public Top10Countries(int day) {
        this.day = day;
    }

    /**
     * Update country list by inserting the provided country in the right position relying on its moving average.
     * @param country the country to be added.
     */
    public final void update(Country country) {
        Integer whereToInsert = findWhereToInsert(country.movingAverageIncrease);
        if (countryList.size() < 10) {

            if (whereToInsert == null) {
                countryList.add(country);
            } else {
                countryList.add(whereToInsert, country);
            }
            return;
        }

        if (whereToInsert == null) {
            return;
        }

        if (whereToInsert >= 10) {
            return;
        }

        for (int i = countryList.size() - 1; i > whereToInsert; i--) {
            countryList.set(i, countryList.get(i - 1));
        }

        countryList.set(whereToInsert, country);
    }

    /**
     * Compute the position in the list of the country based on its moving average.
     * @param movingAverageIncrease the seven days moving average of new reported cases for that country.
     * @return the position in which the country has to be added.
     */
    private Integer findWhereToInsert(Double movingAverageIncrease) {
        int lenArray = countryList.size();
        if (lenArray <= 0)
            return 0;


        int low = 0;
        int high = lenArray - 1;
        int mid;


        while (low <= high) {
            mid = (high + low) / 2;

            if (countryList.get(mid).movingAverageIncrease > movingAverageIncrease) {
                low = mid + 1;
            } else if (countryList.get(mid).movingAverageIncrease < movingAverageIncrease) {
                high = mid - 1;
            } else {
                return mid;
            }
        }

        if (movingAverageIncrease > countryList.get(0).movingAverageIncrease) {
            return 0;
        }
        if (movingAverageIncrease < countryList.get(lenArray - 1).movingAverageIncrease) {
            return lenArray;
        }

        return null;
    }

    /**
     * Prints the countries report.
     */
    public final void print() {
        for (int i = 0; i < countryList.size(); i++) {
            Country country = countryList.get(i);
            String p = Integer.toString((i + 1));

            String cr = Integer.toString(country.countryRank);
            System.out.println(
                    "PositionInHighscore: " + it.polimi.middlewaretechfordistsys.utils.LogUtils.padLeftZeros(p, 2) +
                            " | CountryRank: " + it.polimi.middlewaretechfordistsys.utils.LogUtils.padLeftZeros(cr, 2) +
                            " | MovingAverageIncrease: " + country.movingAverageIncrease);
        }
    }

    @Override
    public final String toString() {
        return "Top10Countries{" +
                "day=" + day +
                ", countryList=" + countryList +
                ", numberOfCountries=" + 10 +
                '}';
    }
}
