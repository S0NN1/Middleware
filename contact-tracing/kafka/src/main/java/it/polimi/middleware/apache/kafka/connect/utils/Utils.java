package it.polimi.middleware.apache.kafka.connect.utils;

import org.apache.kafka.common.serialization.StringDeserializer;

import java.util.Properties;

public class Utils {
    public static Properties setupProps() {
        Properties props = new Properties();
        props.put("bootstrap.servers", "51.103.30.161:9092");
        props.put("group.id", "iot");
        props.put("enable.auto.commit", "true");
        props.put("auto.commit.interval.ms", "1000");
        props.put("key.deserializer", StringDeserializer.class.getName());
        props.put("value.deserializer", StringDeserializer.class.getName());
        return props;
    }
}

