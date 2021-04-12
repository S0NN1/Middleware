package it.polimi.middlewaretechfordistsys.model;

import org.apache.spark.sql.Dataset;
import org.apache.spark.sql.Row;
import org.apache.spark.sql.SparkSession;
import org.apache.spark.sql.types.DataTypes;
import org.apache.spark.sql.types.StructField;
import org.apache.spark.sql.types.StructType;

import java.util.ArrayList;
import java.util.List;

public class Schema {
    private final List<StructField> schemaFields = new ArrayList<>();
    private final StructType schema;

    public Schema(SparkSession sparkSession, String fileName) {
        schemaFields.add(DataTypes.createStructField("dateRep", DataTypes.DateType, false));
        schemaFields.add(DataTypes.createStructField("day", DataTypes.IntegerType, false));
        schemaFields.add(DataTypes.createStructField("month", DataTypes.IntegerType, false));
        schemaFields.add(DataTypes.createStructField("year", DataTypes.IntegerType, false));
        schemaFields.add(DataTypes.createStructField("cases", DataTypes.IntegerType, false));
        schemaFields.add(DataTypes.createStructField("deaths", DataTypes.IntegerType, false));
        schemaFields.add(DataTypes.createStructField("countriesAndTerritories", DataTypes.StringType, false));
        schemaFields.add(DataTypes.createStructField("geoId", DataTypes.StringType, false));
        schemaFields.add(DataTypes.createStructField("countryterritoryCode", DataTypes.StringType, false));
        schemaFields.add(DataTypes.createStructField("popData2019", DataTypes.IntegerType, false));
        schemaFields.add(DataTypes.createStructField("continentExp", DataTypes.StringType, false));
        schemaFields.add(DataTypes.createStructField("Cumulative_number_for_14_days_of_COVID-19_cases_per_100000", DataTypes.IntegerType, false));
        schema = DataTypes.createStructType(schemaFields);

        Dataset<Row> covid19CasesWorldwide = sparkSession.read()
                .option("header", "false")
                .option("delimiter", ",")
                .schema(schema)
                .csv(fileName);
    }

}
