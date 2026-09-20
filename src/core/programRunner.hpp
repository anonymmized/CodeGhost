#pragma once

class Runner {
    private:
        CliArgs parsedArguments;
    public:
        void run(int argc, char* argv[]);
};
