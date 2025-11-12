#include "src/compiler/Lexer.h"
#include "src/compiler/Parser.h"
#include "src/compiler/TypeChecker.h"
#include "src/compiler/CodeGenerator.h"
#include "src/vm/VM.h"
#include "src/debugger/Debugger.h"
#include <iostream>
#include <fstream>
#include <cstring>

#include "src/debugger/Debugger.h"
#include "src/native/Native.h"
#include "src/vm/Loader.h"

enum class Mode {
    COMPILE,
    RUN,
    BUILD_AND_RUN,
    DEBUG,
    HELP
};

struct Config {
    Mode mode = Mode::HELP;
    std::string inputFile;
    std::string outputFile;
    bool verbose = false;
    bool keepBytecode = false;
    bool debugMode = false;
};



void print_help(const char *program_name) {
    std::cout << "Droplet\n\n";
    std::cout << "Use:\n";
    std::cout << "  " << program_name << " [command] [options] <file>\n\n";

    std::cout << "Commands:\n";
    std::cout << "  compile <input.drop> [-o <output.dbc>]   Compile source to bytecode\n";
    std::cout << "  run <input.dbc>                          Run bytecode file\n";
    std::cout << "  exec <input.drop>                        Compile and run (temp bytecode)\n";
    std::cout << "  debug <input.drop>                       Compile and debug interactively\n";
    std::cout << "  help                                     Show help\n\n";

    std::cout << "Options:\n";
    std::cout << "  -o <file>     Specify output file (default: <input>.dbc)\n";
    std::cout << "  -v, --verbose Enable verbose output\n";
    std::cout << "  -k, --keep    Keep bytecode after exec (use with exec)\n";
    std::cout << "  -d, --debug   Enable debug mode\n\n";

    std::cout << "Debug Mode:\n";
    std::cout << "  When running in debug mode, you'll enter an interactive debugger\n";
    std::cout << "  similar to GDB. Type 'help' in the debugger for available commands.\n\n";
}

Config parse_args(const int argc, char *argv[]) {
    Config config;

    if (argc < 2) {
        config.mode = Mode::HELP;
        return config;
    }

    const std::string command = argv[1];

    // Parse command
    if (command == "compile" || command == "c") {
        config.mode = Mode::COMPILE;
    } else if (command == "run" || command == "r") {
        config.mode = Mode::RUN;
    } else if (command == "exec" || command == "e") {
        config.mode = Mode::BUILD_AND_RUN;
    } else if (command == "debug" || command == "d") {
        config.mode = Mode::DEBUG;
        config.debugMode = true;
    } else if (command == "help" || command == "-h" || command == "--help") {
        config.mode = Mode::HELP;
        return config;
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        std::cerr << "Use 'help' to see available commands\n";
        config.mode = Mode::HELP;
        return config;
    }

    // Parse remaining arguments
    int i = 2;
    while (i < argc) {
        std::string arg = argv[i];

        if (arg == "-o" && i + 1 < argc) {
            config.outputFile = argv[++i];
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "-k" || arg == "--keep") {
            config.keepBytecode = true;
        } else if (arg == "-d" || arg == "--debug") {
            config.debugMode = true;
        } else if (arg[0] != '-') {
            config.inputFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
        }
        i++;
    }

    // Validate input file
    if (config.mode != Mode::HELP && config.inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        config.mode = Mode::HELP;
        return config;
    }

    // Set default output file for compile mode
    if (config.mode == Mode::COMPILE && config.outputFile.empty()) {
        const size_t dotPos = config.inputFile.find_last_of('.');
        if (dotPos != std::string::npos) {
            config.outputFile = config.inputFile.substr(0, dotPos) + ".dbc";
        } else {
            config.outputFile = config.inputFile + ".dbc";
        }
    }

    // Set temp output for exec/debug mode
    if ((config.mode == Mode::BUILD_AND_RUN || config.mode == Mode::DEBUG)
        && config.outputFile.empty()) {
        config.outputFile = ".temp_droplet.dbc";
    }

    return config;
}

