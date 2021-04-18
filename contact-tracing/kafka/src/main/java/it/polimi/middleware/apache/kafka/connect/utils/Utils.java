package it.polimi.middleware.apache.kafka.connect.utils;

import org.apache.kafka.common.serialization.StringDeserializer;
import org.apache.kafka.common.serialization.StringSerializer;

import java.util.Properties;

public class Utils {
    private static final String KAFKA_BROKER_IP = "51.103.25.211";

    public static Properties setupConsumerProps() {
        Properties props = new Properties();
        props.put("bootstrap.servers", KAFKA_BROKER_IP + ":9092");
        props.put("group.id", "iot");
        props.put("enable.auto.commit", "true");
        props.put("auto.commit.interval.ms", "1000");
        props.put("key.deserializer", StringDeserializer.class.getName());
        props.put("value.deserializer", StringDeserializer.class.getName());
        return props;
    }

    public static Properties setupProducerProps() {
        Properties props = new Properties();
        props.put("bootstrap.servers", KAFKA_BROKER_IP + ":9092");
        props.put("acks", "all");
        props.put("key.serializer", StringSerializer.class.getName());
        props.put("value.serializer", StringSerializer.class.getName());
        return props;
    }
}

