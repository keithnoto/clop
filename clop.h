/* 
  
	Command-Line Option Parser

	2008-xx-xx	noto	original C version 
	2018-04-14	noto	original C++ version

  INTRODUCTION:

	For a program with GNU-style options of either the form 

		(i) single hyphen, single character like "-a", or
		(ii) double hyphen, multi-character like "--alphabet",
	
	this module will parse the command line and set values to variables.  It
	will also print a usage description to the screen.

  USAGE: 

	OptionParser optionparser; 

	optionparser.add( pointer-to-variable, flag like "-a", flag like "--alphabet", description of option ); 
	... more optionparser.add statements ...

	const std::vector<std::string> arguments = optionparser.parse( argc, argv ); 

	optionparser.help( FILE* , program synopsis, program version, program usage, bool--include default values in option descriptions? ); 

  EXAMPLE: 

	const char *SYNOPSIS = "greatest program ever"; 
	const char *VERSION = "1.0"; 
	const std::string USAGE = std::string(argv[0]) + " [options] <your name>"; 
	OptionParser optionparser;
	
	std::string color = "(color not given)"; 
	optionparser.add( &color, "-c", "--color", "your favorite color"); 

	int age = -1;
	optionparser.add( &age, "-a", "your age");  // only "short" option flag given here

	bool help = false; 
	optionparser.add( &help, "-h", "--help", "print help description and exit"); 

	const std::vector<std::string> arguments = optionparser.parse( argc, argv ); 

	if (arguments.size() != 1 || help) {  // expected exactly one argument
		optionparser.print_help(stderr, SYNOPSIS, VERSION, USAGE.c_str()); 
		return 1;
	}

	cout << "your name is: " << args[0] << endl;
	cout << "your favorite color is: " << color << endl;
	if (optionparser.set(&age)) {  // did user change the value of `age' with the "-a" option?
		cout << "your age is: " << age << endl;
	}


  NOTES:

	all variable types given to OptionParser::add must be readable and writable
	via operator>>(istream&, T&) and operator<<(ostream&, const T&) functions.
	The exception is char* (and const char*) c-style strings, which are handled
	by specialized functions.  That means you can use this parser with
	user-defined types.

	bool (and only bool) variable types do not require an argument.  If a
	variable is bool, using its option flag will TOGGLE its value.  That means
	you can set it to true and let the user set it to false, if that's more
	convenient for its usage in your program.

	The user may consolidate multiple boolean option flags (like tar does).  If
	there are three boolean options with flags -a, -b, and -c, the command line
	string -abc will toggle them all.

	the option flag "--" is illegal.  if "--" is on the command line, it is
	ignored and all command-line arguments following it are treated as literal
	options.  You can turn this behavior off by setting the public bool
	OptionParser member interpret_double_hypen to false.

	if a command line argument begins with '-' and is not interpreted as an
	option (e.g., given '-a' but no option '-a' exists), then the program will
	throw an exception.  you can change this behavior by setting the public
	OptionParser bool member hypen_arg_error to false.  This is useful (e.g.,)
	if you expect an argument that could be a negative number.  if
	hypen_arg_error is false, a command-line string that is not interpretable
	as an option will be included in the result of OptionParser::parse.
	
	Note that if one of the program arguments is -abcdef and there are Boolean
	options -a, -b, -c, but not all of -d, -e, and -f, then "-abcdef" will
	throw an exception (if OptionParser::hypen_arg_error is true) or it will be
	returned as an argument (if OptionParser::hypen_arg_error is false) and the
	variables for -a, -b, -c will not be toggled.

	you can ask if an option is set (bool OptionParser::set) with its variable
	pointer or flag.  this is useful if you only want to take action if the
	user assigned a value or if you want to bail out if one option is meant to
	override another and both are set.  this might be only slightly cleaner
	than comparing the variable's value to its original, but it's easier.

	The help message can print compile info stored in a compile time variable
	named CLOP_COMPILE_INFO.  I.e., `g++ -DCLOP_COMPILE_INFO="\"`date`\""
	myprogram.cpp clop.cpp` would compile the help function to print "Compile
	info:  <date at compile time>" when the compiled program prints a
	clop_t::help(...) message.

	This library also provides `procinfo` which takes the argc,argv command
	line arguments and returns a string listing the calling information:
	program name, program version, CLOP_COMPILE_INFO if available (see above),
	and the command line arguments.

*/

#ifndef CLOP_H
#define CLOP_H

