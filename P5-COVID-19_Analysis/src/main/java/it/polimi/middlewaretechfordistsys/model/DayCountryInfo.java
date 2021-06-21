package it.polimi.middlewaretechfordistsys.model;

/***
 * Data about a country, in a given rank and day
 */
public class DayCountryInfo {
    public final Double movingAverageValue;
    public final Integer countryRank;
    public final Double movingAverageIncrease;
    private final Integer day;

    public DayCountryInfo(Integer countryRank, Double movingAverageValue, Double movingAverageIncrease, int day) {
        this.countryRank = countryRank;
        this.movingAverageValue = movingAverageValue;
        this.movingAverageIncrease = movingAverageIncrease;
        this.day = day;
    }

    @Override
    public final String toString() {
        return "Country{" +
                "movingAverageValue=" + movingAverageValue +
                ", countryRank=" + countryRank +
                ", movingAverageIncrease=" + movingAverageIncrease +
                ", day=" + day +
                '}';
    }
}
