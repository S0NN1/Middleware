package it.polimi.middlewaretechfordistsys.model;

public class Country {
    public Double movingAverageValue;
    public Integer countryRank;
    public Double movingAverageIncrease;

    public Country(Integer countryRank, Double movingAverageValue, Double movingAverageIncrease){
        this.countryRank = countryRank;
        this.movingAverageValue = movingAverageValue;
        this.movingAverageIncrease = movingAverageIncrease;
    }
}
