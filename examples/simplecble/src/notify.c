#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

void msleep(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

#include <simplecble/simplecble.h>

#define PERIPHERAL_LIST_SIZE (size_t)10
#define SERVICES_LIST_SIZE (size_t)32

typedef struct {
    simpleble_uuid_t service;
    simpleble_uuid_t characteristic;
} service_characteristic_t;

static void clean_on_exit(void);

static void adapter_on_scan_start(simpleble_adapter_t adapter, void* userdata);
static void adapter_on_scan_stop(simpleble_adapter_t adapter, void* userdata);
static void adapter_on_scan_found(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata);
static void peripheral_on_notify(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                 simpleble_uuid_t characteristic, const uint8_t* data, size_t data_length,
                                 void* userdata);

static simpleble_peripheral_t peripheral_list[PERIPHERAL_LIST_SIZE] = {0};
static size_t peripheral_list_len = 0;
static simpleble_adapter_t adapter = NULL;

int main(void) {
    atexit(clean_on_exit);

    int result = EXIT_FAILURE;
    simpleble_error_t* error = NULL;
    size_t adapter_count = simpleble_adapter_get_count(&error);
    if (error) goto cleanup;
    if (adapter_count == 0) {
        printf("No adapter was found.\n");
        goto cleanup;
    }

    // TODO: Allow the user to pick an adapter.
    adapter = simpleble_adapter_get_handle(0, &error);
    if (error) goto cleanup;

    simpleble_adapter_set_callback_on_scan_start(adapter, adapter_on_scan_start, NULL);
    simpleble_adapter_set_callback_on_scan_stop(adapter, adapter_on_scan_stop, NULL);
    simpleble_adapter_set_callback_on_scan_found(adapter, adapter_on_scan_found, NULL);

    simpleble_adapter_scan_for(adapter, 5000, &error);
    if (error) goto cleanup;

    printf("The following devices were found:\n");
    for (size_t i = 0; i < peripheral_list_len; i++) {
        char* peripheral_identifier = simpleble_peripheral_identifier(peripheral_list[i], &error);
        char* peripheral_address = simpleble_peripheral_address(peripheral_list[i], &error);
        printf("[%zu] %s [%s]\n", i, peripheral_identifier ? peripheral_identifier : "Unknown",
               peripheral_address ? peripheral_address : "Unknown");
        simpleble_free(peripheral_identifier);
        simpleble_free(peripheral_address);
    }

    printf("Please select a device to connect to: ");
    int selection = -1;
    if (scanf("%d", &selection) != 1 || selection < 0 || selection >= (int)peripheral_list_len) {
        printf("Invalid selection.\n");
        goto cleanup;
    }

    simpleble_peripheral_t peripheral = peripheral_list[selection];
    char* peripheral_identifier = simpleble_peripheral_identifier(peripheral, &error);
    char* peripheral_address = simpleble_peripheral_address(peripheral, &error);
    printf("Connecting to %s [%s]\n", peripheral_identifier ? peripheral_identifier : "Unknown",
           peripheral_address ? peripheral_address : "Unknown");
    simpleble_free(peripheral_identifier);
    simpleble_free(peripheral_address);

    simpleble_peripheral_connect(peripheral, &error);
    if (error) goto cleanup;

    printf("Successfully connected, listing services and characteristics.\n");

    size_t services_count = simpleble_peripheral_services_count(peripheral, &error);
    service_characteristic_t characteristic_list[SERVICES_LIST_SIZE] = {0};
    size_t characteristic_count = 0;
    for (size_t i = 0; i < services_count; i++) {
        simpleble_service_t service;
        simpleble_peripheral_services_get(peripheral, i, &service, &error);
        if (error) continue;

        for (size_t j = 0; j < service.characteristic_count; j++) {
            if (characteristic_count >= SERVICES_LIST_SIZE) break;

            printf("[%zu] %s %s\n", characteristic_count, service.uuid.value, service.characteristics[j].uuid.value);
            characteristic_list[characteristic_count].service = service.uuid;
            characteristic_list[characteristic_count].characteristic = service.characteristics[j].uuid;
            characteristic_count++;
        }
    }

    printf("Please select a characteristic to subscribe to: ");
    int characteristic_selection = -1;
    if (scanf("%d", &characteristic_selection) != 1 || characteristic_selection < 0 ||
        characteristic_selection >= (int)characteristic_count) {
        printf("Invalid selection.\n");
        goto disconnect;
    }
    service_characteristic_t selected_characteristic = characteristic_list[characteristic_selection];

    simpleble_peripheral_notify(peripheral, selected_characteristic.service, selected_characteristic.characteristic,
                                peripheral_on_notify, NULL, &error);
    if (error) goto disconnect;

    // Sleep for 5 seconds.
    msleep(5000);
    simpleble_peripheral_unsubscribe(peripheral, selected_characteristic.service,
                                     selected_characteristic.characteristic, &error);
    if (error) goto disconnect;
    result = EXIT_SUCCESS;

disconnect:
    if (error) fprintf(stderr, "%s\n", simpleble_error_message(error));
    simpleble_peripheral_disconnect(peripheral, &error);
    if (error) result = EXIT_FAILURE;

cleanup:
    if (error) fprintf(stderr, "%s\n", simpleble_error_message(error));
    simpleble_error_release(&error);
    return result;
}

static void clean_on_exit(void) {
    printf("Releasing allocated resources.\n");
    for (size_t i = 0; i < peripheral_list_len; i++) {
        simpleble_peripheral_release_handle(peripheral_list[i]);
    }
    simpleble_adapter_release_handle(adapter);
}

static void adapter_on_scan_start(simpleble_adapter_t adapter, void* userdata) {
    simpleble_error_t* error = NULL;
    char* identifier = simpleble_adapter_identifier(adapter, &error);
    printf("Adapter %s started scanning.\n", identifier ? identifier : "Unknown");
    simpleble_free(identifier);
    simpleble_error_release(&error);
}

static void adapter_on_scan_stop(simpleble_adapter_t adapter, void* userdata) {
    simpleble_error_t* error = NULL;
    char* identifier = simpleble_adapter_identifier(adapter, &error);
    printf("Adapter %s stopped scanning.\n", identifier ? identifier : "Unknown");
    simpleble_free(identifier);
    simpleble_error_release(&error);
}

static void adapter_on_scan_found(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata) {
    simpleble_error_t* error = NULL;

    char* adapter_identifier = simpleble_adapter_identifier(adapter, &error);
    char* peripheral_identifier = simpleble_peripheral_identifier(peripheral, &error);
    char* peripheral_address = simpleble_peripheral_address(peripheral, &error);

    printf("Adapter %s found device: %s [%s]\n", adapter_identifier ? adapter_identifier : "Unknown",
           peripheral_identifier ? peripheral_identifier : "Unknown",
           peripheral_address ? peripheral_address : "Unknown");
    if (peripheral_list_len < PERIPHERAL_LIST_SIZE) {
        peripheral_list[peripheral_list_len++] = peripheral;
    } else {
        simpleble_peripheral_release_handle(peripheral);
    }

    simpleble_free(adapter_identifier);
    simpleble_free(peripheral_identifier);
    simpleble_free(peripheral_address);
    simpleble_error_release(&error);
}

static void peripheral_on_notify(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                 simpleble_uuid_t characteristic, const uint8_t* data, size_t data_length,
                                 void* userdata) {
    printf("Received: ");
    for (size_t i = 0; i < data_length; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}