#include <cstdio>
#include <cstdarg>
#include <cassert>

#include <vector>
#include <map>
#include <sstream>

#include "dau.h"
namespace noto {

// help information for an option
struct help_t {
	std::vector<std::string> flags; // list of flags
	std::string metavar; 	// argument type
	std::string description; 	// option description
	std::string default_value; // original (i.e, default) value of variable
};

// option base class.  subclasses will be parameterized by variable type 
class option_t { 

  public: 

	const help_t help;  // help info

	option_t(const help_t &h) : help(h) { } // create option with help info

	// functions to be overridden with parameterized subclass
	virtual bool requires_value() const = 0; // does the option require an argument (not true only for booleans)
	virtual void assign(const std::string &value) = 0; // assign a value to the variable
	virtual void toggle() = 0;  // toggle a (boolean) value
	virtual const void* varptr() const = 0; // get a const pointer to the option's variable
	virtual ~option_t() { } 

};
std::ostream& operator<<(std::ostream&,const option_t&);

template <typename T>
class typed_option_t : public option_t {

  public:

	T *variable;  // variable to assign value to

	typed_option_t(T *v, const help_t &h) : option_t(h), variable(v) { }

	void assign(const std::string &value); 
	void toggle(); 
	const void* varptr() const { return (const void*)variable; }
	bool requires_value() const; 
};

// does an option require a value? 
template <typename T> 
bool typed_option_t<T>::requires_value() const { return true; }

template <> 
inline bool typed_option_t<bool>::requires_value() const { return false; } 

// how to assign a value to an option's variable 
template <typename T>
void typed_option_t<T>::assign(const std::string &value) { 
	std::istringstream iss(value); 
	iss >> (*(this->variable)); 
}

// necessary to override certain types

// string variables cannot use instream functions because the value may contain (or be) whitespace
template<>
inline void typed_option_t<std::string>::assign(const std::string &value) { 
	*(this->variable) = value; // std::string copy
}

// char* variables cannot use instream functions because they need to allocate space
template<>
inline void typed_option_t<char*>::assign(const std::string &value) { 
	*(this->variable) = new char[value.size()+1]; 
	strcpy(*(this->variable), value.c_str()); 
}
template<>
inline void typed_option_t<const char*>::assign(const std::string &value) { 
	char *value_cstr = new char[value.size()+1]; 
	strcpy(value_cstr, value.c_str()); 
	*(this->variable) = value_cstr;
}

// bool variables should never be assigned a value (they toggle--see elsewhere)
template<>
inline void typed_option_t<bool*>::assign(const std::string &value) { 
	assert(false); 
	assert(value=="avoid compiler warnings");
}



// how to toggle a (Boolean, non-argument-required) variable's value 
template<typename T> 
void typed_option_t<T>::toggle() { assert(false); } // can't toggle an assigned-value option

template<>
// inline void typed_option_t<bool>::toggle() { *(this->variable) = (!(*(this->variable))); }
inline void typed_option_t<bool>::toggle() { *(this->variable) = ( (this->help.default_value == std::string("0")) ? true : false ); } // change to !default (in case weirdo user calls parse more than once)

/** command line option parser class */
class clop_t {
  
  private: 

	std::vector<option_t*> options; // list of all options, in order (for help message)
	std::map<const std::string, option_t*> flagset; // flags and the variables they set
  	std::map<const void *,std::string> assigned_options; // which have been assigned?

  public:

  	bool hypen_arg_error = true; 
	bool interpret_double_hypen = true;

  	// add option to parser
	// @param variable pointer to variable in question
	template <typename T> 
	void add(T *variable, const char *shortflag, const char *longflag, const char *help); 
	template <typename T> 
	void add(T *variable, const char *flag, const char *help); 

	// parse options and return list of non-option arguments
	std::vector<std::string> parse(const int argc, const char * const * const argv); // all those consts in case user programmer uses them

	// ask if an option is set, given its variable, or one of its flags (return false if no such option)
	bool set(const void *variable) const { return assigned_options.find(variable)!=assigned_options.end(); } 
	bool set(const std::string &flag) const { return flagset.find(flag)!=flagset.end() && assigned_options.find(flagset.at(flag)->varptr())!=assigned_options.end(); }
	bool set(const char *flag) const { return this->set(std::string(flag)); }

	// print standard full usage 
	void help(FILE *fout=stderr, const char *synopsis=NULL, const char *version=NULL, const char *usage=NULL, bool print_default_value=false) const;

