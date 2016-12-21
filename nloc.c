#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <regex.h>
#include <ftw.h>
#include <getopt.h>

const float version = .01;

#ifndef MAP_POPULATE
// OSX doesn't support MAP_POPULATE
#define MAP_POPULATE 0
#endif

int debug;

// var exclusions []string
// var pipeline chan SourceStat

typedef bool (*verify)(void);
//func(*countContext, string) bool

struct genericLanguage {
  char *name;
  char *suffix;
  char *commentleader;
  char *commenttrailer;
  char *eolcomment;
  verify verifier;
};

struct scriptingLanguage  {
  char *name;
  char *suffix;
  char *hashbang;
  verify verifier;
};

// var scriptingLanguages []scriptingLanguage

struct pascalLike  {
  char *name;
  char *suffix;
  bool bracketcomments;
  verify verifier;
};

// var pascalLikes []pascalLike

#define dt "\"\"\""
#define st "'''"

regex_t  *dtriple, *striple, *dtrailer, *strailer, *dlonely, *slonely, *podheader;

struct fortranLike {
  char *name;
  char *suffix;
  regex_t *comment;
  regex_t *nocomment;
};

// var fortranLikes []fortranLike

/*
var neverInterestingBySuffix map[string]bool
var neverInterestingByBasename map[string]bool

var cHeaderPriority []string
var generated string
*/

const char *neverInterestingByPrefix[] = {"."};
const char *neverInterestingByInfix[] = {".so.", "/."};

const char * ignoreSuffixes[]  = { "~",
				   ".a", ".la", ".o", ".so", ".ko",
				   ".gif", ".jpg", ".jpeg", ".ico", ".xpm", ".xbm", ".bmp",
				   ".ps", ".pdf", ".eps",
				   ".tfm", ".ttf", ".bdf", ".afm",
				   ".fig", ".pic",
				   ".pyc", ".pyo", ".elc",
				   ".1", ".2", ".3", ".4", ".5", ".6", ".7", ".8", ".n", ".man",
				   ".html", ".htm", ".sgml", ".xml",
				  ".txt", ".tex", ".texi",
				  ".po",
				  ".gz", ".bz2", ".Z", ".tgz", ".zip",
				  ".au", ".wav", ".ogg", NULL,
    };

const char generated[] = "automatically generated|generated automatically|generated by|a lexical scanner generated by flex|this is a generated file|generated with the.*utility|do not edit|do not hand-hack";

	// For speed, try to put more common languages and extensions
	// earlier in this list.
	//
	// Verifiers are expensive, so try to put extensions that need them
	// after extensions that don't. But remember that you don't pay the
	// overhead for a verifier unless the extension matches.
	//
	// If you have mutiple entries for an extension, (a) all the entries
	// with verifiers should go first, and (b) there should be at most one
	// entry without a verifier (because any second and later ones will be
	// pre-empted by it).
	//
	// All entries for a given language should be in a contiguous span,
	// otherwise the primtive duplicate director in listLanguages will
	// be foild.

#define nil NULL

// Some forward declarations
bool really_is_lex() { return false; };
bool really_is_objc() { return false; };
bool really_is_pop11() { return false; };
bool really_is_occam(){ return false; };
bool really_is_pascal(){ return false; };
bool really_is_expect(){ return false; };
bool really_is_prolog(){ return false; };
bool really_is_sather(){ return false; };

