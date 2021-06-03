package it.polimi.middlewaretechfordistsys.model;

/***
 * Data about a country, in a given rank and day
 */
public class Country {
    public final Double movingAverageValue;
    public final Integer countryRank;
    public final Double movingAverageIncrease;
    private final Integer rank;
    private final Integer day;

    public Country(Integer countryRank, Double movingAverageValue, Double movingAverageIncrease, int rank, int day) {
        this.countryRank = countryRank;
        this.movingAverageValue = movingAverageValue;
        this.movingAverageIncrease = movingAverageIncrease;
        this.rank = rank;
        this.day = day;
    }

    @Override
    public final String toString() {
        return "Country{" +
                "movingAverageValue=" + movingAverageValue +
                ", countryRank=" + countryRank +
                ", movingAverageIncrease=" + movingAverageIncrease +
                ", rank=" + rank +
                ", day=" + day +
                '}';
    }
}
