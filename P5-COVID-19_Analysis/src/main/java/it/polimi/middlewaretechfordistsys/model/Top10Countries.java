package it.polimi.middlewaretechfordistsys.model;

import scala.Int;

import java.awt.*;
import java.util.ArrayList;

public class Top10Countries {
    ArrayList<Country> countryList = new ArrayList<>();
    public Integer day;

    public Top10Countries(int day){
        this.day = day;
    }


    public void Update(int countryRank, Double movingAverage) {
        Integer whereToInsert = FindWhereToInsert(movingAverage);
        if (countryList.size()<10){

            if (whereToInsert == null) {
                countryList.add(new Country(countryRank, movingAverage));
            }
            else {
                countryList.add(whereToInsert, new Country(countryRank, movingAverage));
            }
            return;
        }

        if (whereToInsert == null) {
            return;
        }

        if (whereToInsert >= 10)
        {
            return;
        }

        for (int i=countryList.size() -1; i>whereToInsert; i--)
        {
            countryList.set(i, countryList.get(i-1));
        }

        countryList.set(whereToInsert, new Country(countryRank, movingAverage));
    }

    private Integer FindWhereToInsert(Double movingAverage) {
        int lenArray = countryList.size();
        if (lenArray <= 0)
            return 0;


        int low  = 0;
        int high = lenArray - 1;
        int mid = 0;


        while (low <= high) {
            mid = (high + low) / 2;

            //If x is greater, ignore left half
            if (countryList.get(mid).movingAverage > movingAverage ){
                low = mid + 1;
            }

            //If x is smaller, ignore right half
            else if  (countryList.get(mid).movingAverage  < movingAverage) {
                high = mid - 1;
            }

            //means x is present at mid
            else{
                return mid;
            }
        }

        if (movingAverage > countryList.get(0).movingAverage)
        {
            return 0;
        }
        if (movingAverage < countryList.get(lenArray-1).movingAverage)
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
                System.out.println("PositionInHighscore: "+padLeftZeros(p, 2)+" | CountryRank: "+padLeftZeros(cr,2)+" | MovingAverage: " + country.movingAverage);
            }
        }
    }
}