struct genericLanguage genericLanguages[] = {
		/* C family */
		{"c", ".c", "/*", "*/", "//", nil},
		{"c-header", ".h", "/*", "*/", "//", nil},
		{"c-header", ".hpp", "/*", "*/", "//", nil},
		{"c-header", ".hxx", "/*", "*/", "//", nil},
		{"yacc", ".y", "/*", "*/", "//", nil},
		{"lex", ".l", "/*", "*/", "//", really_is_lex},
		{"c++", ".cpp", "/*", "*/", "//", nil},
		{"c++", ".cxx", "/*", "*/", "//", nil},
		{"c++", ".cc", "/*", "*/", "//", nil},
		{"java", ".java", "/*", "*/", "//", nil},
		{"obj-c", ".m", "/*", "*/", "//", really_is_objc},
		{"c#", ".cs", "/*", "*/", "//", nil},
		{"php", ".php", "/*", "*/", "//", nil},
		{"php3", ".php", "/*", "*/", "//", nil},
		{"php4", ".php", "/*", "*/", "//", nil},
		{"php5", ".php", "/*", "*/", "//", nil},
		{"php6", ".php", "/*", "*/", "//", nil},
		{"php7", ".php", "/*", "*/", "//", nil},
		{"go", ".go", "/*", "*/", "//", nil},
		{"swift", ".swift", "/*", "*/", "//", nil},
		{"sql", ".sql", "/*", "*/", "--", nil},
		{"haskell", ".hs", "{-", "-}", "--", nil},
		{"pl/1", ".pl1", "/*", "*/", "", nil},
		/* everything else */
		{"asm", ".asm", "", "", ";", nil},
		{"asm", ".s", "", "", ";", nil},
		{"asm", ".S", "", "", ";", nil},
		{"ada", ".ada", "", "", "--", nil},
		{"ada", ".adb", "", "", "--", nil},
		{"ada", ".ads", "", "", "--", nil},
		{"ada", ".pad", "", "", "--", nil},	// Oracle Ada preprocessoer.
		{"makefile", ".mk", "", "", "#", nil},
		{"makefile", "Makefile", "", "", "#", nil},
		{"makefile", "makefile", "", "", "#", nil},
		{"makefile", "Imakefile", "", "", "#", nil},
		{"m4", ".m4", "", "", "#", nil},
		{"lisp", ".lisp", "", "", ";", nil},
		{"lisp", ".lsp", "", "", ";", nil},	// XLISP
		{"lisp", ".cl", "", "", ";", nil},	// Common Lisp
		{"lisp", ".l", "", "", ";", nil},
		{"scheme", ".scm", "", "", ";", nil},
		{"elisp", ".el", "", "", ";", nil},	// Emacs Lisp
		{"cobol", ".CBL", "", "", "*", nil},
		{"cobol", ".cbl", "", "", "*", nil},
		{"cobol", ".COB", "", "", "*", nil},
		{"cobol", ".cob", "", "", "*", nil},
		{"eiffel", ".e", "", "", "--", nil},
		{"sather", ".sa", "", "", "--", really_is_sather},
		{"lua", ".lua", "", "", "--", nil},
		{"clu", ".clu", "", "", "%", nil},
		{"rust", ".rs", "", "", "//", nil},
		{"rust", ".rlib", "", "", "//", nil},
		{"erlang", ".erl", "", "", "%", nil},
		//{"turing", ".t", "", "", "%", nil},
		{"d", ".d", "", "", "//", nil},
		{"occam", ".f", "", "", "//", really_is_occam},
		{"prolog", ".pl", "", "", "%", really_is_prolog},
		{"mumps", ".m", "", "", ";", nil},
		{"pop11", ".p", "", "", ";", really_is_pop11},
		// autoconf cruft
		{"autotools", "config.h.in", "/*", "*/", "//", nil},
		{"autotools", "autogen.sh", "", "", "#", nil},
		{"autotools", "configure.in", "", "", "#", nil},
		{"autotools", "Makefile.in", "", "", "#", nil},
		{"autotools", ".am", "", "", "#", nil},
		{"autotools", ".ac", "", "", "#", nil},
		{"autotools", ".mf", "", "", "#", nil},
		// Scons
		{"scons", "SConstruct", "", "", "#", nil},
	};

struct scriptingLanguage	scriptingLanguages[] = {
		{"tcl", ".tcl", "tcl", nil},	/* before sh, because tclsh */
		{"tcl", ".tcl", "wish", nil},
		{"csh", ".csh", "csh", nil},
		{"shell", ".sh", "sh", nil},
		{"ruby", ".rb", "ruby", nil},
		{"awk", ".awk", "awk", nil},
		{"sed", ".sed", "sed", nil},
		{"expect", ".exp", "expect", really_is_expect},
};

