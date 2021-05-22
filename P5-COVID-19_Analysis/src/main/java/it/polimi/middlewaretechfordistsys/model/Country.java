package it.polimi.middlewaretechfordistsys.model;

import scala.Int;

public class Country {
    public Double movingAverage;
    public Integer countryRank;

    public Country(Integer countryRank, Double movingAverage){
        this.countryRank = countryRank;
        this.movingAverage = movingAverage;
    }
}
