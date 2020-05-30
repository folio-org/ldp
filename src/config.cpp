#include "../etymoncpp/include/util.h"
#include "config.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"

constexpr json::ParseFlag pflags = json::kParseTrailingCommasFlag;

Config::Config(const string& config)
{
    string configFile;
    if (config != "") {
        configFile = config;
    } else {
        char* s = getenv("LDPCONFIG");
        configFile = s ? s : "";
        etymon::trim(&configFile);
        if (configFile.empty())
            throw runtime_error("configuration file not specified");
    }
    // Load and parse JSON file.
    etymon::File f(configFile, "r");
    char readBuffer[65536];
    json::FileReadStream is(f.file, readBuffer, sizeof(readBuffer));
    jsondoc.ParseStream<pflags>(is);
}

bool Config::get(const string& key, string* value) const
{
    if (const json::Value* v = json::Pointer(key.c_str()).Get(jsondoc)) {
        if (v->IsString() == false)
            throw runtime_error(
                    "Unexpected data type in configuration setting:\n"
                    "    Key: " + key + "\n"
                    "    Expected type: string");
        *value = v->GetString();
        return true;
    } else {
        return false;
    }
}

bool Config::getBool(const string& key, bool* value) const
{
    if (const json::Value* v = json::Pointer(key.c_str()).Get(jsondoc)) {
        if (v->IsBool() == false)
            throw runtime_error(
                    "Unexpected data type in configuration setting:\n"
                    "    Key: " + key + "\n"
                    "    Expected type: Boolean");
        *value = v->GetBool();
        return true;
    } else {
        return false;
    }
}

bool Config::getInt(const string& key, int* value) const
{
    if (const json::Value* v = json::Pointer(key.c_str()).Get(jsondoc)) {
        if (v->IsInt() == false)
            throw runtime_error(
                    "Unexpected data type in configuration setting:\n"
                    "    Key: " + key + "\n"
                    "    Expected type: integer");
        *value = v->GetInt();
        return true;
    } else {
        return false;
    }
}

void Config::getRequired(const string& key, string* value) const
{
    if (!get(key, value))
        throw runtime_error("configuration value not found: " + key);
}

void Config::getOptional(const string& key, string* value) const
{
    if (!get(key, value)) {
        value->clear();
    }
}
