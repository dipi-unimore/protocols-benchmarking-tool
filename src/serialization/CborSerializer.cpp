#include "pbt/serialization/CborSerializer.hpp"
#include <cbor.h>
#include <nlohmann/json.hpp>
#include <format>
#include <stdexcept>

namespace pbt {

// Encode a nlohmann::json value to CBOR using tinycbor.
static void encode_value(CborEncoder* enc, const nlohmann::json& v);

static void encode_object(CborEncoder* enc, const nlohmann::json& obj) {
    CborEncoder map;
    cbor_encoder_create_map(enc, &map, obj.size());
    for (auto& [k, v] : obj.items()) {
        cbor_encode_text_stringz(&map, k.c_str());
        encode_value(&map, v);
    }
    cbor_encoder_close_container(enc, &map);
}

static void encode_array(CborEncoder* enc, const nlohmann::json& arr) {
    CborEncoder array;
    cbor_encoder_create_array(enc, &array, arr.size());
    for (auto& elem : arr) encode_value(&array, elem);
    cbor_encoder_close_container(enc, &array);
}

static void encode_value(CborEncoder* enc, const nlohmann::json& v) {
    if (v.is_null())            cbor_encode_null(enc);
    else if (v.is_boolean())    cbor_encode_boolean(enc, v.get<bool>());
    else if (v.is_number_integer()) cbor_encode_int(enc, v.get<int64_t>());
    else if (v.is_number_float())   cbor_encode_double(enc, v.get<double>());
    else if (v.is_string()) {
        auto s = v.get<std::string>();
        cbor_encode_text_string(enc, s.c_str(), s.size());
    } else if (v.is_object())   encode_object(enc, v);
    else if (v.is_array())      encode_array(enc, v);
}

std::expected<Bytes, Error>
CborSerializer::serialize(std::span<const uint8_t> payload) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(payload.begin(), payload.end());
    } catch (const std::exception& e) {
        return std::unexpected(Error{
            std::format("CBOR serialize: invalid JSON input: {}", e.what())});
    }

    // Two-pass: estimate then encode
    Bytes buf(payload.size() * 2 + 64);
    for (int attempt = 0; attempt < 3; ++attempt) {
        CborEncoder enc;
        cbor_encoder_init(&enc, buf.data(), buf.size(), 0);
        encode_value(&enc, j);
        std::size_t needed = cbor_encoder_get_extra_bytes_needed(&enc);
        if (needed == 0) {
            buf.resize(cbor_encoder_get_buffer_size(&enc, buf.data()));
            return buf;
        }
        buf.resize(buf.size() + needed + 64);
    }
    return std::unexpected(Error{"CBOR encode: buffer resize failed"});
}

// Decode CBOR to JSON string bytes
static nlohmann::json decode_value(CborValue* it);

static nlohmann::json decode_object(CborValue* map) {
    nlohmann::json obj = nlohmann::json::object();
    CborValue elem;
    cbor_value_enter_container(map, &elem);
    while (!cbor_value_at_end(&elem)) {
        size_t len;
        cbor_value_get_string_length(&elem, &len);
        std::string key(len, '\0');
        cbor_value_copy_text_string(&elem, key.data(), &len, &elem);
        key.resize(len);
        obj[key] = decode_value(&elem);
        cbor_value_advance(&elem);
    }
    cbor_value_leave_container(map, &elem);
    return obj;
}

static nlohmann::json decode_array(CborValue* arr) {
    nlohmann::json result = nlohmann::json::array();
    CborValue elem;
    cbor_value_enter_container(arr, &elem);
    while (!cbor_value_at_end(&elem)) {
        result.push_back(decode_value(&elem));
        cbor_value_advance(&elem);
    }
    cbor_value_leave_container(arr, &elem);
    return result;
}

static nlohmann::json decode_value(CborValue* it) {
    CborType t = cbor_value_get_type(it);
    switch (t) {
        case CborNullType:    return nullptr;
        case CborBooleanType: {
            bool b; cbor_value_get_boolean(it, &b); return b;
        }
        case CborIntegerType: {
            int64_t n; cbor_value_get_int64(it, &n); return n;
        }
        case CborDoubleType: {
            double d; cbor_value_get_double(it, &d); return d;
        }
        case CborTextStringType: {
            size_t len; cbor_value_get_string_length(it, &len);
            std::string s(len, '\0');
            cbor_value_copy_text_string(it, s.data(), &len, nullptr);
            s.resize(len);
            return s;
        }
        case CborMapType:   return decode_object(it);
        case CborArrayType: return decode_array(it);
        default:            return nullptr;
    }
}

std::expected<Bytes, Error>
CborSerializer::deserialize(std::span<const uint8_t> data) {
    CborParser parser;
    CborValue  root;
    CborError  err = cbor_parser_init(data.data(), data.size(), 0, &parser, &root);
    if (err != CborNoError)
        return std::unexpected(Error{std::format("CBOR parse error: {}", static_cast<int>(err))});
    try {
        auto j = decode_value(&root);
        auto s = j.dump();
        return Bytes(s.begin(), s.end());
    } catch (const std::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

}  // namespace pbt
