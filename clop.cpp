#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <deque>
#include "dau.h"
#include "clop.h" 

namespace noto {

// subroutine for clop_t::parse
bool process_arg(const std::string &arg, std::deque<std::string> &Q, const std::map<const std::string,option_t*> &flagset, std::map<const void *,std::string> &assigned_options);

std::vector<std::string> clop_t::parse(const int argc, const char * const * const argv) {

	this->assigned_options.clear();

	// create mutable set of arguments from argv
	std::deque<std::string> Q; 
	for (int a = 1; a < argc; a++) {
		Q.push_back(argv[a]); 
	}
	std::vector<std::string> result; // return value: list of arguments not interpreted as option flags or values

	// look for option flags in arguments
	while (!Q.empty()) { 

		const std::string arg = Q.front(); 
		Q.pop_front(); 
		if (this->interpret_double_hypen && arg == std::string("--")) { 
			// assume this means all arguments after this are literal/verbatim
			while (!Q.empty()) { 
				result.push_back(Q.front());
				Q.pop_front(); 
			}
		} else if (!process_arg(arg, Q, flagset, assigned_options)) { 
			// regular argument
			if (this->hypen_arg_error && arg[0]=='-') {
				throw DAU() << "illegal option \"" << arg << "\"";
			}
			result.push_back(arg); 
		}
	}
	return result;
}

// sub-subroutine for clop_t::parse
void assign_value(option_t *option, const std::string &flag, const std::string &value, std::map<const void *,std::string> &assigned_options) {

	if (assigned_options.find(option->varptr()) != assigned_options.end()) {
		throw DAU() << "option " << (*option) << " double-initialized with " << assigned_options[ option->varptr() ] << " and " << flag;
	}

	assigned_options[option->varptr()] = flag;

	if (option->requires_value()) {
		option->assign(value); 
	} else {
		option->toggle();
	}
}

// sub-subroutine for clop_t::parse
std::vector<std::string>
expand_arg(const std::string &arg) {

	assert(arg.size() > 0 && arg[0] == '-'); 
	std::vector<std::string> result;
	for (size_t i = 1; i < arg.size(); ++i) {
		result.push_back(std::string("-") + arg.substr(i, 1)); 
	}
	return result;
}

// subroutine for clop_t::parse
bool process_arg(const std::string &arg, std::deque<std::string> &Q, const std::map<const std::string,option_t*> &flagset, std::map<const void *,std::string> &assigned_options)
{
	// if arg is -abc for boolean options -a, -b, -c, reset Q and continue
	if (arg.size() >= 3 && arg[0] == '-' && arg[1] != '-') { 
		std::vector<std::string> expanded_args = expand_arg(arg);
		// all expanded args are boolean option flags
		bool legal = true;
		for (size_t i = 0; i < expanded_args.size(); ++i) { 
			if (flagset.find(expanded_args[i]) == flagset.end()) { 
				legal = false;
				break;
			}
		}

		if (legal) { 
			for (size_t i = expanded_args.size() - 1; i < expanded_args.size(); --i) { 
				Q.push_front(expanded_args[i]);
			}
			return true;
		} 
	}
	// to be changed if this arg is interpreted as a flag (to a legit value)
	std::string value = "";
	// for each flag (and mapped option_t*) we know about...
	std::map<const std::string,option_t*>::const_iterator it;
	for (it = flagset.begin(); it != flagset.end(); ++it) {
		// convenience
		const std::string flag = it->first;
		option_t *option = it->second;		
		if (arg == flag) {		
			// arg is exactly flag, if there's a value, it will be the next argument in line
			if (option->requires_value()) {
				if (Q.empty()) {
					throw DAU() << "option " << (*option) << ", flag " << flag << " requires a value";
				}
				assign_value(option, flag, Q.front(), assigned_options); 
				Q.pop_front();
			} else { 
				assign_value(option, flag, "", assigned_options); 
			}
			return true;
		}
		// enough room for flag and '=' and a value
		// arg starts with flag
		// flag is immediately followed by '='
		else if (option->requires_value() && arg.size() >= (flag.size() + 2)
				 && flag == arg.substr(0, flag.size()) && arg[flag.size()] == '=')
		{ 
			// looks like: -flag=value
			// +1 <=> skip over '=' character
			assign_value(option, flag, arg.substr(flag.size() + 1), assigned_options);
			return true;
		}
	}
	return false;
}

// subroutine for clop_t::help
// print a paragraph, break at white space best you can
//	w1 = chars remaining on line 1
//	w2 = paragraph width
void pbreak(FILE *out, const char *text, unsigned int w1, unsigned int w2, const char *delimeter) {
		
	const char NBSP = '\b'; // TRICK:  Designate one special character to be interpreted as non-breaking space (TODO: but this isn't "advertised" for callers to use)

	for (size_t cur = 0; ;) { // cur (cursor) is current index in character string 
		
		// find break ('\033' is special for highlights, NBSP is special character not to break at)
		
		size_t w = (cur == 0 ? w1 : w2);  // characters left available on line: if this is first line, use w1 (chars remaining), o/w use w2 (full width)
		size_t bp = cur + std::min( w, strlen( text+cur ) ); // set bp (breakpoint) to maximum 
		for (; cur < bp && (text[bp-1] > ' ' || text[bp-1] == '\033' || text[bp-1] == NBSP); bp--) { }  // and work backwards

		// no break, print it all
		if (bp == cur) { bp = cur + std::min( w, strlen( text+cur ) ); }
		
		for (; text[cur] && cur < bp; cur++) {
			if (text[cur] == '\n') { // special case.  We're not going to run out of room.
				bp = cur + 1;
				break;
			}
			fputc(text[cur] == NBSP ? ' ' : text[cur], out); 
		}

		cur = bp; // end-of-loop update, cursor for next loop <- breakpoint
		if (!text[cur]) { break; } // NULL terminator ends the work
		fprintf(out, "%s", delimeter);	

	}
}

// "generic" help message generator
void
clop_t::help(FILE *out, const char *synopsis, const char *version, const char *usage, bool print_default_value) const
{ 
	const bool tty = isatty(fileno(out));
	const std::string HON = (tty) ? "\033[1m" : "";	// highlight on (for section headings)
	const std::string OL_HON = (tty) ? "\033[1m": "";	// option list highlight on (for "-a, --apple")
	const std::string OT_HON = (tty) ? "\033[1m" : "";	// option type highlight on (for "integer")
	const std::string DF_HON = (tty) ? "\033[1m" : "";	// default value highlight on (for "(default: 211)")
	const std::string HOFF = (tty) ? "\033[0m" : "";	// highlight off
	size_t termwidth; // terminal width, if we can get it.
	if (tty) { 
		// we have a tty so ws_col should give us the right terminal width
		struct winsize ws;
		ioctl(fileno(out), TIOCGWINSZ, (void *)(&ws));
		termwidth = ws.ws_col; 
	} else {
		termwidth = 80;
	}
	fputc('\n', out);
	if (synopsis) {
		fprintf(out, "%sSynopsis%s:\n\n    ", HON.c_str(), HOFF.c_str());
		pbreak(out, synopsis, termwidth - 4, termwidth - 4,  "\n    "); 
		fputs("\n\n", out);
	}
	if (version) { 
		fprintf(out, "%sVersion%s:  ", HON.c_str(), HOFF.c_str());
		pbreak(out, version, termwidth - 10, termwidth, "\n"); 
		fputs("\n\n", out);
	}
	#ifdef CLOP_COMPILE_INFO
	fprintf(out, "%sCompile info%s:  ", HON.c_str(), HOFF.c_str());
	pbreak(out, CLOP_COMPILE_INFO, termwidth - 15, termwidth, "\n");
	fputs("\n\n", out);
	#endif
	if (usage) { 
		fprintf(out, "%sUsage%s:  ", HON.c_str(), HOFF.c_str());
		pbreak(out, usage, termwidth - 8, termwidth, "\n"); 
		fputs("\n\n", out);
	}

	// print options
	if (options.size()) { 
		fprintf(out, "%sOptions%s", HON.c_str(), HOFF.c_str()); 
		pbreak(out, ":", termwidth - 7, termwidth, "\n"); 
		fputs("\n\n", out);
	}
	for (size_t i = 0; i < options.size(); ++i) {
		const option_t *option = options[i]; 
		fputs("    ", out); 
		fprintf(out, "%s", OL_HON.c_str());
		for (size_t f = 0; f < option->help.flags.size(); ++f) { 
			fprintf(out, "%s", option->help.flags[f].c_str());
			if (f < option->help.flags.size() - 1) {
				fputs(", ", out);
			}
		}
		fprintf(out, "%s", HOFF.c_str());
		fprintf(out," %s%s%s", OT_HON.c_str(), option->help.metavar.c_str(), HOFF.c_str());
		char opt_delim[] = "\n        ";
		char opt_delen = strlen(opt_delim) - 1;
		fprintf(out, "%s", opt_delim);
		std::ostringstream help_description_oss; 
		help_description_oss << option->help.description; 
		if (option->requires_value() && print_default_value) {
			help_description_oss << DF_HON << " (default: " << option->help.default_value << ")" << HOFF;
		}
		pbreak(out, help_description_oss.str().c_str(), termwidth - opt_delen, termwidth - opt_delen, opt_delim); 
		fputs("\n\n", out);
	}
}

std::string
procinfo(const int argc, const char * const * const argv, const char *version, int arglimit) {

	std::ostringstream oss; 
	if (argc) {
		oss << argv[0];
	} else {
		oss << "procinfo";
	}
	if (version) {
		oss << "; version: " << version;
	}
	#ifdef CLOP_COMPILE_INFO
	oss << "; compile info: " << CLOP_COMPILE_INFO; 
	#endif
	if (arglimit) { 
		oss << "; command:";
		if (argc <= arglimit) {
			// just print all args
			for (int i = 0; i < argc; i++) {
				oss << " " << argv[i];
			}
		} else { 
			int pb = ((int)(1 + 0.6 * arglimit));
			// print first pb arguments
			for (int i = 0; i < pb; ++i) {
				oss << " " << argv[i]; 
			}
			oss << " ... (" << argc << " total arguments, including executable) ...";
			// print last arglimit-pb args
			for (int i = argc - (arglimit - pb); i < argc; i++) {
				oss << " " << argv[i]; 
			}
		}
	}
	return oss.str();
}

std::ostream& operator<<(std::ostream &out, const option_t &option) { 

	// out << "option ";
	for (size_t i = 0; i < option.help.flags.size(); ++i) {
		out << option.help.flags[i]; 
		if (i < (option.help.flags.size() - 1)) {
			out << ",";
		}
	}
	out << ":" << option.help.metavar; 
	if (option.requires_value()) {
		out << "=" << option.help.default_value; 
	}
	return out;
}

} // namespace
