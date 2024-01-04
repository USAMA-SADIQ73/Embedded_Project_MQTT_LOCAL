/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include <esp_err.h>
#include "mqtt_client.h"

#include "driver/gpio.h"
#include <ultrasonic.h>

#include "ds18b20.h"
static const char *TAG = "MQTT_EXAMPLE";
esp_mqtt_client_handle_t client;



#define ActuatorNODE
//define SensorNODE


 


#ifdef ActuatorNODE

#define LED_PIN_OUT 2
#define LED_Button 26
#define FAN_PIN_OUT 33
#define FAN_Button 25

void ActuatorTask(void *pvParameters)
{
    int msg_id;
    bool ButtonLED = 0;
    bool LEDState = 0;
    bool ButtonFAN = 0;
    bool FANState = 0;
    gpio_set_direction(LED_PIN_OUT, GPIO_MODE_OUTPUT);
    gpio_set_direction(FAN_PIN_OUT, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_Button,GPIO_MODE_INPUT);
    gpio_set_direction(FAN_Button,GPIO_MODE_INPUT);

     while (1)
        {
           
             ButtonLED = gpio_get_level(LED_Button);
             ButtonFAN = gpio_get_level(FAN_Button);
             if(ButtonLED == 1 && LEDState == 0)
             {
               
                gpio_set_level(2,1);
                LEDState = 1;
                msg_id = esp_mqtt_client_publish(client, "/Actuator/LED", "ON", 0, 0, 0);
                ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

             }
             else if(ButtonLED == 1 && LEDState == 1)
             {
                
                gpio_set_level(2,0);
                LEDState = 0;
                msg_id = esp_mqtt_client_publish(client, "/Actuator/LED", "OFF", 0, 0, 0);
                ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

             }
             
            if(ButtonFAN == 1 && FANState == 0)
             {
               
                gpio_set_level(FAN_PIN_OUT,1);
                FANState = 1;
                msg_id = esp_mqtt_client_publish(client, "/Actuator/FAN", "ON", 0, 0, 0);
                ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
               // vTaskDelay(1000 / portTICK_PERIOD_MS);

             }
             else if(ButtonFAN == 1 && FANState == 1)
             {
                
                gpio_set_level(FAN_PIN_OUT,0);
                FANState = 0;
                msg_id = esp_mqtt_client_publish(client, "/Actuator/FAN", "OFF", 0, 0, 0);
                ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

             }
                vTaskDelay(500 / portTICK_PERIOD_MS);
        }

}

#endif


#ifdef SensorNODE

#define MAX_DISTANCE_CM 500 // 5m max
#define TRIGGER_GPIO 5
#define ECHO_GPIO 18

#define TEMP_BUS 26
#define LED 2
#define HIGH 1
#define LOW 0
#define Temp_sensors 2
DeviceAddress tempSensors[Temp_sensors];

void getTempAddresses(DeviceAddress *tempSensorAddresses) {
	unsigned int numberFound = 0;
	reset_search();
	// search for 2 addresses on the oneWire protocol
	while (search(tempSensorAddresses[numberFound],true)) {
		numberFound++;
		if (numberFound == Temp_sensors) break;
	}
	     if (numberFound != Temp_sensors)
		 {
			 printf("Error: Could not find all temp sensors\n");
			 printf("Number found: %d\n", numberFound);
		 }

		 
	return;
}


void Temperature_task(void *pvParameters)
{
    int msg_id;
    char Dtemp1[12];
    char Dtemp2[12];
    float temp1 = 0;
    float temp2 = 0;

    gpio_reset_pin(LED);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

	ds18b20_init(TEMP_BUS);
	getTempAddresses(tempSensors);
	ds18b20_setResolution(tempSensors,Temp_sensors,10);



   	while (1) {
        getTempAddresses(tempSensors);
		ds18b20_requestTemperatures();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
		temp1 = ds18b20_getTempC((DeviceAddress *)tempSensors[0]);
       
		temp2 = ds18b20_getTempC((DeviceAddress *)tempSensors[1]);
         vTaskDelay(1000 / portTICK_PERIOD_MS);
		printf("\nTemperatures : %0.1fC %0.1fC \n", temp1,temp2);
        sprintf(Dtemp1, "%0.1f", temp1);
            sprintf(Dtemp2, "%0.1f", temp2);
         printf("\nsTemp1 = %s ,sTemp2 = %s",Dtemp1,Dtemp2);
      //  vTaskDelay(1000 / portTICK_PERIOD_MS);

             msg_id = esp_mqtt_client_publish(client, "/Sensor/Temp1", Dtemp1, 0, 0, 0);
             ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
               // vTaskDelay(1000 / portTICK_PERIOD_MS);

           
             msg_id = esp_mqtt_client_publish(client, "/Sensor/Temp2", Dtemp2, 0, 0, 0);
             ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
              //  vTaskDelay(1000 / portTICK_PERIOD_MS);
            
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}


}


void ultrasonic_test(void *pvParameters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };

    ultrasonic_init(&sensor);
    float distance = 0;
    char DDistance[12];
    int  msg_id = 0;
    while (1)
    {
        distance = 0;
        esp_err_t res = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK)
        {
            printf("Error %d: ", res);
            switch (res)
            {
                case ESP_ERR_ULTRASONIC_PING:
                    printf("Cannot ping (device is in invalid state)\n");
                    break;
                case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                    printf("Ping timeout (no device found)\n");
                    break;
                case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                    printf("Echo timeout (i.e. distance too big)\n");
                    break;
                default:
                    printf("%s\n", esp_err_to_name(res));
            }
        }
        else
            printf("Distance: %0.02f cm\n", distance*100);
             sprintf(DDistance, "%0.2f",distance*100);
             msg_id = esp_mqtt_client_publish(client, "/Sensor/WaterLevel", DDistance, 0, 0, 0);
             ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
               // vTaskDelay(1000 / portTICK_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
#endif






static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        
        msg_id = esp_mqtt_client_subscribe(client, "/Actuator/LED", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/Actuator/FAN", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);


#ifdef ActuatorNODE    
        //if topic is Actuator/LED and data received it ON turn on LED on pin 2 and if message is OFF turn of LED
        if (strncmp(event->topic, "/Actuator/LED", event->topic_len) == 0) {
            if (strncmp(event->data, "ON", event->data_len) == 0) {
                printf("\non statement\n");
                gpio_set_level(LED_PIN_OUT, 1);
            } else {
                printf("\noff statement\n");
                gpio_set_level(LED_PIN_OUT, 0);
            }
        }
          if (strncmp(event->topic, "/Actuator/FAN", event->topic_len) == 0) {
            if (strncmp(event->data, "ON", event->data_len) == 0) {
                printf("\non FANstatement\n");
                gpio_set_level(FAN_PIN_OUT, 1);
            } else {
                printf("\noff FANstatement\n");
                gpio_set_level(FAN_PIN_OUT, 0);
            }
        }
#endif       


        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();


#ifdef SensorNODE 
    xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(Temperature_task, "Temperature_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
#endif 

#ifdef ActuatorNODE
   xTaskCreate(ActuatorTask, "ActuatorTask", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
#endif   
}
