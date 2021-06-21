package it.polimi.middlewaretechfordistsys.model;

import org.apache.spark.sql.types.DataTypes;
import org.apache.spark.sql.types.StructField;
import org.apache.spark.sql.types.StructType;

import java.util.ArrayList;
import java.util.List;

/**
 * schema:
 *  {
 *      day :Integer,
 *      rank :Integer,
 *      infected :Integer,
 *      sane :Integer,
 *      infected_increment :Double,
 *      sane_increment :Double
 *  }
 */
public final class Schema {

    public static StructType getSchema() {
        List<StructField> mySchemaFields = new ArrayList<>();
        mySchemaFields.add(DataTypes.createStructField("day", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("rank", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("infectedValue", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("saneValue", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("infectedIncrement", DataTypes.IntegerType, false));
        mySchemaFields.add(DataTypes.createStructField("saneIncrement", DataTypes.IntegerType, false));
        return DataTypes.createStructType(mySchemaFields);
    }
}