struct pascalLike pascalLikes[] = {
		{"pascal", ".pas", true, nil},
		{"pascal", ".p", true, really_is_pascal},
		{"pascal", ".inc", true, really_is_pascal},
		{"modula3", ".i3", false, nil},
		{"modula3", ".m3", false, nil},
		{"modula3", ".ig", false, nil},
		{"modula3", ".mg", false, nil},
		{"ml",      ".ml", false, nil},
		{"mli",      ".ml", false, nil},
		{"mll",      ".ml", false, nil},
		{"mly",      ".ml", false, nil},
		{"oberon",  ".mod", false, nil},
};


bool init_regex() {
	regcomp(dtriple, dt "." dt, 0);
	regcomp(striple, st "." st, 0);
	regcomp(dlonely,"^[ \t]*\"[^\"]+\"",0);
	regcomp(slonely,"^[ \t]*'[^']+'",0);
	regcomp(strailer,".*" st,0);
	regcomp(dtrailer,".*" dt,0);
}

/*

// really_is_occam - returns TRUE if filename contents really are occam.
func really_is_occam(ctx *countContext, path string) bool {
	return has_keywords(ctx, path, "occam", []string{"--", "PROC"})
}

// really_is_lex - returns TRUE if filename contents really are lex.
func really_is_lex(ctx *countContext, path string) bool {
	return has_keywords(ctx, path, "lex", []string{"%{", "%%", "%}"})
}

// really_is_pop11 - returns TRUE if filename contents really are pop11.
func really_is_pop11(ctx *countContext, path string) bool {
	return has_keywords(ctx, path, "pop11", []string{"define", "printf"})
}

// really_is_sather - returns TRUE if filename contents really are sather.
func really_is_sather(ctx *countContext, path string) bool {
	return has_keywords(ctx, path, "sather", []string{"class"})
}

// really_is_prolog - returns TRUE if filename contents really are prolog.
// Without this check, Perl files will be falsely identified.
func really_is_prolog(ctx *countContext, path string) bool {
	ctx.setup(path)
	defer ctx.teardown()

	for ctx.munchline() {
		if bytes.HasPrefix(ctx.line, []byte("#")) {
			return false
		} else if ctx.matchline("\\$[[:alpha]]") {
			return false
		}
	}

	return true
}
*/

/**
 * Using preprocessor macros instead of enums, per request; normally
 * I would use enums, since they obey scoping rules and
 * show up in debuggers.
 */
#define TEXT           0
#define SAW_SLASH      1
#define SAW_STAR       2
#define SINGLE_COMMENT 3
#define MULTI_COMMENT  4

#define TOTAL_STATES   5

#define NO_ACTION      0
#define INC_TOTAL      1
#define INC_SINGLE     2
#define INC_MULTI      4

/**
 * This example assumes 7-bit ASCII, for a total of
 * 128 character encodings.  You'll want to change this
 * to handle other encodings.
 */
#define ENCODINGS    128

typedef unsigned char states;

/**
 * Need a state table to control state transitions and an action
 * table to specify what happens on a transition.  Each table
 * is indexed by the state and the next input character.
 */
states state[TOTAL_STATES][ENCODINGS]; // Since these tables are declared at file scope, they will be initialized to
states action[TOTAL_STATES][ENCODINGS]; // all elements 0, which correspond to the "default" states defined above.

/**
 * Initialize our state table.
 */
void initState( states (*state)[ENCODINGS] )
{
  /**
   * If we're in the TEXT state and see a '/' character, move to the SAW_SLASH
   * state, otherwise stay in the TEXT state
   */
  state[TEXT]['/'] = SAW_SLASH;

  /**
   * If we're in the SAW_SLASH state, we can go one of three ways depending
   * on the next character.
   */
  state[SAW_SLASH]['/'] = SINGLE_COMMENT;
  state[SAW_SLASH]['*'] = MULTI_COMMENT;
  state[SAW_SLASH]['\n'] = TEXT;

  /**
   * For all but a few specific characters, if we're in any one of
   * the SAW_STAR, SINGLE_COMMENT, or MULTI_COMMENT states,
   * we stay in that state.
   */
  for ( size_t i = 0; i < ENCODINGS; i++ )
  {
    state[SAW_STAR][i] = MULTI_COMMENT;
    state[SINGLE_COMMENT][i] = SINGLE_COMMENT;
    state[MULTI_COMMENT][i] = MULTI_COMMENT;
  }

  /**
   * Exceptions to the loop above.
   */
  state[SAW_STAR]['/'] = TEXT;
  state[SAW_STAR]['*'] = SAW_STAR;

  state[SINGLE_COMMENT]['\n'] = TEXT;
  state[MULTI_COMMENT]['*'] = SAW_STAR;
}

