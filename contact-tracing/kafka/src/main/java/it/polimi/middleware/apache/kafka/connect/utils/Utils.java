package it.polimi.middleware.apache.kafka.connect.utils;

import io.confluent.ksql.api.client.Client;
import io.confluent.ksql.api.client.ClientOptions;
import org.apache.kafka.common.serialization.StringDeserializer;
import org.apache.kafka.common.serialization.StringSerializer;

import java.util.Properties;

public class Utils {
    private static final String KAFKA_BROKER_IP = "51.103.25.211";
    private static final String KAFKA_BROKER_PORT = "9092";
    private static final int KAFKA_KSQL_PORT = 8088;

    public static Properties setupConsumerProps() {
        Properties props = new Properties();
        props.put("bootstrap.servers", KAFKA_BROKER_IP + ":" + KAFKA_BROKER_PORT);
        props.put("group.id", "iot");
        props.put("enable.auto.commit", "true");
        props.put("auto.commit.interval.ms", "1000");
        props.put("key.deserializer", StringDeserializer.class.getName());
        props.put("value.deserializer", StringDeserializer.class.getName());
        return props;
    }

    public static Properties setupProducerProps() {
        Properties props = new Properties();
        props.put("bootstrap.servers", KAFKA_BROKER_IP + ":" + KAFKA_BROKER_PORT);
        props.put("acks", "all");
        props.put("key.serializer", StringSerializer.class.getName());
        props.put("value.serializer", StringSerializer.class.getName());
        return props;
    }
    public static Client createKSQLClient(){
        ClientOptions options = ClientOptions.create()
                .setHost(KAFKA_BROKER_IP)
                .setPort(KAFKA_KSQL_PORT);
        return Client.create(options);
    }
}

