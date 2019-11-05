#Authorship

*	2008-xx-xx	noto	original C version 
*	2018-04-14	noto	original C++ version

#Introduction

For a program with GNU-style options of either the form 

*	single hyphen, single character like `-a`, or
*	double hyphen, multi-character like `--alphabet`,
	
this module will parse the command line and set values to variables.  It will
also print a usage description to the screen.

#Usage

```cpp

OptionParser optionparser; 

optionparser.add( pointer-to-variable, flag like "-a", flag like "--alphabet", description of option ); 
... more optionparser.add statements ...

const std::vector<std::string> arguments = optionparser.parse( argc, argv ); 

optionparser.help( FILE* , program synopsis, program version, program usage, bool--include default values in option descriptions? ); 

```

#Example 

```cpp

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

```

#Notes

all variable types given to OptionParser::add must be readable and writable via
`operator>>(istream&, T&)` and `operator<<(ostream&, const T&)` functions.  The
exception is `char*` (and `const char*`) c-style strings, which are handled by
specialized functions.  That means you can use this parser with user-defined
types.

`bool` (and only `bool`) variable types do not require an argument.  If a
variable is `bool`, using its option flag will *toggle* its value.  That means
you can set it to `true` and let the user set it to `false`, if that's more
convenient for its usage in your program.

The user may consolidate multiple Boolean option flags (like `tar` does).  If
there are three Boolean options with flags `-a`, `-b`, and `-c`, the command line
string `-abc` will toggle them all.

the option flag `--` is illegal.  if `--` is on the command line, it is
ignored and all command-line arguments following it are treated as literal
options.  You can turn this behavior off by setting the public bool
`OptionParser` member `interpret_double_hypen` to `false`.

if a command line argument begins with `-` and is not interpreted as an
option (/e.g./, given `-a` but no option `-a` exists), then the program will
throw an exception.  you can change this behavior by setting the public
`OptionParser` `bool` member `hypen_arg_error` to `false`.  This is useful (/e.g./,)
if you expect an argument that could be a negative number.  if
`hypen_arg_error` is false, a command-line string that is not interpretable
as an option will be included in the result of `OptionParser::parse`.
	
Note that if one of the program arguments is `-abcdef` and there are Boolean
options `-a`, `-b`, `-c`, but not all of `-d`, `-e`, and `-f`, then `-abcdef` will
throw an exception (if `OptionParser::hypen_arg_error` is `true`) or it will be
returned as an argument (if `OptionParser::hypen_arg_error` is `false`) and the
variables for `-a`, `-b`, `-c` will not be toggled.

you can ask if an option is set (`bool OptionParser::set`) with its variable
pointer or flag.  this is useful if you only want to take action if the user
assigned a value or if you want to bail out if one option is meant to override
another and both are set.  this might be only slightly cleaner than comparing
the variable's value to its original, but it's easier.

The help message can print compile info stored in a compile time variable named
`CLOP_COMPILE_INFO`.  /I.e./, `g++ -DCLOP_COMPILE_INFO="\"`date`\""
myprogram.cpp clop.cpp` would compile the help function to print "Compile info:
<date at compile time>" when the compiled program prints a `clop_t::help(...)`
message.

This library also provides `procinfo` which takes the argc,argv command
line arguments and returns a string listing the calling information:
program name, program version, `CLOP_COMPILE_INFO` if available (see above),
and the command line arguments.