/**
 * Initialize our action table
 */
void initAction( states (*action)[ENCODINGS] )
{
  action[TEXT]['\n'] = INC_TOTAL;
  action[SAW_STAR]['/'] = INC_MULTI;
  action[MULTI_COMMENT]['\n'] = INC_MULTI | INC_TOTAL;   // Multiple actions are bitwise-OR'd
  action[SINGLE_COMMENT]['\n'] = INC_SINGLE | INC_TOTAL; // together
  action[SAW_SLASH]['\n'] = INC_TOTAL;
}

/**
 * Scan the input file for comments
 */
void countComments( int fp , size_t *totalLines, size_t *single, size_t *multi )
{
//  *totalLines = *single = *multi = 0;

  int c;
  off_t pos = 0;
  states curState = TEXT, curAction = NO_ACTION;
  struct stat stats;
  fstat(fp, &stats);
  off_t size = stats.st_size;
  if(size < 1) return;
  unsigned char * f = mmap(NULL, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fp, 0);
  if(f == NULL) return;

  while ( pos <  size )
  {
    c = f[pos++];
    if(c > 127) { c = 127; } // FIXME: munch utf-8

    curAction = action[curState][c]; // Read the action before we overwrite the state
    curState = state[curState][c];   // Get the new state (which may be the same as the old state)

    if ( curAction == 0) continue;

    if ( curAction & INC_TOTAL )     // Execute the action.
      (*totalLines)++;

    if ( curAction & INC_SINGLE )
      (*single)++;

    if ( curAction & INC_MULTI )
      (*multi)++;
  }
  munmap(f,size);
}

void reportCocomo(uint32_t sloc) {
	const float TIME_MULT = 2.4;
	const float TIME_EXP = 1.05;
	const float SCHED_MULT = 2.5;
	const float SCHED_EXP = 0.38;
	const float SALARY = 60384;	// From payscale.com, late 2016
	const float OVERHEAD = 2.40;
	printf("Total Physical Source Lines of Code (SLOC)                = %d\n", sloc);
	float person_months = TIME_MULT * pow((double)(sloc)/1000, TIME_EXP);
	printf("Development Effort Estimate, Person-Years (Person-Months) = %2.2f; (%2.2f)\n", person_months / 12, person_months);
	printf(" (Basic COCOMO model, Person-Months = %2.2f * (KSLOC**%2.2f))\n", TIME_MULT, TIME_EXP);
	float sched_months = SCHED_MULT * pow(person_months, SCHED_EXP);
	printf("Schedule Estimate, Years (Months)                         = %2.2f (%2.2f)\n", sched_months/12, sched_months);
	printf(" (Basic COCOMO model, Months = %2.2f * (person-months**%2.2f))\n", SCHED_MULT, SCHED_EXP);
	printf("Estimated Average Number of Developers (Effort/Schedule)  = %2.2f\n", person_months / sched_months);
	printf("Total Estimated Cost to Develop                           = $%d\n", (int)(SALARY * (person_months / 12) * OVERHEAD));
	printf(" (average salary = $%f/year, overhead = %2.2f).\n", SALARY, OVERHEAD);
}

