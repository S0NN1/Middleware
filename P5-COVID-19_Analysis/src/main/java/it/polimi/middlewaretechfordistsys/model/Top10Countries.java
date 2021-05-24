package it.polimi.middlewaretechfordistsys.model;

import java.util.ArrayList;

public class Top10Countries {
    ArrayList<Country> countryList = new ArrayList<>();
    public Integer day;

    public Top10Countries(int day){
        this.day = day;
    }

    final int tenCountries = 10;

    public void Update(Country country) {
        Integer whereToInsert = FindWhereToInsert(country.movingAverageIncrease);
        if (countryList.size()<tenCountries){

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

        for (int i=countryList.size() -1; i>whereToInsert; i--)
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
        int mid = 0;


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

    public String padLeftZeros(String inputString, int length) {
        if (inputString.length() >= length) {
            return inputString;
        }
        StringBuilder sb = new StringBuilder();
        while (sb.length() < length - inputString.length()) {
            sb.append('0');
        }
        sb.append(inputString);

        return sb.toString();
    }

    public void print() {
        if (countryList != null){
            for (int i=0; i<this.countryList.size(); i++){
                Country country = this.countryList.get(i);
                String p = Integer.toString((i+1));

                String cr = Integer.toString(country.countryRank);
                System.out.println("PositionInHighscore: "+padLeftZeros(p, 2)+" | CountryRank: "+padLeftZeros(cr,2)+" | MovingAverage: " + country.movingAverageValue);
            }
        }
    }
}
