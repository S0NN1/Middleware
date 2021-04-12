package it.polimi.middleware.apache.kafka.connect.consumers;

import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;

import java.time.Duration;
import java.util.Arrays;
import java.util.Properties;

import static it.polimi.middleware.apache.kafka.connect.utils.Utils.setupProps;

public class AlertConsumer {
    public static void main(String[] args) {
        Properties props = setupProps();
        KafkaConsumer<String, String> consumer = new KafkaConsumer<String, String>(props);
        consumer.subscribe(Arrays.asList("quickstart-events"));
        while (true) {
            ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(100));
            for (ConsumerRecord<String, String> record : records) {

            }
        }
    }
}
