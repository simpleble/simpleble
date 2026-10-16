#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>

#include <simpleble/Logging.h>

namespace {

class LoggingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        original_directory_ = std::filesystem::current_path();
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        directory_ = std::filesystem::temp_directory_path() / ("simpleble-logging-" + std::to_string(suffix));
        directory_created_ = std::filesystem::create_directory(directory_);
        ASSERT_TRUE(directory_created_);
        std::filesystem::current_path(directory_);
        previous_level_ = SimpleBLE::Logging::Logger::get()->get_level();
    }

    void TearDown() override {
        auto logger = SimpleBLE::Logging::Logger::get();
        logger->log_default_stdout();
        logger->set_level(previous_level_);
        std::filesystem::current_path(original_directory_);
        if (directory_created_) std::filesystem::remove_all(directory_);
    }

    std::filesystem::path directory_;

  private:
    std::filesystem::path original_directory_;
    bool directory_created_{false};
    SimpleBLE::Logging::Level previous_level_{SimpleBLE::Logging::Info};
};

TEST_F(LoggingTest, DefaultFilePreservesTimestampNameAndMessage) {
    auto logger = SimpleBLE::Logging::Logger::get();
    logger->set_level(SimpleBLE::Logging::Info);
    logger->log_default_file();
    logger->log(SimpleBLE::Logging::Info, "test", "logging.cpp", 17, "default_file", "file logging works");

    const auto begin = std::filesystem::directory_iterator(directory_);
    ASSERT_NE(begin, std::filesystem::directory_iterator{});
    const auto path = begin->path();
    auto next = begin;
    ASSERT_EQ(++next, std::filesystem::directory_iterator{});
    EXPECT_TRUE(std::regex_match(path.filename().string(),
                                std::regex("simpleble_[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{2}-[0-9]{2}-[0-9]{2}")));

    std::ifstream file(path);
    ASSERT_TRUE(file.is_open());
    const std::string contents{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    EXPECT_EQ(contents, "[INFO] test: logging.cpp:17 in default_file: file logging works\n");
}

}  // namespace
