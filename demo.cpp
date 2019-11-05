const char SYNOPSIS[] = "Test program that uses clop"; 
const char VERSION[] = "1";

// std
#include <iostream>
#include "clop.h"
using namespace noto;

int main( int argc, const char **argv ) {

	const std::string USAGE = std::string(argv[0]) + " [options] <your name> <your age>"; 
	OptionParser clop;
	clop.hypen_arg_error = false;

	int int1 = 1; 
	clop.add( &int1, "-i", "--int1", "integer option #1" ); 

	int int2 = 2; 
	clop.add( &int2, "-j", "--int2", "integer option #2" ); 

	double double1 = 3.14; 
	// clop.add( &double1, "-r", "--double1", "--double2", "--double3", "double option #1" ); 
	clop.add( &double1, "-r", "--double1", "double option #1" ); 

	char *string1 = NULL; 
	clop.add( &string1, "-s", "string option #1" ); 

	std::string string2 = "", string3="def3", string4="def4";
	clop.add( &string2, "-t", "string option #2" ); 
	// cannot do 
	// clop.add( NULL, "-t", "string option #2 again" );  // (won't even compile)
	// clop.add( &string3, "-t", "--badt2", "string option #2 again" ); 
	// clop.add( &string2, "--t2", "string option #2 another way" ); 

	bool bool1=false, bool2=false, bool3=true, bool4=true;
	clop.add( &bool1, "-a", "bool option #1" ); 
	clop.add( &bool2, "-b", "bool option #2" ); 
	clop.add( &bool3, "-c", "bool option #3" ); 
	clop.add( &bool4, "-d", "bool option #4" ); 
	
	char g = '8';
	clop.add( &g, "-g", "char option");

	bool help = false;
	clop.add( &help, "-h", "--help", "print usage and exit"); 

	std::vector<std::string> args = clop.parse(argc, argv);

	if (help) {
		clop.help(stderr, SYNOPSIS, VERSION, USAGE.c_str(), true);
		return 1;
	}

	std::cout << "program info: " << procinfo(argc,argv,VERSION) << std::endl;

	for (size_t i=0; i<args.size(); i++) {
		std::cout << "argument #" << (i+1) << " is: \"" << args[i] << "\"" << std::endl;
	}
	std::cout << "--- " << args.size() << " arguments." << std::endl;

	std::cout << "integer option #1 is " << "(" << (clop.set(&int1) ? "set" : "default") << "): " << int1 << std::endl;
	std::cout << "integer option #2 is " << "(" << (clop.set(std::string("--int2")) ? "set" : "default") << "): " << int2 << std::endl;
	std::cout << "double option #1 is: " << double1 << std::endl;

	if (string1) { std::cout << "string option #1 is: \"" << string1 << "\"" << std::endl; } else { std::cout << "string option #1 is NULL" << std::endl; }
	std::cout << "string option #2 is: \"" << string2 << "\"" << std::endl; 

	std::cout << "bool option #1 is: " << (bool1?"true":"false") << " (" << (clop.set(&bool1) ? "set" : "not set") << ")" << std::endl;
	std::cout << "bool option #2 is: " << (bool2?"true":"false") << " (" << (clop.set("-b") ? "set" : "not set") << ")" << std::endl;
	std::cout << "bool option #3 is: " << (bool3?"true":"false") << " (" << (clop.set(&bool3) ? "set" : "not set") << ")" << std::endl;
	std::cout << "bool option #4 is: " << (bool4?"true":"false") << " (" << (clop.set(&bool4) ? "set" : "not set") << ")" << std::endl;

	std::cout << "char option is: '" << g << "'" << std::endl;
	
	std::cout << "all done!" << std::endl;

	return 0; 

}
