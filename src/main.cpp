#include "CodeGeneration.hpp"
#include <fstream>

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Please provide an input schema and namespace\n");
        return 0;
    }

    std::ifstream file{argv[1]};
    auto const json = Json::parse(file);
    Schema schema = json.at("data").at("__schema");

    auto const source = generateTypes(schema, argv[2]);
    std::ofstream out{"Generated.hpp"};
    out << source;
    out.close();

    return 0;
}
