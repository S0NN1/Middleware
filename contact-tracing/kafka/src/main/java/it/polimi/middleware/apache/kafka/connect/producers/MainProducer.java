package it.polimi.middleware.apache.kafka.connect.producers;

import io.confluent.ksql.api.client.Client;
import io.confluent.ksql.api.client.Row;
import io.confluent.ksql.api.client.StreamedQueryResult;
import it.polimi.middleware.apache.kafka.connect.utils.Utils;
import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.Producer;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.serialization.StringSerializer;

import java.util.Properties;
import java.util.concurrent.ExecutionException;

import static it.polimi.middleware.apache.kafka.connect.utils.Utils.createKSQLClient;

public class MainProducer {
    private static final String ksqlQuery = "SELECT * FROM CONNECTIONS EMIT CHANGES;";
    public static void main(String[] args) throws ExecutionException, InterruptedException {
        Properties props = Utils.setupProducerProps();
        Producer<String, String> producer = new KafkaProducer<String, String>(props);
        Client ksqlClient = createKSQLClient();
        StreamedQueryResult streamedQueryResult = ksqlClient.streamQuery(ksqlQuery).get();

        while(true) {
            // Block until a new row is available
            Row row = streamedQueryResult.poll();
            if (row != null) {
                System.out.println(row.values());
            } else {
                System.out.println("Query has ended.");
                break;
            }
        }
//        for (int i = 0; i < 10; i++) {
//            String key = Integer.toString(i);
//            String value = "PIRO GAY" + key;
//            producer.send(new ProducerRecord<String, String>("kafka-to-mqtt-alert", key, value));
//        }
//        producer.close();
    }
}