/* https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html#Getopt-Long-Option-Example */
/*
func listLanguages() []string {
	var names []string = []string{"python", "waf", "perl"}
	var lastlang string
	for i := range genericLanguages {
		lang := genericLanguages[i].name
		if lang != lastlang {
			names = append(names, lang)
			lastlang = lang
		}
	}

	for i := range scriptingLanguages {
		lang := scriptingLanguages[i].name
		if lang != lastlang {
			names = append(names, lang)
			lastlang = lang
		}
	}

	for i := range pascalLikes {
		lang := pascalLikes[i].name
		if lang != lastlang {
			names = append(names, lang)
			lastlang = lang
		}
	}

	for i := range fortranLikes {
		lang := fortranLikes[i].name
		if lang != lastlang {
			names = append(names, lang)
			lastlang = lang
		}
	}
	sort.Strings(names)
	return names
	}

func listExtensions() {
	extensions := map[string][]string{
		"python": []string{".py"},
		"waf": []string{"waf"},
		"perl": []string{"pl", "pm"},
	}
	for i := range genericLanguages {
		lang := genericLanguages[i]
		extensions[lang.name] = append(extensions[lang.name], lang.suffix)
	}

	for i := range scriptingLanguages {
		lang := scriptingLanguages[i]
		extensions[lang.name] = append(extensions[lang.name], lang.suffix)
	}

	for i := range pascalLikes {
		lang := pascalLikes[i]
		extensions[lang.name] = append(extensions[lang.name], lang.suffix)
	}

	for i := range fortranLikes {
		lang := fortranLikes[i]
		extensions[lang.name] = append(extensions[lang.name], lang.suffix)
	}
	names := listLanguages()
	for i := range names {
		fmt.Printf("%s: %v\n", names[i], extensions[names[i]])
	}
}

type sortable []countRecord
func (a sortable) Len() int {return len(a)}
func (a sortable) Swap(i int, j int)  { a[i], a[j] = a[j], a[i] }
func (a sortable) Less(i, j int) bool { return -a[i].linecount < -a[j].linecount }

*/

struct opts {
	int individual;
        int unclassified;
	int cocomo;
	int list;
	int extensions;
	int debug;
	int showversion;
	char *excludePtr;
	char *cpuprofile;
};

int flag_parse(char **argv, int argc) {
	struct opts o;
	int c;
	struct option long_options[] =
        {
          {"individual", no_argument,       &o.individual, 'i'},
          {"unclassified",   no_argument,       &o.unclassified, 'u'},
          {"cocomo",     no_argument,       &o.cocomo, 'c'},
          {"list",  no_argument,       &o.list, 'l'},
          {"showversion",  no_argument,       &o.showversion, 'V'},
          {"cpuprofile",  required_argument, &o.cpuprofile, 'C'},
          {"debug",  required_argument, &o.debug, 'd'},
          {0, 0, 0, 0}
        };

      int option_index = 0;

      while(c = getopt_long (argc, argv, "iuclVC:d:",
		      long_options, &option_index) != -1) {
	      switch(c) {
      	      case 'i':
	      case 'u':
	      case 'c':
	      case 'l':
	      case 'V':
	      case 'C':
	      case 'd':
	      default: break;
	      }
      }

      if(o.showversion) {
	      printf("nloc %.1f\n", version);
	      exit(0);
      }

      if(o.list) {
	      printf("%s\n", listLanguages());
	      exit(0);
      }

      if(o.extensions) {
	      listExtensions();
	      exit(0);
      }

      // ...

/*      if (o.cocomo) {
		reportCocomo(totals.linecount)
	}
*/
}


/**
 * Main function.
 */
int main( int argc, char **argv )
{
  size_t totalLines = 0, single = 0, multi = 0;
  /**
   * Input sanity check
   */
  if ( argc < 2 )
  {
    fprintf( stderr, "USAGE: %s <filenames> \n", argv[0] );
    exit( EXIT_FAILURE );
  }
  for(int i = 1; i < argc; i++) {
  /**
   * Open the input file
   */
  int fp = open( argv[i], O_RDONLY);

  if ( fp < 1 )
  {
    fprintf( stderr, "Cannot open file %s\n", argv[1] );
    exit( EXIT_FAILURE );
  }

  /**
   * If input file was successfully opened, initialize our
   * state and action tables.
   */
  initState( state );
  initAction( action );


  /**
   * Do the thing.
   */
  countComments( fp, &totalLines, &single, &multi );
  close( fp );
  }
  printf( "Total lines          : %zu\n", totalLines );
  printf( "Single-comment lines : %zu\n", single );
  printf( "Multi-comment lines  : %zu\n", multi );

  return EXIT_SUCCESS;
}