std::vector<std::string> readSourceLines(const std::string &inputPath) {
    std::ifstream file(inputPath);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

void loadDebugInfoForModules(Debugger *debugger, const ModuleLoader &moduleLoader) {
    // Get all loaded modules
    const auto &modules = moduleLoader.getLoadedModules();

    for (const auto &[modulePath, moduleInfo]: modules) {
        if (!moduleInfo || moduleInfo->filePath.empty()) {
            continue;
        }

        // Read source lines for this module
        std::vector<std::string> lines = readSourceLines(moduleInfo->filePath);

        if (!lines.empty()) {
            std::cerr << "[DEBUG] Loading source for module: " << moduleInfo->filePath << "\n";
            debugger->setSourceFile(moduleInfo->filePath, lines);
        }
    }
}

bool compile_source(const std::string &inputPath, const std::string &outputPath,
                    bool verbose, bool generateDebugInfo,
                    std::map<uint32_t, FunctionDebugInfo> *debugInfo = nullptr,
                    ModuleLoader *moduleLoader = nullptr) {
    // ADD THIS PARAMETER
    try {
        // Read source file
        std::ifstream file(inputPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << inputPath << "\n";
            return false;
        }

        std::string source((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        if (verbose) {
            std::cout << "=== Compiling " << inputPath << " ===\n\n";
        }

        // Lexical Analysis
        if (verbose) std::cout << "[1/4] Lexical Analysis...\n";
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        if (verbose) std::cout << "      Generated " << tokens.size() << " tokens\n\n";

        // Parsing
        if (verbose) std::cout << "[2/4] Parsing...\n";
        Parser parser(tokens);
        Program program = parser.parse();
        if (verbose) {
            std::cout << "      Classes: " << program.classes.size() << "\n";
            std::cout << "      Functions: " << program.functions.size() << "\n";
            std::cout << "      FFI Declarations: " << program.ffiDecls.size() << "\n\n";
        }

        // USE THE PROVIDED MODULE LOADER OR CREATE A NEW ONE
        ModuleLoader *loaderToUse = moduleLoader;
        std::unique_ptr<ModuleLoader> tempLoader;
        if (!loaderToUse) {
            tempLoader = std::make_unique<ModuleLoader>();
            loaderToUse = tempLoader.get();
        }

        // Type Checking
        if (verbose) std::cout << "[3/4] Type Checking...\n";
        TypeChecker typeChecker;
        typeChecker.registerFFIFunctions(program.functions);
        typeChecker.setModuleLoader(loaderToUse);
        typeChecker.check(program);
        if (verbose) std::cout << "      Type checking completed successfully\n";

        // Print class information
        if (verbose) {
            const auto &classes = typeChecker.getClassInfo();
            for (const auto &[name, info]: classes) {
                if (name == "list" || name == "dict" || name == "str") continue;

                std::cout << "      Class " << name;
                if (!info.parentClass.empty()) {
                    std::cout << " : " << info.parentClass;
                }
                std::cout << "\n";
                std::cout << "        Fields: " << info.fields.size() << "\n";
                std::cout << "        Methods: " << info.methods.size() << "\n";
                std::cout << "        Total field count: " << info.totalFieldCount << "\n";
            }
            std::cout << "\n";
        }

        // Code Generation
        if (verbose) std::cout << "[4/4] Code Generation...\n";
        CodeGenerator codegen(typeChecker);
        codegen.setModuleLoader(loaderToUse);
        codegen.setGenerateDebugInfo(generateDebugInfo);
        codegen.setSourceFile(inputPath);

        bool success = codegen.generateWithModules(program, outputPath);

        if (success) {
            // Extract debug info if requested
            if (generateDebugInfo && debugInfo) {
                *debugInfo = codegen.getDebugInfo();
            }

            if (verbose) {
                std::cout << "      Successfully generated " << outputPath << "\n\n";
                std::cout << "=== Compilation Complete ===\n";
            } else {
                std::cout << "Compiled: " << inputPath << " -> " << outputPath << "\n";
            }
            return true;
        } else {
            std::cerr << "Code generation failed\n";
            return false;
        }
    } catch (const ParseError &e) {
        std::cerr << "Parser Error: " << e.what() << "\n";
        return false;
    } catch (const TypeChecker::TypeError &e) {
        std::cerr << "Type Error: " << e.what() << "\n";
        return false;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
}

bool run_bytecode(const std::string &bytecodeFile, bool verbose, bool debugMode,
                  const std::map<uint32_t, FunctionDebugInfo> *debugInfo = nullptr,
                  const std::string &sourceFile = "") {
    try {
        if (verbose) {
            std::cout << "=== Running " << bytecodeFile << " ===\n\n";
        }

        VM vm;
        Loader loader;

        register_native_functions(vm);

        // Load bytecode file
        if (!loader.load_dbc_file(bytecodeFile, vm)) {
            std::cerr << "Failed to load bytecode file: " << bytecodeFile << "\n";
            return false;
        }

        if (verbose) {
            std::cout << "Loaded " << vm.functions.size() << " functions\n";
            std::cout << "Global constants: " << vm.global_constants.size() << "\n\n";
        }

        // Setup debugger if in debug mode
        Debugger *debugger = nullptr;
        if (debugMode) {
            debugger = new Debugger(&vm);
            vm.setDebugger(debugger);

            // Load debug info
            if (debugInfo) {
                for (const auto &[idx, info]: *debugInfo) {
                    debugger->addFunctionDebugInfo(idx, info);
                }
            }

            // Load source file
            if (!sourceFile.empty()) {
                std::vector<std::string> lines = readSourceLines(sourceFile);
                debugger->setSourceFile(sourceFile, lines);
            }

            debugger->start();
            std::cout << "\n";
        }

        if (verbose && !debugMode) {
            std::cout << "=== Program Output ===\n";
        }

        // Find and call main function
        uint32_t mainIdx = vm.get_function_index("main");
        if (mainIdx == UINT32_MAX) {
            std::cerr << "Error: No 'main' function found\n";
            delete debugger;
            return false;
        }

        // Call main function
        vm.call_function_by_index(mainIdx, 0);
        vm.run();

        if (verbose && !debugMode) {
            std::cout << "\n=== Execution Complete ===\n";
        }

        delete debugger;
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Runtime Error: " << e.what() << "\n";
        return false;
    }
}

int main(const int argc, char *argv[]) {
    const Config config = parse_args(argc, argv);

    switch (config.mode) {
        case Mode::HELP:
            print_help(argv[0]);
            return 0;

        case Mode::COMPILE: {
            const bool success = compile_source(config.inputFile, config.outputFile,
                                                config.verbose, config.debugMode);
            return success ? 0 : 1;
        }

        case Mode::RUN: {
            const bool success = run_bytecode(config.inputFile, config.verbose, false);
            return success ? 0 : 1;
        }

        case Mode::BUILD_AND_RUN: {
            // Compile first
            std::cout << "Compiling...\n";
            const bool compiled = compile_source(config.inputFile, config.outputFile,
                                                 false, false);
            if (!compiled) {
                return 1;
            }

            std::cout << "Running...\n";
            const bool ran = run_bytecode(config.outputFile, config.verbose, false);

            // Clean up temp bytecode unless --keep flag is set
            if (!config.keepBytecode && config.outputFile == ".temp_droplet.dbc") {
                std::remove(config.outputFile.c_str());
            }

            return ran ? 0 : 1;
        }

        case Mode::DEBUG: {
            std::cout << "Compiling with debug info...\n";
            std::map<uint32_t, FunctionDebugInfo> debugInfo;

            // CREATE MODULE LOADER HERE SO WE CAN ACCESS IT LATER
            ModuleLoader moduleLoader;

            const bool compiled = compile_source(config.inputFile, config.outputFile,
                                                 false, true, &debugInfo, &moduleLoader);
            if (!compiled) {
                return 1;
            }

            // DIAGNOSTIC: Print what debug info was collected
            std::cout << "\n=== DEBUG INFO COLLECTED ===\n";
            std::cout << "Number of functions with debug info: " << debugInfo.size() << "\n";
            for (const auto &[idx, info]: debugInfo) {
                std::cout << "Function [" << idx << "]: " << info.name << "\n";
                std::cout << "  File: " << info.file << "\n";
                std::cout << "  IP->Location mappings: " << info.ipToLocation.size() << "\n";
                for (const auto &[ip, loc]: info.ipToLocation) {
                    std::cout << "    IP " << ip << " -> " << loc.file << ":" << loc.line << "\n";
                }
                std::cout << "  Local variables: " << info.localVariables.size() << "\n";
                for (const auto &[name, slot]: info.localVariables) {
                    std::cout << "    " << name << " = slot " << (int) slot << "\n";
                }
            }
            std::cout << "============================\n\n";

            std::cout << "Starting debugger...\n\n";

            // CREATE VM AND DEBUGGER
            VM vm;
            Loader loader;
            register_native_functions(vm);

            if (!loader.load_dbc_file(config.outputFile, vm)) {
                std::cerr << "Failed to load bytecode file: " << config.outputFile << "\n";
                return 1;
            }

            Debugger debugger(&vm);
            vm.setDebugger(&debugger);

            // Load debug info for all functions
            for (const auto &[idx, info]: debugInfo) {
                debugger.addFunctionDebugInfo(idx, info);
            }

            // Load main source file
            std::vector<std::string> mainLines = readSourceLines(config.inputFile);
            debugger.setSourceFile(config.inputFile, mainLines);

            // LOAD SOURCE FILES FOR ALL IMPORTED MODULES
            loadDebugInfoForModules(&debugger, moduleLoader);

            debugger.start();
            std::cout << "\n";

            // Find and call main function
            uint32_t mainIdx = vm.get_function_index("main");
            if (mainIdx == UINT32_MAX) {
                std::cerr << "Error: No 'main' function found\n";
                return 1;
            }

            // Call main function
            vm.call_function_by_index(mainIdx, 0);
            vm.run();

            if (config.outputFile == ".temp_droplet.dbc") {
                std::remove(config.outputFile.c_str());
            }

            return 0;
        }
    }

    return 0;
}
