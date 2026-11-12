#include "lemon_cli/opencode_profile.h"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

int main() {
    int failures = 0;
    auto check = [&failures](bool ok, const char* name) {
        std::printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
        if (!ok) ++failures;
    };
    for (const auto& [version, major] : std::vector<std::pair<std::string, int>>{
             {"opencode v2.2.7\n", 2}, {"2.2.7", 2}, {"v2.0.0-beta.1", 2}, {"1.2.3", 1},
             {"opencode v1.9.0", 1}, {"v10.0.0", 10}, {"999.9.9", 999}, {"99999999999999.1.1", 0},
             {"fake-agent-ok", 0}, {"", 0}, {"error: 2.2.7", 0}}) {
        check(lemon_cli::opencode_major_version(version) == major, version.c_str());
    }

    const std::vector<lemon_cli::AgentModelEntry> models = {
        {"Qwen3.8-Flash-Next-Halogen", "Qwen (local)", 65536, {"chat", "tool-calling"}},
        {"Vision", "Vision", 8192, {"chat", "vision"}},
        {"Plain", "Plain", 8192, {"chat"}},
        {"VisionTools", "Vision tools", 8192, {"chat", "vision", "tool-calling"}}
    };
    const auto config = lemon_cli::build_opencode_v2_config(
        "https://server.example/v1", "Qwen3.8-Flash-Next-Halogen", models);
    check(config.at("model") == "Lemonade/Qwen3.8-Flash-Next-Halogen", "selected model");
    const auto& provider = config.at("providers").at("Lemonade");
    check(provider.at("package") == "@opencode/ai/providers/openai-compatible", "v2 package");
    check(provider.at("settings").at("baseURL") == "https://server.example/v1", "endpoint");
    check(provider.at("env") == nlohmann::json::array({"LEMONADE_API_KEY"}), "credential environment");
    check(provider.at("models").at("Qwen3.8-Flash-Next-Halogen").at("limit").at("context") == 65536,
          "saved context size");
    check(!provider.contains("npm") && !provider.contains("options"), "v2 schema");

    const auto& configured_models = provider.at("models");
    check(configured_models.at("Qwen3.8-Flash-Next-Halogen").value("capabilities", nlohmann::json()) ==
              nlohmann::json{{"tools", true}, {"input", {"text"}}, {"output", {"text"}}},
          "text tool model capabilities");
    check(configured_models.at("Vision").value("capabilities", nlohmann::json()) ==
              nlohmann::json{{"tools", false}, {"input", {"text", "image"}}, {"output", {"text"}}},
          "vision model without tools capabilities");
    check(configured_models.at("Plain").value("capabilities", nlohmann::json()) ==
              nlohmann::json{{"tools", false}, {"input", {"text"}}, {"output", {"text"}}},
          "plain text model capabilities");
    check(configured_models.at("VisionTools").value("capabilities", nlohmann::json()) ==
              nlohmann::json{{"tools", true}, {"input", {"text", "image"}}, {"output", {"text"}}},
          "vision tool model capabilities");

    const auto legacy = lemon_cli::opencode_profile().build_provider_block(
        "http://localhost:8000/v1", "test-key", models);
    check(legacy.at("npm") == "@ai-sdk/openai-compatible", "v1 package preserved");
    check(legacy.at("options").at("apiKey") == "test-key", "v1 credentials preserved");
    for (const auto& model : models) {
        check(!legacy.at("models").at(model.id).contains("capabilities"), "v1 capabilities unchanged");
    }
    return failures ? 1 : 0;
}