	// TODO: useful to return a description of an option?

};
typedef clop_t OptionParser;

inline bool legal_short_flag(const char *flag) { return !flag || (strlen(flag)==2 && flag[0]=='-' && flag[1]!='-'); }
inline bool legal_long_flag(const char *flag) { return !flag || (strlen(flag)>=3 && flag[0]=='-' && flag[1]=='-' && std::string(flag).find('=')==std::string::npos); }

// What is the argument type called, based on it's variable type (pointer)? 
template <typename T>
std::string argument_type(T*) { return "value"; }

inline std::string argument_type(int *) { return "integer"; }
inline std::string argument_type(long *) { return "integer"; }
inline std::string argument_type(unsigned int *) { return "natural"; }
inline std::string argument_type(unsigned long *) { return "natural"; }
inline std::string argument_type(float *) { return "real"; }
inline std::string argument_type(double *) { return "real"; }
inline std::string argument_type(char *) { return "single character"; }
inline std::string argument_type(char **) { return "string"; }
inline std::string argument_type(const char **) { return "string"; }
inline std::string argument_type(std::string *) { return "string"; }
inline std::string argument_type(bool *) { return ""; }

// get current value as string (subroutine for clop_t::add to create help.description)
template <typename T> std::string current_value(T *variable) { std::ostringstream oss; oss << *variable; return oss.str(); }	

// this is the default, force false="0" 
template <> inline std::string current_value(bool *variable) { return (*variable) ? "1" : "0"; }

// for fun, I'll add quotes and things for known types
template <> inline std::string current_value(const char* *cstr) { if (*cstr) { std::ostringstream oss; oss << "\"" << (*cstr) << "\""; return oss.str(); } else { return "NULL"; }}
template <> inline std::string current_value(char* *cstr) { return current_value( (const char **)cstr ); }
template <> inline std::string current_value(std::string *str) { std::ostringstream oss; oss << "\"" << *str << "\""; return oss.str(); }
template <> inline std::string current_value(char *c) { std::ostringstream oss; oss << "'" << *c << "'"; return oss.str(); }

template <typename T> 
void clop_t::add(T *variable, const char *flag, const char *help) {
	if (!flag) { throw DAU() << "creation of option without an indicator flag"; }
	if (legal_short_flag(flag)) { return this->add(variable, flag, NULL, help); }
	if (legal_long_flag(flag)) { return this->add(variable, NULL, flag, help); }
	throw DAU() << "illegal option flag/name: " << flag; 
}
	
template <typename T> 
void clop_t::add(T *variable, const char *shortflag, const char *longflag, const char *help_description) {

	if (!variable) { throw DAU() << "creation of option with NULL variable"; }
	if (!shortflag && !longflag) { throw DAU() << "creation of option without an indicator flag"; }
	if (shortflag && !legal_short_flag(shortflag)) { throw DAU() << "illegal option flag: " << shortflag; }
	if (longflag && !legal_long_flag(longflag)) { throw DAU() << "illegal option name: " << longflag; }

	help_t help; 
	if (shortflag) { help.flags.push_back(std::string(shortflag)); }
	if (longflag) { help.flags.push_back(std::string(longflag)); }
	help.metavar = argument_type(variable);
	help.description = help_description;
	help.default_value = current_value(variable);

	typed_option_t<T> *option = new typed_option_t<T>(variable, help);
	this->options.push_back(option);
	
	for (std::map<const std::string,option_t *>::const_iterator it=flagset.begin(); it!=flagset.end(); it++) { // for each existing flag -> option

		if (it->second->varptr() == variable) { throw DAU() << "option " << (*(it->second)) << " and " << (*option) << " associated with the same variable"; }
		if (shortflag && !strcmp(it->first.c_str(),shortflag)) { throw DAU() << "option flag " << shortflag << " assigned to multiple options: (i) " << (*(it->second)) << ", and (ii) " << (*option); }
		if (longflag && !strcmp(it->first.c_str(), longflag)) { throw DAU() << "option name " << longflag << " assigned to multiple options: (i) " << (*(it->second)) << ", and (ii) " << (*option); }

	} // next previous flag,option

	// add these flags to flagset now
	if (shortflag) { flagset[std::string(shortflag)] = option; }
	if (longflag) { flagset[std::string(longflag)] = option; }
}

// return string with command line and version information
std::string procinfo(const int argc, const char * const * const argv, const char *version=NULL, int arglimit=20); 

}//namespace
#endif
