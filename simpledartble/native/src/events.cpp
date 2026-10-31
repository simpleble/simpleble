#include <simpledartble/simpledartble.h>

#include <dart_native_api.h>
#include <simplecble/peripheral.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace {

// NOTE: Dart_PostCObject copies the message, and a port whose isolate has shut down rejects it. SimpleBLE keeps
// calling back after a Flutter hot restart, so callbacks never call into Dart directly.
using PostCObject = bool (*)(Dart_Port port, Dart_CObject* message);

std::atomic<PostCObject> post_cobject{nullptr};
std::atomic<int64_t> next_token{1};
std::atomic<int64_t> log_token{0};

// The port events go to, and the claim that set it. Posting holds the mutex, so once a claim is released, nothing
// posts to a Dart VM that may be shutting down.
std::mutex owner_mutex;
Dart_Port owner = ILLEGAL_PORT;
int64_t owner_claim = 0;
int64_t next_claim = 1;

bool post(Dart_Port port, Dart_CObject* message) {
    auto function = post_cobject.load();
    return function != nullptr && port != ILLEGAL_PORT && function(port, message);
}

// Posts to the current owner, if any.
bool post_to_owner(Dart_CObject* message) {
    std::scoped_lock lock(owner_mutex);
    return post(owner, message);
}

Dart_Port current_owner() {
    std::scoped_lock lock(owner_mutex);
    return owner;
}

Dart_CObject null() {
    Dart_CObject object{};
    object.type = Dart_CObject_kNull;
    return object;
}

Dart_CObject integer(int64_t value) {
    Dart_CObject object{};
    object.type = Dart_CObject_kInt64;
    object.value.as_int64 = value;
    return object;
}

Dart_CObject text(const char* value) {
    if (value == nullptr) return null();
    Dart_CObject object{};
    object.type = Dart_CObject_kString;
    object.value.as_string = const_cast<char*>(value);
    return object;
}

Dart_CObject bytes(const uint8_t* data, size_t length) {
    static const uint8_t empty = 0;
    Dart_CObject object{};
    object.type = Dart_CObject_kTypedData;
    object.value.as_typed_data.type = Dart_TypedData_kUint8;
    object.value.as_typed_data.length = static_cast<intptr_t>(length);
    object.value.as_typed_data.values = length > 0 ? data : &empty;
    return object;
}

Dart_CObject array(Dart_CObject** values, intptr_t length) {
    Dart_CObject object{};
    object.type = Dart_CObject_kArray;
    object.value.as_array.length = length;
    object.value.as_array.values = values;
    return object;
}

int64_t token_of(void* userdata) { return reinterpret_cast<intptr_t>(userdata); }

// Posts [token] or [token, payload].
bool post_event(void* userdata, Dart_CObject* payload) {
    Dart_CObject token = integer(token_of(userdata));
    Dart_CObject* values[] = {&token, payload};
    Dart_CObject message = array(values, payload != nullptr ? 2 : 1);
    return post_to_owner(&message);
}

// A callback waiting for Dart's answer. Without one, it uses its fallback.
struct Question {
    Dart_Port port = ILLEGAL_PORT;
    std::mutex mutex;
    std::condition_variable condition;
    bool answered = false;
    bool has_answer = false;
    std::vector<uint8_t> bytes;
    std::string text;
    bool flag = false;
};

std::mutex questions_mutex;
std::map<int64_t, std::shared_ptr<Question>> questions;
std::atomic<int64_t> next_question{1};

std::shared_ptr<Question> take_question(int64_t id) {
    std::scoped_lock lock(questions_mutex);
    auto found = questions.find(id);
    if (found == questions.end()) return nullptr;
    auto question = found->second;
    questions.erase(found);
    return question;
}

template <typename Fill>
void answer(int64_t id, Fill fill) {
    auto question = take_question(id);
    if (!question) return;
    {
        std::scoped_lock lock(question->mutex);
        fill(*question);
        question->answered = true;
    }
    question->condition.notify_all();
}

// Posts [token, question, arguments] and waits until Dart answers, or until nothing can anymore.
std::shared_ptr<Question> ask(void* userdata, std::initializer_list<const char*> arguments = {}) {
    auto question = std::make_shared<Question>();
    const int64_t id = next_question++;
    {
        std::scoped_lock lock(questions_mutex);
        question->port = current_owner();
        questions[id] = question;
    }

    std::vector<Dart_CObject> strings;
    for (const char* argument : arguments) {
        strings.push_back(text(argument != nullptr ? argument : ""));
    }
    std::vector<Dart_CObject*> string_values;
    for (auto& string : strings) {
        string_values.push_back(&string);
    }

    Dart_CObject token = integer(token_of(userdata));
    Dart_CObject number = integer(id);
    Dart_CObject list = array(string_values.data(), static_cast<intptr_t>(string_values.size()));
    Dart_CObject* values[] = {&token, &number, &list};
    Dart_CObject message = array(values, 3);
    bool posted = false;
    {
        std::scoped_lock lock(owner_mutex);
        posted = owner == question->port && post(owner, &message);
    }
    if (!posted) {
        answer(id, [](Question&) {});
    }

    std::unique_lock lock(question->mutex);
    question->condition.wait(lock, [&] { return question->answered; });
    return question;
}

}  // namespace

void simpledartble_initialize(void* function) { post_cobject = reinterpret_cast<PostCObject>(function); }

