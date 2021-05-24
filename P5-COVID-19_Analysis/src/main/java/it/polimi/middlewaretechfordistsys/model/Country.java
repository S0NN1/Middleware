package it.polimi.middlewaretechfordistsys.model;

public class Country {
    public final Double movingAverageValue;
    public final Integer countryRank;
    public final Double movingAverageIncrease;

    public Country(Integer countryRank, Double movingAverageValue, Double movingAverageIncrease){
        this.countryRank = countryRank;
        this.movingAverageValue = movingAverageValue;
        this.movingAverageIncrease = movingAverageIncrease;
    }
}
