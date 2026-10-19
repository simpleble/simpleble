#include <gtest/gtest.h>

#include "simplecble/simplecble.h"

// Each listed handle is a new wrapper, so bindings match local objects by identity instead.
TEST(LocalIdentityTest, ListedHandlesShareTheIdentityOfTheirObject) {
    simpleble_error_t* error = nullptr;
    simpleble_adapter_list_t adapters = nullptr;
    simpleble_adapter_get_adapters(&adapters, &error);
    simpleble_adapter_t adapter = simpleble_adapter_list_get(adapters, 0, &error);
    simpleble_adapter_list_release(adapters);
    ASSERT_EQ(error, nullptr);
    simpleble_local_peripheral_t host = simpleble_adapter_create_local_peripheral(adapter, &error);
    ASSERT_EQ(error, nullptr);

    simpleble_local_service_t service = simpleble_local_peripheral_add_service(
        host, {"0000aaa0-0000-1000-8000-00805f9b34fb"}, &error);
    simpleble_local_characteristic_t first = simpleble_local_service_add_characteristic(
        service, {"0000aaa1-0000-1000-8000-00805f9b34fb"}, SIMPLEBLE_LOCAL_CHARACTERISTIC_READ, &error);
    simpleble_local_characteristic_t second = simpleble_local_service_add_characteristic(
        service, {"0000aaa2-0000-1000-8000-00805f9b34fb"}, SIMPLEBLE_LOCAL_CHARACTERISTIC_READ, &error);
    ASSERT_EQ(error, nullptr);

    simpleble_local_service_list_t services = nullptr;
    simpleble_local_peripheral_services(host, &services, &error);
    simpleble_local_service_t listed_service = simpleble_local_service_list_get(services, 0, &error);
    simpleble_local_service_list_release(services);
    simpleble_local_characteristic_list_t characteristics = nullptr;
    simpleble_local_service_characteristics(listed_service, &characteristics, &error);
    simpleble_local_characteristic_t listed_first = simpleble_local_characteristic_list_get(characteristics, 0, &error);
    simpleble_local_characteristic_t listed_second = simpleble_local_characteristic_list_get(characteristics, 1,
                                                                                             &error);
    simpleble_local_characteristic_list_release(characteristics);
    ASSERT_EQ(error, nullptr);

    EXPECT_NE(listed_service, service);
    EXPECT_EQ(simpleble_local_service_identity(listed_service), simpleble_local_service_identity(service));
    EXPECT_EQ(simpleble_local_characteristic_identity(listed_first), simpleble_local_characteristic_identity(first));
    EXPECT_EQ(simpleble_local_characteristic_identity(listed_second), simpleble_local_characteristic_identity(second));
    EXPECT_NE(simpleble_local_characteristic_identity(first), simpleble_local_characteristic_identity(second));

    simpleble_local_characteristic_release_handle(listed_second);
    simpleble_local_characteristic_release_handle(listed_first);
    simpleble_local_service_release_handle(listed_service);
    simpleble_local_characteristic_release_handle(second);
    simpleble_local_characteristic_release_handle(first);
    simpleble_local_service_release_handle(service);
    simpleble_local_peripheral_release_handle(host);
    simpleble_adapter_release_handle(adapter);
    simpleble_error_release(&error);
}
