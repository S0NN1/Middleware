package it.polimi.middleware.kafka.utils;

import io.confluent.ksql.api.client.Client;
import io.confluent.ksql.api.client.ClientOptions;
import org.apache.kafka.common.serialization.StringSerializer;

import java.util.Properties;

/**
 * Utils class used for setup function (initially thought to be used in multiple classes).
 */
public class Utils {
    private static final String KAFKA_BROKER_IP = "51.103.25.211";
    private static final String KAFKA_BROKER_PORT = "9092";
    private static final int KAFKA_KSQL_PORT = 8088;
    private static final String MAX_IDLE_TIMEOUT = "86400000";

    /**
     * Sets producer props.
     *
     * @return the producer props.
     */
    public static Properties setupProducerProps() {
        Properties props = new Properties();
        props.put("bootstrap.servers", KAFKA_BROKER_IP + ":" + KAFKA_BROKER_PORT);
        props.put("acks", "all");
        props.put("connections.max.idle.ms", MAX_IDLE_TIMEOUT);
        props.put("key.serializer", StringSerializer.class.getName());
        props.put("value.serializer", StringSerializer.class.getName());
        return props;
    }

    /**
     * Create ksql client client with host and port defined.
     *
     * @return the client.
     */
    public static Client createKSQLClient() {
        ClientOptions options = ClientOptions.create()
                .setHost(KAFKA_BROKER_IP)
                .setPort(KAFKA_KSQL_PORT);
        return Client.create(options);
    }
}

