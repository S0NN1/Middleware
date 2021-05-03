package it.polimi.middleware.apache.kafka.connect.consumers;

import io.confluent.ksql.api.client.Client;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;

import java.time.Duration;
import java.util.Arrays;
import java.util.Properties;
import java.util.concurrent.ExecutionException;

import static it.polimi.middleware.apache.kafka.connect.utils.Utils.createKSQLClient;
import static it.polimi.middleware.apache.kafka.connect.utils.Utils.setupConsumerProps;

public class MainConsumer {
    private static final String ksqlStream = "CREATE STREAM CONNECTIONS (" +
            "EventType VARCHAR , " +
            "ClientId VARCHAR, " +
            "SenderClientId VARCHAR) " +
            "WITH (" +
            "KAFKA_TOPIC='mqtt-to-kafka-connections', " +
            "VALUE_FORMAT='json');";
    public static void main(String[] args) throws ExecutionException, InterruptedException {
        Properties props = setupConsumerProps();
        KafkaConsumer<String, String> consumer = new KafkaConsumer<String, String>(props);
        consumer.subscribe(Arrays.asList("mqtt-to-kafka-connections"));
        Client ksqlClient = createKSQLClient();
        ksqlClient.executeStatement(ksqlStream).get();
        while (true) {
            ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(100));
            for (ConsumerRecord<String, String> record : records) {
                System.out.println(record.value());
            }
        }
    }
}