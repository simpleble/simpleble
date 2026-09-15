#include <stdio.h>
#include <stdlib.h>

#include <simplecble/simplecble.h>

#define PERIPHERAL_LIST_SIZE (size_t)10

static void clean_on_exit(void);

static void adapter_on_scan_start(simpleble_adapter_t adapter, void* userdata);
static void adapter_on_scan_stop(simpleble_adapter_t adapter, void* userdata);
static void adapter_on_scan_found(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata);

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

    size_t services_count = simpleble_peripheral_services_count(peripheral, &error);
    printf("Successfully connected, listing %zu services.\n", services_count);

    for (size_t i = 0; i < services_count; i++) {
        simpleble_service_t service = {0};
        simpleble_peripheral_services_get(peripheral, i, &service, &error);
        if (error) continue;

        printf("Service: %s - (%zu characteristics)\n", service.uuid.value, service.characteristic_count);
        for (size_t j = 0; j < service.characteristic_count; j++) {
            printf("  Characteristic: %s - (%zu descriptors)\n", service.characteristics[j].uuid.value,
                   service.characteristics[j].descriptor_count);
            for (size_t k = 0; k < service.characteristics[j].descriptor_count; k++) {
                printf("    Descriptor: %s\n", service.characteristics[j].descriptors[k].uuid.value);
            }
        }
        simpleble_service_release(&service);
    }

    simpleble_peripheral_disconnect(peripheral, &error);
    if (error) goto cleanup;

    result = EXIT_SUCCESS;

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
