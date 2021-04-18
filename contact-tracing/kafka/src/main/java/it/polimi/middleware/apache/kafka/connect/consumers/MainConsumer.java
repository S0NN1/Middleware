package it.polimi.middleware.apache.kafka.connect.consumers;

import it.polimi.middleware.apache.kafka.connect.producers.MainProducer;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.json.JSONObject;

import java.time.Duration;
import java.util.Arrays;
import java.util.Properties;

import static it.polimi.middleware.apache.kafka.connect.utils.Utils.setupConsumerProps;

public class MainConsumer {
    public static void main(String[] args) {
        Properties props = setupConsumerProps();
        KafkaConsumer<String, String> consumer = new KafkaConsumer<String, String>(props);
        consumer.subscribe(Arrays.asList("mqtt-to-kafka-connections"));
        while (true) {
            ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(100));
            for (ConsumerRecord<String, String> record : records) {
                JSONObject json = new JSONObject(record.value().substring(5));
                System.out.println(json);
            }
        }
    }
}