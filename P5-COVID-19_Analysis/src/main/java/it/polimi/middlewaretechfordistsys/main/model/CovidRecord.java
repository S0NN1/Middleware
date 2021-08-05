package it.polimi.middlewaretechfordistsys.main.model;

import java.io.Serializable;
import java.util.Date;

public class CovidRecord implements Serializable {
    public final Date dateRep;
    public final int day;
    public final int month;
    public final int year;
    public final int cases;
    public final int deaths;
    public final String countriesAndTerritories;
    public final String geoId;
    public final String countryTerritoryCode;
    public final int popData2019;
    public final String continentExp;
    public final double twoWeeksCovidCases;

    public CovidRecord(Date dateRep, int day, int month, int year, int cases, int deaths, String countriesAndTerritories, String geoId, String countryTerritoryCode, int popData2019, String continentExp, double twoWeeksCovidCases) {
        this.dateRep = dateRep;
        this.day = day;
        this.month = month;
        this.year = year;
        this.cases = cases;
        this.deaths = deaths;
        this.countriesAndTerritories = countriesAndTerritories;
        this.geoId = geoId;
        this.countryTerritoryCode = countryTerritoryCode;
        this.popData2019 = popData2019;
        this.continentExp = continentExp;
        this.twoWeeksCovidCases = twoWeeksCovidCases;
    }
}
