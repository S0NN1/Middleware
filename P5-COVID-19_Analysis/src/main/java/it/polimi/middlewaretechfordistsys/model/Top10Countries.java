package it.polimi.middlewaretechfordistsys.model;

import java.util.ArrayList;

public class Top10Countries {

    //Fields
    @SuppressWarnings("unused")
    public final Integer day;
    private final ArrayList<Country> countryList = new ArrayList<>();

    //Constructor
    public Top10Countries(int day) {
        this.day = day;
    }

    //Constants
    private static final int tenCountries = 10;

    //Update method
    public final void Update(Country country) {
        Integer whereToInsert = FindWhereToInsert(country.movingAverageIncrease);
        if (countryList.size()< tenCountries){

            if (whereToInsert == null) {
                countryList.add(country);
            }
            else {
                countryList.add(whereToInsert, country);
            }
            return;
        }

        if (whereToInsert == null) {
            return;
        }

        if (whereToInsert >= tenCountries)
        {
            return;
        }

        for (int i = countryList.size() -1; i>whereToInsert; i--)
        {
            countryList.set(i, countryList.get(i-1));
        }

        countryList.set(whereToInsert, country);
    }

    private Integer FindWhereToInsert(Double movingAverageIncrease) {
        int lenArray = countryList.size();
        if (lenArray <= 0)
            return 0;


        int low  = 0;
        int high = lenArray - 1;
        int mid;


        while (low <= high) {
            mid = (high + low) / 2;

            if (countryList.get(mid).movingAverageIncrease > movingAverageIncrease ){
                low = mid + 1;
            }
            else if  (countryList.get(mid).movingAverageIncrease < movingAverageIncrease) {
                high = mid - 1;
            }
            else{
                return mid;
            }
        }

        if (movingAverageIncrease > countryList.get(0).movingAverageIncrease)
        {
            return 0;
        }
        if (movingAverageIncrease < countryList.get(lenArray-1).movingAverageIncrease)
        {
            return lenArray;
        }

        return null;
    }

    public final void print() {
        for (int i = 0; i< countryList.size(); i++){
            Country country = countryList.get(i);
            String p = Integer.toString((i+1));

            String cr = Integer.toString(country.countryRank);
            System.out.println(
                    "PositionInHighscore: "+ it.polimi.middlewaretechfordistsys.utils.LogUtils.padLeftZeros(p, 2)+
                    " | CountryRank: "+it.polimi.middlewaretechfordistsys.utils.LogUtils.padLeftZeros(cr,2)+
                    " | MovingAverageIncrease: " + country.movingAverageIncrease);
        }
    }

    @Override
    public final String toString() {
        return "Top10Countries{" +
                "day=" + day +
                ", countryList=" + countryList +
                ", tenCountries=" + tenCountries +
                '}';
    }
}
