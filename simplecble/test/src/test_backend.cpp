#include <gtest/gtest.h>

#include "simplecble/backend.h"
#include "simplecble/error.h"
#include "simplecble/simplecble.h"

TEST(BackendTest, NamedBackendsFromC) {
    simpleble_error_t* error = nullptr;

    simpleble_backend_t native = simpleble_backend_get_native(&error);
    ASSERT_NE(native, nullptr);
    char* identifier = simpleble_backend_identifier(native, &error);
    EXPECT_STREQ(identifier, "Plain");
    simpleble_free(identifier);
    simpleble_backend_release_handle(native);

    // This build has neither the Dongl nor the Simulator backend.
    EXPECT_EQ(simpleble_backend_get_dongl(&error), nullptr);
    EXPECT_EQ(simpleble_error_code(error), SIMPLEBLE_ERROR_OPERATION_NOT_SUPPORTED);
    EXPECT_EQ(simpleble_backend_get_simulator(&error), nullptr);
    EXPECT_EQ(simpleble_error_code(error), SIMPLEBLE_ERROR_OPERATION_NOT_SUPPORTED);
    simpleble_error_release(&error);
}
