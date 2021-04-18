package it.polimi.middleware.apache.kafka.connect.producers;

import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.Producer;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.serialization.StringSerializer;

import java.util.Properties;

public class MainProducer {
    public static void main(String[] args) {
        Properties props = new Properties();
        props.put("bootstrap.servers","51.103.30.161:9092");
        props.put("acks","all");
        props.put("key.serializer", StringSerializer.class.getName());
        props.put("value.serializer", StringSerializer.class.getName());
        Producer<String,String> producer = new KafkaProducer<String, String>(props);
        for (int i = 0; i < 10; i++) {
            System.out.println("Itero");
            String key = Integer.toString(i);
            String value = "PIRO GAY" + key;
            producer.send(new ProducerRecord<String, String>("quickstart-events", key, value));
        }
        producer.close();
    }
}
