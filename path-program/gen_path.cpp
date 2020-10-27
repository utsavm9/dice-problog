/**
 * Usage: g++ gen_path.cpp && ./a.out > grid2.dice
 */

#include <algorithm>
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

typedef unordered_map<int, vector<pair<int, double>>> graph_t;

/**
 * Works like sprintf(), expect it returns a string
 * and takes a string as the first argument.
 * 
 * All the rest of the arguments should be like what sprintf() accepts.
 */
string ssprintf(string formatted, ...) {
    const char *format = formatted.c_str();
    char *buffer = new char[formatted.size() + 100];

    va_list args;
    va_start(args, formatted);
    vsprintf(buffer, format, args);
    va_end(args);

    string sBuffer = buffer;
    delete[] buffer;
    return sBuffer;
}

string construct_tuple(vector<string> &variables) {
    string starting;
    string ending;

    if (variables.size() == 0) {
        return "";
    }

    for (int i = 0; i < variables.size() - 1; ++i) {
        starting += "(" + variables[i] + ", ";
        ending += ")";
    }
    starting += variables[variables.size() - 1];
    return starting + ending;
}

string construct_binop(vector<string> &vars, string op) {
    if (vars.size() <= 0)
        return "";

    string result = vars[0];
    for (int i = 1; i < vars.size(); ++i) {
        result += " " + op + " " + vars[i];
    }

    return result;
}

vector<string> gen_len_1_prog(graph_t graph, int gridSize) {
    vector<string> program;
    vector<string> variables;

    // For each From-Node
    for (int i = 1; i <= gridSize; ++i) {
        for (int j = 1; j <= gridSize; ++j) {
            int fromNode = 10 * i + j;

            // For each To-Node
            for (int k = i; k <= gridSize; ++k) {
                for (int l = 1; l <= gridSize; ++l) {
                    int toNode = 10 * k + l;

                    if (toNode <= fromNode)
                        continue;

                    // Find if an edge exists between those nodes
                    auto &edges = graph[fromNode];
                    auto probEdge = find_if(edges.begin(), edges.end(),
                                            [toNode](pair<int, int> p) { return p.first == toNode; });

                    string probability;
                    if (probEdge == edges.end()) {
                        probability = "false";
                    } else {
                        probability = "flip " + to_string((*probEdge).second);
                    }

                    // Push the appropriate line
                    string varName = ssprintf("path%d%dto%d%d", i, j, k, l);
                    variables.push_back(varName);
                    string progLine = "let " + varName + " = " + probability +" in";
                    program.push_back(progLine);
                }
            }
        }
    }

    program.push_back(construct_tuple(variables));

    return program;
}

vector<string> gen_len_2_prog(int gridSize, string len1Func) {
    vector<string> program;

    // Get all node names in a vector
    vector<int> nodes;
    for (int i = 1; i <= gridSize; ++i) {
        for (int j = 1; j <= gridSize; ++j) {
            nodes.push_back(10 * i + j);
        }
    }

    // Get the length 1 path probabilities
    program.push_back("let len1Paths = " + len1Func + "() in");
    program.push_back("");

    // Unpack the tuple
    // For each From-Node
    int numSnd = 0;
    for (int i = 0; i < nodes.size(); ++i) {
        for (int j = i + 1; j < nodes.size(); ++j) {
            string snds = "";
            for (int k = 0; k < numSnd; ++k) {
                snds += "snd ";
            }
            ++numSnd;

            int numVars = ((nodes.size() - 1) * nodes.size()) / 2;
            string fst = (numVars == numSnd) ? "" : " fst";

            string varName = ssprintf("path%dto%dlen1", nodes[i], nodes[j]);
            program.push_back("let " + varName + " =" + fst + " " + snds + "len1Paths in");
        }
    }
    program.push_back("");

    // Calculate length 2 path probabilities
    vector<string> variables;
    for (int i = 0; i < nodes.size(); ++i) {
        for (int j = i + 1; j < nodes.size(); ++j) {

            vector<string> paths;

            // Length 1 path
            paths.push_back(ssprintf("path%dto%dlen1", nodes[i], nodes[j]));

            // All the len2 paths between nodes[i] and nodes[j]
            for (int k = 0; k < nodes.size(); ++k) {
                if (i == k || k == j)
                    continue;

                int minNode1 = min(nodes[i], nodes[k]);
                int maxNode1 = max(nodes[i], nodes[k]);
                int minNode2 = min(nodes[k], nodes[j]);
                int maxNode2 = max(nodes[k], nodes[j]);

                paths.push_back(ssprintf("(path%dto%dlen1 && path%dto%dlen1)", minNode1, maxNode1, minNode2, maxNode2));
            }

            // Declare new variable
            string exp = construct_binop(paths, "||");
            string var = ssprintf("path%dto%dlen2", nodes[i], nodes[j]);
            variables.push_back(var);
            program.push_back("let " + var + " = " + exp + " in");
        }
    }
    program.push_back("");

    // Pack the tuple
    program.push_back(construct_tuple(variables));
    return program;
}

void wrap_in_function(vector<string> &program, string functionName) {
    for_each(program.begin(), program.end(), [](string &s) { s = "\t" + s; });
    program.insert(program.begin(), "fun " + functionName + "() {");
    program.push_back("}");
    program.push_back("");
}

int main() {

    int gridSize = 2;
    graph_t graph = {
        {11, {{12, 0.1}, {21, 0.1}}},
        {12, {{22, 0.1}}},
        {21, {}},
        {22, {}},
    };

    string len1func = "path_length_equal_1";
    string len2func = "path_length_less_equal_2";
    vector<string> progLen1 = gen_len_1_prog(graph, gridSize);
    wrap_in_function(progLen1, len1func);

    vector<string> progLen2 = gen_len_2_prog(gridSize, len1func);
    wrap_in_function(progLen2, len2func);

    // Calculate function 2
    progLen2.push_back(len2func + "()");

    for_each(progLen1.begin(), progLen1.end(), [](string &s) { cout << s << endl; });
    for_each(progLen2.begin(), progLen2.end(), [](string &s) { cout << s << endl; });

    return 0;
}