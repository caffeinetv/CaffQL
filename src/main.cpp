#include "CodeGeneration.hpp"
#include "cxxopts.hpp"
#include <fstream>

namespace caffql {

struct ProgramInputs {
    std::string schemaFile;
    std::string outputFile;
    std::string generatedNamespace;
};

ProgramInputs parseCommandLine(int argc, char * argv[]) {
    try {
        cxxopts::Options options("caffql", "Generate c++ types and GraphQL request and response serialization from a GraphQL json schema file.");
        options.add_options()
        ("s,schema", "input json schema file", cxxopts::value<std::string>())
        ("o,output", "output generated header file", cxxopts::value<std::string>())
        ("n,namespace", "generated namespace", cxxopts::value<std::string>()->default_value("caffql"))
        ("h,help", "help");

        auto result = options.parse(argc, argv);

        if (result.count("help") || result.arguments().empty()) {
            printf("%s\n", options.help().c_str());
            exit(0);
        }

        if (!result.count("schema")) {
            printf("input schema is required\n");
            exit(1);
        }

        if (!result.count("output")) {
            printf("output file is required\n");
            exit(1);
        }

        auto schemaFile = result["schema"].as<std::string>();
        auto generatedNamespace = result["namespace"].as<std::string>();
        auto outputFile = result["output"].as<std::string>();

        return {
            result["schema"].as<std::string>(),
            result["output"].as<std::string>(),
            result["namespace"].as<std::string>()
        };
    } catch (cxxopts::OptionException const & e) {
        printf("Error parsing options: %s\n", e.what());
        exit(1);
    }
}

}

int main(int argc, char * argv[]) {
    using namespace caffql;

    auto const inputs = parseCommandLine(argc, argv);

    try {
        std::ifstream file(inputs.schemaFile);
        auto const json = Json::parse(file);
        Schema schema = json.at("data").at("__schema");

        auto const source = generateTypes(schema, inputs.generatedNamespace);
        std::ofstream out(inputs.outputFile);
        out << source;
        out.close();

        printf("Generated %s with namespace %s from %s\n",
               inputs.outputFile.c_str(),
               inputs.generatedNamespace.c_str(),
               inputs.schemaFile.c_str());

        return 0;
    } catch (std::ios_base::failure const & e) {
        printf("File error: %s\n", e.what());
    } catch (Json::parse_error const & e) {
        printf("Error parsing schema file: %s\n", e.what());
    } catch (Json::exception const & e) {
        printf("Error deserializing schema file: %s\n", e.what());
    } catch (std::exception const & e) {
        printf("Error occurred: %s\n", e.what());
    } catch (...) {
        printf("Unknown error occurred\n");
    }

    return 1;
}
