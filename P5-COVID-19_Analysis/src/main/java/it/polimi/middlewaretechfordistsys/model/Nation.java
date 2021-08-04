package it.polimi.middlewaretechfordistsys.model;

import java.io.Serializable;
import java.util.Date;

public class Nation implements Serializable {
    private final String continentExp;
    private final String country;
    private final Date date;

    public Nation(String continentExp, String country, Date date) {
        this.continentExp = continentExp;
        this.country = country;
        this.date = date;
    }

    public String getContinentExp() {
        return continentExp;
    }

    public String getCountry() {
        return country;
    }

    public Date getDate() {
        return date;
    }
}
