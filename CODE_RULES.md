follow this shit or you are gay

no breaking lines on if statements and parameters of function defs and impls

dont use size_t or shit, use only __ int types

a function that is called in side the program with constant values that dont change (e.g strlen("this")) then the function doesnt need checks for NULL or any other shit about the params, this is unnecray compution. Use only checks if the function is used in loops and will have values not known at compiletime or are not checked already by previus funcs or in the callers code before the call