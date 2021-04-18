package it.polimi.middleware.apache.kafka.connect.producers;

import it.polimi.middleware.apache.kafka.connect.utils.Utils;
import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.Producer;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.serialization.StringSerializer;

import java.util.Properties;

public class MainProducer {
    public static void main(String[] args) {
        Properties props = Utils.setupProducerProps();
        Producer<String, String> producer = new KafkaProducer<String, String>(props);
        for (int i = 0; i < 10; i++) {
            String key = Integer.toString(i);
            String value = "PIRO GAY" + key;
            producer.send(new ProducerRecord<String, String>("kafka-to-mqtt-alert", key, value));
        }
        producer.close();
    }
}