namespace {

// Gives the questions that were not asked of the current owner their fallback.
void abandon_questions() {
    std::vector<int64_t> abandoned;
    {
        const Dart_Port port = current_owner();
        std::scoped_lock lock(questions_mutex);
        for (const auto& [id, question] : questions) {
            if (question->port != port) abandoned.push_back(id);
        }
    }
    for (const int64_t id : abandoned) {
        answer(id, [](Question&) {});
    }
}

}  // namespace

int64_t simpledartble_claim(int64_t port) {
    int64_t claim = 0;
    {
        std::scoped_lock lock(owner_mutex);
        if (owner != ILLEGAL_PORT && owner != port) {
            Dart_CObject ping = null();
            if (post(owner, &ping)) return 0;
        }
        owner = port;
        owner_claim = next_claim++;
        claim = owner_claim;
    }
    abandon_questions();
    return claim;
}

void simpledartble_release(void* claim) {
    {
        std::scoped_lock lock(owner_mutex);
        if (owner_claim != reinterpret_cast<intptr_t>(claim)) return;
        owner = ILLEGAL_PORT;
        owner_claim = 0;
    }
    abandon_questions();
}

int64_t simpledartble_token(void) { return next_token++; }

void simpledartble_on_signal(void*, void* userdata) { post_event(userdata, nullptr); }

void simpledartble_on_peripheral(simpleble_adapter_t, simpleble_peripheral_t peripheral, void* userdata) {
    Dart_CObject handle = integer(reinterpret_cast<intptr_t>(peripheral));
    if (!post_event(userdata, &handle)) simpleble_peripheral_release_handle(peripheral);
}

void simpledartble_on_value(simpleble_peripheral_t, simpleble_uuid_t, simpleble_uuid_t, const uint8_t* data,
                            size_t data_length, void* userdata) {
    Dart_CObject value = bytes(data, data_length);
    post_event(userdata, &value);
}

void simpledartble_on_bytes(void*, const uint8_t* data, size_t data_length, void* userdata) {
    Dart_CObject value = bytes(data, data_length);
    post_event(userdata, &value);
}

void simpledartble_on_text(void*, const char* value, void* userdata) {
    Dart_CObject string = text(value);
    post_event(userdata, &string);
}

const uint8_t* simpledartble_on_read(simpleble_local_characteristic_t, size_t* data_length, void* userdata) {
    // NOTE: SimpleCBLE copies the value on this thread right after the callback returns.
    thread_local std::vector<uint8_t> value;
    auto question = ask(userdata);
    value = question->has_answer ? question->bytes : std::vector<uint8_t>{};
    *data_length = value.size();
    return value.empty() ? nullptr : value.data();
}

namespace {

// Writes a question's six-digit answer into passkey, or returns false without one.
bool take_passkey(const Question& question, char passkey[7]) {
    if (!question.has_answer || question.text.size() != 6) return false;
    std::memcpy(passkey, question.text.c_str(), 7);
    return true;
}

}  // namespace

bool simpledartble_on_passkey_request(simpleble_peripheral_t, char passkey[7], void* userdata) {
    return take_passkey(*ask(userdata), passkey);
}

bool simpledartble_on_numeric_comparison(simpleble_peripheral_t, const char* passkey, void* userdata) {
    auto question = ask(userdata, {passkey});
    return question->has_answer && question->flag;
}

bool simpledartble_on_linux_passkey_request(simpleble_adapter_t, const char* peer, char passkey[7], void* userdata) {
    return take_passkey(*ask(userdata, {peer}), passkey);
}

void simpledartble_on_linux_passkey_display(simpleble_adapter_t, const char* peer, const char* passkey,
                                            void* userdata) {
    Dart_CObject fields[] = {text(peer), text(passkey)};
    Dart_CObject* values[] = {&fields[0], &fields[1]};
    Dart_CObject pair = array(values, 2);
    post_event(userdata, &pair);
}

bool simpledartble_on_linux_numeric_comparison(simpleble_adapter_t, const char* peer, const char* passkey,
                                               void* userdata) {
    auto question = ask(userdata, {peer, passkey});
    return question->has_answer && question->flag;
}

bool simpledartble_on_linux_pairing_authorization(simpleble_adapter_t, const char* peer, void* userdata) {
    auto question = ask(userdata, {peer});
    return question->has_answer && question->flag;
}

void simpledartble_answer_bytes(int64_t id, const uint8_t* data, size_t data_length) {
    answer(id, [&](Question& question) {
        question.bytes.assign(data, data + data_length);
        question.has_answer = true;
    });
}

void simpledartble_answer_text(int64_t id, const char* value) {
    answer(id, [&](Question& question) {
        question.text = value != nullptr ? value : "";
        question.has_answer = value != nullptr;
    });
}

void simpledartble_answer_flag(int64_t id, bool flag) {
    answer(id, [&](Question& question) {
        question.flag = flag;
        question.has_answer = true;
    });
}

void simpledartble_answer_none(int64_t id) {
    answer(id, [](Question&) {});
}

void simpledartble_log_to(int64_t token) { log_token = token; }

void simpledartble_on_log(int level, const char* module, const char* file, uint32_t line, const char* function,
                          const char* message) {
    Dart_CObject fields[] = {
        integer(level), text(module), text(file), integer(line), text(function), text(message),
    };
    Dart_CObject* values[] = {&fields[0], &fields[1], &fields[2], &fields[3], &fields[4], &fields[5]};
    Dart_CObject entry = array(values, 6);
    post_event(reinterpret_cast<void*>(static_cast<intptr_t>(log_token.load())), &entry);
}
