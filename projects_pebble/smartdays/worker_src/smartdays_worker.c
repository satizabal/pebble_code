#include <pebble_worker.h>

#define DATA_LOG_TAG_ACCELEROMETER 51
#define BUFFER_SIZE 25
#define BUFFER_SIZE_BYTES sizeof(uint64_t)+(3*BUFFER_SIZE*sizeof(int16_t))

#define WORKER_DOTS 10
static DataLoggingSessionRef s_log_ref;
static int packets_sent = 0;

typedef struct packet {
  uint64_t timestamp;
  int16_t xyz[BUFFER_SIZE][3];
} accel_packet;                 //if BUFFER_SIZE_BYTES is not a multiple of 8, C appends some bytes to perform memory packing (8 bytes)

static accel_packet to_send;


static void data_handler(AccelRawData *data, uint32_t num_samples, uint64_t timestamp) {
  uint16_t i;
  static DataLoggingResult result = DATA_LOGGING_SUCCESS;
  
  to_send.timestamp = timestamp;
  for (i = 0; i < num_samples; i++) {
    to_send.xyz[i][0] = (int16_t)data[i].x;                 //save the measures
    to_send.xyz[i][1] = (int16_t)data[i].y;
    to_send.xyz[i][2] = (int16_t)data[i].z;
  }
  result = data_logging_log(s_log_ref, &to_send, 1);        //push the data
                                                            //data are sent to the phone (if available) ~every minute (I don't know how to change that)
  if (result == DATA_LOGGING_SUCCESS) {
    packets_sent++;
  }

  AppWorkerMessage msg_data;
  msg_data.data0 = packets_sent % WORKER_DOTS;
  msg_data.data1 = result;
  app_worker_send_message(WORKER_DOTS, &msg_data);          //send a message to the application
}

void delayed_logging(void *data) {
  // Start logging
  s_log_ref = data_logging_create(DATA_LOG_TAG_ACCELEROMETER, DATA_LOGGING_BYTE_ARRAY, BUFFER_SIZE_BYTES, true);
  // Subscribe to the accelerometer data service
  accel_raw_data_service_subscribe(BUFFER_SIZE, data_handler);
  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
}

static void worker_init() {
  app_timer_register(1000, delayed_logging, NULL);
}

static void worker_deinit() {
  // Finish logging session
  accel_data_service_unsubscribe();
  data_logging_finish(s_log_ref);
}

int main(void) {
  worker_init();
  worker_event_loop();
  worker_deinit();
}
