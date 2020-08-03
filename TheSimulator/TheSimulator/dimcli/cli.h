// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// cli.h - dimcli
//
// Command line parser toolkit
//
// Instead of trying to figure it out from just this header please, PLEASE,
// take a moment and look at the documentation and examples:
// https://github.com/gknowles/dimcli

#pragma once

#pragma warning(push, 0) // works like a charm for all the warnings

/****************************************************************************
*
*   Configuration
*
***/

// The dimcli_userconfig.h include is defined by the application and may be
// used to configure the standard library before the headers get included.
// This includes macros such as _ITERATOR_DEBUG_LEVEL (Microsoft),
// _LIBCPP_DEBUG (libc++), etc.
#ifdef __has_include
#if __has_include("dimcli_userconfig.h")
#include "dimcli_userconfig.h"
#elif __has_include("cppconf/dimcli_userconfig.h")
#include "cppconf/dimcli_userconfig.h"
#endif
#endif

//---------------------------------------------------------------------------
// Configuration of this installation, these are options that must be the
// same when building the app as when building the library.

// DIMCLI_LIB_DYN_LINK: Forces all libraries that have separate source, to be
// linked as dll's rather than static libraries on Microsoft Windows (this
// macro is used to turn on __declspec(dllimport) modifiers, so that the
// compiler knows which symbols to look for in a dll rather than in a static
// library). Note that there may be some libraries that can only be linked in
// one way (statically or dynamically), in these cases this macro has no
// effect.
//#define DIMCLI_LIB_DYN_LINK

// DIMCLI_LIB_WINAPI_FAMILY_APP: Removes all functions that rely on windows
// WINAPI_FAMILY_DESKTOP mode, such as the console and environment
// variables.
//#define DIMCLI_LIB_WINAPI_FAMILY_APP


//---------------------------------------------------------------------------
// Configuration of the application. These options, if desired, are set by the
// application before including the library headers.

// DIMCLI_LIB_KEEP_MACROS: By default the DIMCLI_LIB_* macros defined
// internally (including in this file) are undef'd so they don't leak out to
// application code. Setting this macro leaves them available for the
// application to use. Also included are other platform specific adjustments,
// such as suppression of specific compiler warnings.

// DIMCLI_LIB_NO_FILESYSTEM: Prevents the <filesystem> header from being
// included and as a side-effect disables support for response files. You can
// try this if you are working with an older compiler with incompatible or
// broken filesystem support.


//===========================================================================
// Internal
//===========================================================================

#if defined(DIMCLI_LIB_SOURCE) && !defined(DIMCLI_LIB_KEEP_MACROS)
#define DIMCLI_LIB_KEEP_MACROS
#endif

#ifdef DIMCLI_LIB_WINAPI_FAMILY_APP
#define DIMCLI_LIB_NO_ENV
#define DIMCLI_LIB_NO_CONSOLE
#endif

#ifdef _MSC_VER
#ifndef DIMCLI_LIB_KEEP_MACROS
#pragma warning(push)
#endif
// attribute 'identifier' is not recognized
#pragma warning(disable : 5030)
#endif

#ifdef DIMCLI_LIB_DYN_LINK
#if defined(_MSC_VER)
// 'identifier': class 'type' needs to have dll-interface to be used
// by clients of class 'type2'
#pragma warning(disable : 4251)
#endif

#if defined(_WIN32)
#ifdef DIMCLI_LIB_SOURCE
#define DIMCLI_LIB_DECL __declspec(dllexport)
#else
#define DIMCLI_LIB_DECL __declspec(dllimport)
#endif
#endif
#else
#define DIMCLI_LIB_DECL
#endif

#if !defined(_CPPUNWIND) && !defined(_HAS_EXCEPTIONS)
#define _HAS_EXCEPTIONS 0
#endif


/****************************************************************************
*
*   Includes
*
***/

#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef DIMCLI_LIB_NO_FILESYSTEM
#if defined(_MSC_VER) && _MSC_VER < 1914
#include <experimental/filesystem>
#define DIMCLI_LIB_FILESYSTEM std::experimental::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::experimental::filesystem::path
#elif defined(__has_include)
#if __has_include(<filesystem>)
#include <filesystem>
#define DIMCLI_LIB_FILESYSTEM std::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::filesystem::path
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#define DIMCLI_LIB_FILESYSTEM std::experimental::filesystem
#define DIMCLI_LIB_FILESYSTEM_PATH std::experimental::filesystem::path
#endif
#endif
#endif


namespace Dim {

// forward declarations
class Cli;

#ifdef DIMCLI_LIB_BUILD_COVERAGE
void assertHandler(char const expr[], unsigned line);

#undef assert
#define assert(expr) \
    (void) ( (!!(expr)) || (Dim::assertHandler(#expr, unsigned(__LINE__)), 0) )
#endif


/****************************************************************************
*
*   Exit codes
*
***/

// These mirror the program exit codes defined in <sysexits.h> on Unix.
enum {
    kExitOk = 0,        // EX_OK
    kExitUsage = 64,    // EX_USAGE     - bad command line
    kExitSoftware = 70, // EX_SOFTWARE  - internal software error
};


/****************************************************************************
*
*   Cli
*
***/

class DIMCLI_LIB_DECL Cli {
public:
    struct Config;

    class OptBase;
    template <typename A, typename T> class OptShim;
    template <typename T> class Opt;
    template <typename T> class OptVec;
    struct OptIndex;

    struct ArgMatch;
    template <typename T> struct Value;
    template <typename T> struct ValueVec;

public:
    // Creates a handle to the shared command line configuration, this
    // indirection allows options to be statically registered from multiple
    // source files.
    Cli();
    Cli(Cli const & from);
    Cli & operator=(Cli const & from);
    Cli & operator=(Cli && from) noexcept;

    //-----------------------------------------------------------------------
    // CONFIGURATION

    template <typename T>
    Opt<T> & opt(std::string const & keys, T const & def = {});

    template <typename T>
    Opt<T> & opt(T * value, std::string const & keys);

    template <typename T, typename U, typename =
        typename std::enable_if<std::is_convertible<U, T>::value>::type
    >
    Opt<T> & opt(T * value, std::string const & keys, U const & def);

    template <typename T>
    Opt<T> & opt(Opt<T> & value, std::string const & keys);

    template <typename T, typename U, typename =
        typename std::enable_if<std::is_convertible<U, T>::value>::type
    >
    Opt<T> & opt(Opt<T> & value, std::string const & keys, U const & def);

    template <typename T>
    OptVec<T> & optVec(std::string const & keys, int nargs = -1);

    template <typename T>
    OptVec<T> & optVec(
        std::vector<T> * values,
        std::string const & keys,
        int nargs = -1
    );

    template <typename T>
    OptVec<T> & optVec(
        OptVec<T> & values,
        std::string const & keys,
        int nargs = -1
    );

    // Add -y, --yes option that exits early when false and has an "are you
    // sure?" style prompt when it's not present.
    Opt<bool> & confirmOpt(std::string const & prompt = {});

    // Get reference to internal help option, can be used to change the
    // description, option group, etc. To completely replace it, add another
    // option that responds to --help.
    Opt<bool> & helpOpt();

    // Add --password option and prompts for a password if it's not given
    // on the command line. If confirm is true and it's not on the command
    // line you have to enter it twice.
    Opt<std::string> & passwordOpt(bool confirm = false);

    // Add --version option that shows "${progName.filename()} version ${ver}"
    // and exits. An empty progName defaults to argv[0].
    Opt<bool> & versionOpt(
        std::string const & ver,
        std::string const & progName = {}
    );

    //-----------------------------------------------------------------------
    // A group collects options into sections in the help text. Options are
    // always added to a group, either the default group of the cli (or of the
    // selected command), or an explicitly created one.

    // Changes config context to point at the selected option group of the
    // current command, that you can then start stuffing things into.
    Cli & group(std::string const & name);

    // Heading title to display, defaults to group name. If empty there will
    // be a single blank line separating this group from the previous one.
    Cli & title(std::string const & val);

    // Option groups are sorted by key, defaults to group name.
    Cli & sortKey(std::string const & key);

    std::string const & group() const { return m_group; }
    std::string const & title() const;
    std::string const & sortKey() const;

    //-----------------------------------------------------------------------
    // Changes config context to reference the options of the selected
    // command. Use "" to specify the top level context. If a new command is
    // selected it will be created in the command group of the current context.
    Cli & command(std::string const & name, std::string const & group = {});

    // Function signature of actions that are tied to commands.
    using ActionFn = bool(Cli & cli);

    // Action that should be taken when the currently selected command is run.
    // Actions are executed when cli.exec() is called by the application. The
    // command's action function should:
    //  - do something useful
    //  - return false on errors (and use fail() to set exitCode, et al)
    Cli & action(std::function<ActionFn> fn);

    // Arbitrary text can be added to the generated help for each command,
    // this text can come before the usage (header), between the usage and
    // the arguments / options (desc), or after the options (footer). Use
    // line breaks for semantics, let the automatic line wrapping take care
    // of the rest.
    //
    // Unless individually overridden commands default to using the header and
    // footer (but not the desc) specified at the top level.
    Cli & header(std::string const & val);
    Cli & desc(std::string const & val);
    Cli & footer(std::string const & val);

    std::string const & command() const { return m_command; }
    std::string const & header() const;
    std::string const & desc() const;
    std::string const & footer() const;

    // Add "help" command that shows the help text for other commands. Allows
    // users to run "prog help command" instead of the slightly more awkward
    // "prog command --help".
    Cli & helpCmd();

    // Adds before action that replaces the empty command line with "--help".
    Cli & helpNoArgs();

    //-----------------------------------------------------------------------
    // A command group collects commands into sections in the help text, in the
    // same way that option groups do with options.

    // Changes the command group of the current command. Because new commands
    // start out in the same group as the current command, it can be convenient
    // to create all the commands of one group before moving to the next.
    //
    // Setting the command group at the top level (the "" command) only serves
    // to set the initial command group for new commands created while in the
    // top level context.
    Cli & cmdGroup(std::string const & name);

    // Heading title to display, defaults to group name. If empty there will be
    // a single blank line separating this group from the previous one.
    Cli & cmdTitle(std::string const & val);

    // Command groups are sorted by key, defaults to group name.
    Cli & cmdSortKey(std::string const & key);

    std::string const & cmdGroup() const;
    std::string const & cmdTitle() const;
    std::string const & cmdSortKey() const;

    //-----------------------------------------------------------------------
    // Enabled by default, response file expansion replaces arguments of the
    // form "@file" with the contents of the file.
    void responseFiles(bool enable = true);

#if !defined(DIMCLI_LIB_NO_ENV)
    // Environment variable to get initial options from. Defaults to the empty
    // string, but when set the content of the named variable is parsed into
    // args which are then inserted into the argument list right after arg0.
    void envOpts(std::string const & envVar);
#endif

    // Function signature of actions that run before options are populated.
    using BeforeFn = bool(Cli & cli, std::vector<std::string> & args);

    // Actions taken after environment variable and response file expansion
    // but before any individual arguments are parsed. The before action
    // function should:
    //  - inspect and possibly modify the raw arguments coming in
    //  - return false if parsing should stop, via badUsage() for errors
    Cli & before(std::function<BeforeFn> fn);

    // Changes the streams used for prompting, printing help messages, etc.
    // Mainly intended for testing. Setting to null restores the defaults
    // which are cin and cout respectively.
    Cli & iostreams(std::istream * in, std::ostream * out);
    std::istream & conin();
    std::ostream & conout();

    //-----------------------------------------------------------------------
    // RENDERING HELP TEXT

    // printHelp & printUsage return the current exitCode()
    int printHelp(
        std::ostream & os,
        std::string const & progName = {},
        std::string const & cmd = {}
    );
    int printUsage(
        std::ostream & os,
        std::string const & progName = {},
        std::string const & cmd = {}
    );
    // Same as printUsage(), except individually lists all non-default options
    // instead of the [OPTIONS] catchall.
    int printUsageEx(
        std::ostream & os,
        std::string const & progName = {},
        std::string const & cmd = {}
    );

    void printPositionals(
        std::ostream & os,
        std::string const & cmd = {}
    );
    void printOptions(std::ostream & os, std::string const & cmd = {});
    void printCommands(std::ostream & os);

    // If exitCode() is not EX_OK, prints the errMsg and errDetail (if
    // present), otherwise does nothing. Returns exitCode(). Only makes sense
    // after parsing has completed.
    int printError(std::ostream & os);

    //-----------------------------------------------------------------------
    // PARSING

    // Parse the command line, populate the options, and set the error and
    // other miscellaneous state. Returns true if processing should continue.
    //
    // The ostream& argument is only used to print the error message, if any,
    // via printError(). Error information can also be extracted after parse()
    // completes, see errMsg() and friends.
    [[nodiscard]] bool parse(size_t argc, char * argv[]);
    [[nodiscard]] bool parse(std::ostream & oerr, size_t argc, char * argv[]);

    // "args" is non-const so response files can be expanded in place.
    [[nodiscard]] bool parse(std::vector<std::string> & args);
    [[nodiscard]] bool parse(
        std::ostream & oerr,
        std::vector<std::string> & args
    );

    // Sets all options to their defaults, called internally when parsing
    // starts.
    Cli & resetValues();

    // Parse cmdline into vector of args, using the default conventions
    // (Gnu or Windows) of the platform.
    static std::vector<std::string> toArgv(std::string const & cmdline);
    // Copy array of pointers into vector of args.
    static std::vector<std::string> toArgv(size_t argc, char * argv[]);
    // Copy array of wchar_t pointers into vector of UTF-8 encoded args.
    static std::vector<std::string> toArgv(size_t argc, wchar_t * argv[]);

    // Create vector of pointers suitable for use with argc/argv APIs, has a
    // trailing null that is not included in size(). The return values point
    // into the source string vector and are only valid until that vector is
    // resized or destroyed.
    static std::vector<char const *> toPtrArgv(
        std::vector<std::string> const & args
    );

    // Parse according to glib conventions, based on the UNIX98 shell spec.
    static std::vector<std::string> toGlibArgv(std::string const & cmdline);
    // Parse using GNU conventions, same rules as buildargv()
    static std::vector<std::string> toGnuArgv(std::string const & cmdline);
    // Parse using Windows rules
    static std::vector<std::string> toWindowsArgv(std::string const & cmdline);

    // Join args into a single command line, escaping as needed, that parses
    // back into those same args. Uses the default conventions (Gnu or Windows)
    // of the platform.
    static std::string toCmdline(std::vector<std::string> const & args);
    // Join array of pointers into command line, escaping as needed.
    static std::string toCmdline(size_t argc, char * argv[]);
    static std::string toCmdline(size_t argc, char const * argv[]);

    // Join according to glib conventions, based on the UNIX98 shell spec.
    static std::string toGlibCmdline(size_t argc, char * argv[]);
    // Join using GNU conventions, same rules as buildargv()
    static std::string toGnuCmdline(size_t argc, char * argv[]);
    // Join using Windows rules
    static std::string toWindowsCmdline(size_t argc, char * argv[]);

    //-----------------------------------------------------------------------
    // Support functions for use from parsing actions

    // Intended for use in return statements from action callbacks. Sets
    // exitCode (to EX_USAGE), errMsg, and errDetail. errMsg is set to
    // "<prefix>: <value>" or "<prefix>" if value.empty(), with an additional
    // leading prefix of "Command: '<command>'" for subcommands. Always
    // returns false.
    bool badUsage(
        std::string const & prefix,
        std::string const & value = {},
        std::string const & detail = {}
    );

    // Calls badUsage(prefix, value, detail) with prefix set to:
    //  "Invalid '" + opt.from() + "' value"
    bool badUsage(
        OptBase const & opt,
        std::string const & value,
        std::string const & detail = {}
    );

    // True if low <= rawVal <= high, otherwise calls badUsage with "Out of
    // range" message and the low and high in the detail.
    template <typename A, typename T>
    bool badRange(
        A & opt,
        std::string const & val,
        T const & low,
        T const & high
    );

    // Used to populate an option with an arbitrary input string through the
    // standard parsing logic. Since it causes the parse and check actions to
    // be called care must be taken to avoid infinite recursion if used from
    // those actions.
    [[nodiscard]] bool parseValue(
        OptBase & out,
        std::string const & name,
        size_t pos,
        char const src[]
    );

    // fUnit* flags modify how unit suffixes are interpreted by opt.siUnits(),
    // opt.timeUnits(), and opt.anyUnits().
    enum {
        // When this flag is set the symbol must be specified and it's
        // absence in option values makes them invalid arguments.
        fUnitRequire = 1,

        // Makes units case insensitive. For siUnits() unit prefixes are
        // also case insensitive, but fractional prefixes (milli, micro, etc)
        // are prohibited because they would clash with mega and peta.
        fUnitInsensitive = 2,

        // Makes k,M,G,T,P factors of 1024, same as ki,Mi,Gi,Ti,Pi. Factional
        // unit prefixes (milli, micro, etc) are prohibited.
        fUnitBinaryPrefix = 4,
    };

    // Prompt sends a prompt message to cout and read a response from cin
    // (unless cli.iostreams() changed the streams to use), the response is
    // then passed to cli.parseValue() to set the value and run any actions.
    enum {
        fPromptHide = 1,      // Hide user input as they type
        fPromptConfirm = 2,   // Make the user enter it twice
        fPromptNoDefault = 4, // Don't include default value in prompt
    };
    [[nodiscard]] bool prompt(
        OptBase & opt,
        std::string const & msg,
        int flags // fPrompt* flags
    );

    //-----------------------------------------------------------------------
    // AFTER PARSING

    int exitCode() const;
    std::string const & errMsg() const;

    // Additional information that may help the user correct their mistake,
    // may be empty.
    std::string const & errDetail() const;

    // Program name received in argv[0]
    std::string const & progName() const;

    // Command to run, as selected by argv, empty string if there are no
    // commands defined or none were selected.
    std::string const & commandMatched() const;

    // Executes the action of the selected command; returns true if it worked.
    // On failure it's expected to have set exitCode, errMsg, and optionally
    // errDetail via fail(). If no command was selected it runs the action of
    // the empty "" command, which defaults to failing with "No command given."
    // but can be set via cli.action() just like any other command.
    [[nodiscard]] bool exec();
    [[nodiscard]] bool exec(std::ostream & oerr);

    // Helpers to parse and, if successful, execute.
    [[nodiscard]] bool exec(size_t argc, char * argv[]);
    [[nodiscard]] bool exec(std::ostream & oerr, size_t argc, char * argv[]);
    [[nodiscard]] bool exec(std::vector<std::string> & args);
    [[nodiscard]] bool exec(
        std::ostream & oerr,
        std::vector<std::string> & args
    );

    // Sets exitCode(), errMsg(), and errDetail(), intended to be called from
    // command actions, parsing related failures should use badUsage().
    bool fail(
        int code,
        std::string const & msg,
        std::string const & detail = {}
    );

    // Returns true if the named command has been defined, used by the help
    // command implementation. Not reliable before cli.parse() has been called
    // and had a chance to update the internal data structures.
    bool commandExists(std::string const & name) const;

protected:
    Cli(std::shared_ptr<Config> cfg);

private:
    static void consoleEnableEcho(bool enable = true);

    static bool defParseAction(
        Cli & cli,
        OptBase & opt,
        std::string const & val
    );
    static bool requireAction(
        Cli & cli,
        OptBase & opt,
        std::string const & val
    );

    static std::vector<std::pair<std::string, double>> siUnitMapping(
        std::string const & symbol,
        int flags
    );

    void addOpt(std::unique_ptr<OptBase> opt);
    template <typename A> A & addOpt(std::unique_ptr<A> ptr);

    template <typename A, typename V, typename T>
    std::shared_ptr<V> getProxy(T * ptr);

    // Find an option (from any subcommand) that targets the value.
    OptBase * findOpt(void const * value);

    std::string descStr(Cli::OptBase const & opt) const;
    int writeUsageImpl(
        std::ostream & os,
        std::string const & arg0,
        std::string const & cmd,
        bool expandedOptions
    );

    std::shared_ptr<Config> m_cfg;
    std::string m_group;
    std::string m_command;
};

//===========================================================================
template <typename T, typename U, typename>
Cli::Opt<T> & Cli::opt(
    T * value,
    std::string const & keys,
    U const & def
) {
    auto proxy = getProxy<Opt<T>, Value<T>>(value);
    auto ptr = std::make_unique<Opt<T>>(proxy, keys);
    ptr->defaultValue(def);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T>
Cli::Opt<T> & Cli::opt(T * value, std::string const & keys) {
    return opt(value, keys, T{});
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(
    std::vector<T> * values,
    std::string const & keys,
    int nargs
) {
    auto proxy = getProxy<OptVec<T>, ValueVec<T>>(values);
    auto ptr = std::make_unique<OptVec<T>>(proxy, keys, nargs);
    return addOpt(std::move(ptr));
}

//===========================================================================
template <typename T, typename U, typename>
Cli::Opt<T> & Cli::opt(
    Opt<T> & alias,
    std::string const & keys,
   U  const & def
) {
    return opt(&*alias, keys, def);
}

//===========================================================================
template <typename T>
Cli::Opt<T> & Cli::opt(Opt<T> & alias, std::string const & keys) {
    return opt(&*alias, keys, T{});
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(
    OptVec<T> & alias,
    std::string const & keys,
    int nargs
) {
    return optVec(&*alias, keys, nargs);
}

//===========================================================================
template <typename T>
Cli::Opt<T> & Cli::opt(std::string const & keys, T const & def) {
    return opt<T>(nullptr, keys, def);
}

//===========================================================================
template <typename T>
Cli::OptVec<T> & Cli::optVec(std::string const & keys, int nargs) {
    return optVec<T>(nullptr, keys, nargs);
}

//===========================================================================
template <typename A>
A & Cli::addOpt(std::unique_ptr<A> ptr) {
    auto & opt = *ptr;
    opt.parse(&Cli::defParseAction).command(command()).group(group());
    addOpt(std::unique_ptr<OptBase>(ptr.release()));
    return opt;
}

//===========================================================================
template <typename A, typename V, typename T>
std::shared_ptr<V> Cli::getProxy(T * ptr) {
    if (OptBase * opt = findOpt(ptr)) {
        auto dopt = static_cast<A *>(opt);
        return dopt->m_proxy;
    }

    // Since there was no existing proxy to the raw value, create one.
    return std::make_shared<V>(ptr);
}

//===========================================================================
template <typename A, typename T>
bool Cli::badRange(
    A & opt,
    std::string const & val,
    T const & low,
    T const & high
) {
    auto prefix = "Out of range '" + opt.from() + "' value";
    std::string detail, lstr, hstr;
    if (opt.toString(lstr, low) && opt.toString(hstr, high))
        detail = "Must be between '" + lstr + "' and '" + hstr + "'.";
    return badUsage(prefix, val, detail);
}


/****************************************************************************
*
*   CliLocal
*
*   Stand-alone cli instance independent of the shared configuration. Mainly
*   for testing.
*
***/

class DIMCLI_LIB_DECL CliLocal : public Cli {
public:
    CliLocal();
};


/****************************************************************************
*
*   Cli::OptBase
*
*   Common base class for all options, has no information about the derived
*   options value type, and handles all services required by the parser.
*
***/

class DIMCLI_LIB_DECL Cli::OptBase {
public:
    struct ChoiceDesc {
        std::string desc;
        std::string sortKey;
        size_t pos{};
        bool def{};
    };

public:
    OptBase(std::string const & keys, bool boolean);
    virtual ~OptBase() {}

    //-----------------------------------------------------------------------
    // CONFIGURATION

    // Change the locale used when parsing values via iostream. Defaults to
    // the user's preferred locale (aka locale("")) for arithmetic types and
    // the "C" locale for everything else.
    std::locale imbue(std::locale const & loc);

    //-----------------------------------------------------------------------
    // QUERIES

    // True if the value was populated from the command line, whether the
    // resulting value is the same as the default is immaterial.
    explicit operator bool() const { return assigned(); }

    // Name of the last argument to populated the value, or an empty string if
    // it wasn't populated. For vectors, it's what populated the last value.
    virtual std::string const & from() const = 0;

    // Absolute position in argv[] of last the argument that populated the
    // value. For vectors, it refers to where the value on the back came from.
    // If pos() is 0 the value wasn't populated from the command line or
    // wasn't populated at all, check from() to tell the difference.
    //
    // It's possible for a value to come from prompt() or some other action
    // (which should set the position to 0) instead of the command args.
    virtual int pos() const = 0;

    // Number of values, non-vectors are always 1.
    virtual size_t size() const = 0;

    // Defaults to use when populating the option from an action that's not
    // tied to a command line argument.
    std::string const & defaultFrom() const { return m_fromName; }
    std::string defaultPrompt() const;

    // Command and group this option belongs to.
    std::string const & command() const { return m_command; }
    std::string const & group() const { return m_group; }

    //-----------------------------------------------------------------------
    // UPDATE VALUE

    // Set option to its default value.
    virtual void reset() = 0;

    // Parse the string into the value, return false on error.
    [[nodiscard]] virtual bool parseValue(std::string const & value) = 0;

    // Store the unspecified value. Used when an option with an optional value
    // is specified without it. The unspecified value is normally equal to
    // the default value, but can be overridden.
    virtual void unspecifiedValue() = 0;

    //-----------------------------------------------------------------------
    // HELPERS

    template <typename T>
    [[nodiscard]] bool fromString(T & out, std::string const & value) const;
    template <typename T>
    [[nodiscard]] bool toString(std::string & out, T const & src) const;

    // Friendly name for type, used in help text.
    template <typename T>
    std::string toValueDesc() const;

protected:
    virtual bool defaultValueToString(std::string & out) const = 0;
    virtual std::string defaultValueDesc() const = 0;

    virtual bool doParseAction(Cli & cli, std::string const & value) = 0;
    virtual bool doCheckAction(Cli & cli, std::string const & value) = 0;
    virtual bool doAfterActions(Cli & cli) = 0;
    virtual bool assign(std::string const & name, size_t pos) = 0;
    virtual bool assigned() const = 0;

    // True for flags (bool on command line) that default to true.
    virtual bool inverted() const = 0;

    // Allows the type unaware layer to determine if a new option is pointing
    // at the same value as an existing option -- with RTTI disabled
    virtual bool sameValue(void const * value) const = 0;

    void setNameIfEmpty(std::string const & name);

    bool withUnits(
        long double & out,
        std::string const & val,
        std::unordered_map<std::string, long double> const & units,
        int flags
    ) const;

    std::string m_command;
    std::string m_group;

    bool m_visible{true};
    std::string m_desc;
    std::string m_valueDesc;

    // empty() to use default, size 1 and *data == '\0' to suppress
    std::string m_defaultDesc;

    std::unordered_map<std::string, ChoiceDesc> m_choiceDescs;

    // Whether this option has one value or a vector of values.
    bool m_vector{};
    // Maximum allowed values, only for vectors (-1 for unlimited).
    int m_nargs{1};

    // Whether the value is a bool on the command line (no separate value).
    bool m_bool{};

    bool m_flagValue{};
    bool m_flagDefault{};

private:
    friend class Cli;

    template <typename T>
    auto fromString_impl(T & out, std::string const & src, int, int) const
        -> decltype(out = src, bool());
    template <typename T>
    auto fromString_impl(T & out, std::string const & src, int, long) const
        -> decltype(std::declval<std::istream &>() >> out, bool());
    template <typename T>
    bool fromString_impl(T & out, std::string const & src, long, long) const;

    template <typename T>
    auto toString_impl(std::string & out, T const & src, int) const
        -> decltype(std::declval<std::ostream &>() << src, bool());
    template <typename T>
    bool toString_impl(std::string & out, T const & src, long) const;

    std::string m_names;
    std::string m_fromName;
    mutable std::stringstream m_interpreter;
};

//===========================================================================
// fromString - converts from string to T
//===========================================================================
template <typename T>
[[nodiscard]] bool Cli::OptBase::fromString(
    T & out,
    std::string const & src
) const {
    // Versions of fromString_impl taking ints as extra parameters are
    // preferred by the compiler (better conversion from 0), if they don't
    // exist for T (because no out=src assignment operator exists) then the
    // versions taking longs are considered.
    return fromString_impl(out, src, 0, 0);
}

//===========================================================================
template <typename T>
auto Cli::OptBase::fromString_impl(
    T & out,
    std::string const & src,
    int,
    int
) const
    -> decltype(out = src, bool())
{
    out = src;
    return true;
}

//===========================================================================
template <typename T>
auto Cli::OptBase::fromString_impl(
    T & out,
    std::string const & src,
    int,
    long
) const
    -> decltype(std::declval<std::istream &>() >> out, bool())
{
    m_interpreter.clear();
    m_interpreter.str(src);
    if (!(m_interpreter >> out) || !(m_interpreter >> std::ws).eof()) {
        out = {};
        return false;
    }
    return true;
}

//===========================================================================
template <typename T>
bool Cli::OptBase::fromString_impl(
    T &, // out
    std::string const &, // src
    long,
    long
) const {
    // In order to parse an argument one of the following must exist:
    //  - assignment operator for std::string to T
    //  - std::istream extraction operator for T
    //  - specialization of Cli::OptBase::fromString template for T, such as:
    //      template<> bool Cli::OptBase::fromString<MyType>::(
    //          MyType & out, std::string const & src) const { ... }
    //  - parse action attached to the Opt<T> instance that doesn't call
    //    opt.parseValue(), such as opt.choice().
    assert(!"no assignment from string or stream extraction operator");
    return false;
}

//===========================================================================
// toString - converts to string from T, sets to empty string and returns
// false if conversion fails or no conversion available.
//===========================================================================
template <typename T>
[[nodiscard]] bool Cli::OptBase::toString(
    std::string & out,
    T const & src
) const {
    return toString_impl(out, src, 0);
}

//===========================================================================
template <typename T>
[[nodiscard]] auto Cli::OptBase::toString_impl(
    std::string & out,
    T const & src,
    int
) const
    -> decltype(std::declval<std::ostream &>() << src, bool())
{
    m_interpreter.clear();
    m_interpreter.str("");
    if (!(m_interpreter << src)) {
        out.clear();
        return false;
    }
    out = m_interpreter.str();
    return true;
}

//===========================================================================
template <typename T>
bool Cli::OptBase::toString_impl(std::string & out, T const &, long) const {
    out.clear();
    return false;
}

//===========================================================================
// toValueDesc - descriptive name for values of type T
//===========================================================================
template <typename T>
std::string Cli::OptBase::toValueDesc() const {
    if (std::is_integral<T>::value) {
        return "NUM";
    } else if (std::is_floating_point<T>::value) {
        return "FLOAT";
    } else if (std::is_convertible<T, std::string>::value) {
        return "STRING";
    } else {
        return "VALUE";
    }
}

#ifdef DIMCLI_LIB_FILESYSTEM
//===========================================================================
template <>
inline
std::string Cli::OptBase::toValueDesc<DIMCLI_LIB_FILESYSTEM_PATH>() const {
    return "FILE";
}
#endif


/****************************************************************************
*
*   Cli::OptShim
*
*   Common base for the more derived simple and vector option types. Has
*   knowledge of the value type but no knowledge about it.
*
***/

template <typename A, typename T>
class Cli::OptShim : public OptBase {
public:
    OptShim(std::string const & keys, bool boolean);
    OptShim(OptShim const &) = delete;
    OptShim & operator=(OptShim const &) = delete;

    //-----------------------------------------------------------------------
    // CONFIGURATION

    // Set subcommand for which this is an option.
    A & command(std::string const & val);

    // Set group under which this argument will show up in the help text.
    A & group(std::string const & val);

    // Controls whether or not the option appears in help pages.
    A & show(bool visible = true);

    // Set description to associate with the argument in help text.
    A & desc(std::string const & val);

    // Set name of meta-variable in help text. For example, in "--count NUM"
    // this is used to change "NUM" to something else.
    A & valueDesc(std::string const & val);

    // Set text to appear in the default clause of this options the help text.
    // Can change the "0" in "(default: 0)" to something else, or use an empty
    // string to suppress the entire clause.
    A & defaultDesc(std::string const & val);

    // Allows the default to be changed after the opt has been created.
    A & defaultValue(T const & val);

    // The implicit value is used for arguments with optional values when
    // the argument was specified in the command line without a value.
    A & implicitValue(T const & val);

    // Turns the argument into a feature switch, there are normally multiple
    // switches pointed at a single external value, one of which should be
    // flagged as the default. If none (or many) are set marked as the default
    // one will be chosen for you.
    A & flagValue(bool isDefault = false);

    // Adds a choice, when choices have been added only values that match one
    // of the choices are allowed. Useful for things like enums where there is
    // a controlled set of possible values.
    //
    // In help text choices are sorted first by sortKey and then by the order
    // they were added.
    A & choice(
        T const & val,
        std::string const & key,
        std::string const & desc = {},
        std::string const & sortKey = {}
    );

    // Normalizes the value by removing the symbol and if SI unit prefixes
    // (such as u, m, k, M, G, ki, Mi, and Gi) are present, multiplying the
    // value by the corresponding factor and removing it as well. The
    // Cli::fUnit* flags can be used to: base k and friends on 1024, require
    // the symbol, and/or be case-insensitive. Fractional prefixes such as
    // m (milli) work best with floating point options.
    A & siUnits(
        std::string const & symbol = {},
        int flags = 0 // Cli::fUnit* flags
    );

    // Adjusts the value to seconds when time units are present: removing the
    // units and multiplying by the required factor. Recognized units are:
    // y (365d), w (7d), d, h, min, m (minute), s, ms, us, ns. Some behavior
    // can be adjusted with Cli::fUnit* flags.
    A & timeUnits(int flags = 0);

    // Given a series of unit names and factors, incoming values have trailing
    // unit names removed and are multiplied by the factor. For example,
    // given {{"in",1},{"ft",12},{"yd",36}} the inputs '36in', '3ft', and '1yd'
    // would all be converted to 36. Some behavior can be adjusted with
    // Cli::fUnit* flags.
    A & anyUnits(
        std::initializer_list<std::pair<char const *, double>> units,
        int flags = 0 // Cli::fUnit * flags
    );
    template <typename InputIt>
    A & anyUnits(InputIt first, InputIt last, int flags = 0);

    // Forces the value to be within the range, if it's less than the low
    // it's set to the low, if higher than high it's made merely high.
    A & clamp(T const & low, T const & high);

    // Fail if the value given for this option is not in within the range
    // (inclusive) of low to high.
    A & range(T const & low, T const & high);

    // Causes a check whether the option value was set during parsing, and
    // reports badUsage() if it wasn't.
    A & require();

    // Enables prompting. When the option hasn't been provided on the command
    // line the user will be prompted for it. Use Cli::fPrompt* flags to
    // adjust behavior.
    A & prompt(int flags = 0);
    A & prompt(
        std::string const & msg, // custom prompt message
        int flags = 0            // Cli::fPrompt* flags
    );

    // Function signature of actions that are tied to options.
    using ActionFn = bool(Cli & cli, A & opt, std::string const & val);

    // Change the action to take when parsing this argument. The function
    // should:
    //  - Parse the src string and use the result to set the value (or
    //    push_back the new value for vectors).
    //  - Call cli.badUsage() with an error message if there's a problem.
    //  - Return false if the program should stop, otherwise true. This
    //    could be due to error or just to early out like "--version" and
    //    "--help".
    //
    // You can use opt.from() and opt.pos() to get the argument name that the
    // value was attached to on the command line and its position in argv[].
    // For bool arguments the source value string will always be either "0"
    // or "1".
    //
    // If you just need support for a new type you can provide a istream
    // extraction (>>) or assignment from string operator and the default
    // parse action will pick it up.
    A & parse(std::function<ActionFn> fn);

    // Action to take immediately after each value is parsed, unlike parsing
    // itself where there can only be one action, any number of check actions
    // can be added. They will be called in the order they were added and if
    // any of them return false it stops processing. As an example,
    // opt.clamp() and opt.range() both do their job by adding check actions.
    //
    // The function should:
    //  - Check the options new value, possibly in relation to other options.
    //  - Call cli.badUsage() with an error message if there's a problem.
    //  - Return false if the program should stop, otherwise true to let
    //    processing continue.
    //
    // The opt is fully populated so *opt, opt.from(), etc are all available.
    A & check(std::function<ActionFn> fn);

    // Action to run after all arguments have been parsed, any number of
    // after actions can be added and will, for each option, be called in the
    // order they're added. When using subcommands, the after actions bound
    // to unselected subcommands are not executed. The function should:
    //  - Do something interesting.
    //  - Call cli.badUsage() and return false on error.
    //  - Return true if processing should continue.
    A & after(std::function<ActionFn> fn);

    //-----------------------------------------------------------------------
    // QUERIES

    T const & implicitValue() const { return m_implicitValue; }
    T const & defaultValue() const { return m_defValue; }

protected:
    std::string defaultValueDesc() const final;
    bool doParseAction(Cli & cli, std::string const & value) final;
    bool doCheckAction(Cli & cli, std::string const & value) final;
    bool doAfterActions(Cli & cli) final;
    bool inverted() const final;
    bool exec(
        Cli & cli,
        std::string const & value,
        std::vector<std::function<ActionFn>> const & actions
    );

    // If numeric_limits<T>::min & max are defined and 'x' is outside of
    // those limits badRange() is called, otherwise returns true.
    template <typename U>
    auto checkLimits(Cli & cli, std::string const & val, U const & x, int)
    -> decltype(std::declval<T&>() > x);
    template <typename U>
    bool checkLimits(Cli & cli, std::string const & val, U const & x, long);

    std::function<ActionFn> m_parse;
    std::vector<std::function<ActionFn>> m_checks;
    std::vector<std::function<ActionFn>> m_afters;

    T m_implicitValue{};
    T m_defValue{};
    std::vector<T> m_choices;
};

//===========================================================================
template <typename A, typename T>
Cli::OptShim<A, T>::OptShim(std::string const & keys, bool boolean)
    : OptBase(keys, boolean)
{
	if (std::is_arithmetic<T>::value) {
		auto l = std::locale("");
		auto rl = this->imbue(l);
	}
}

//===========================================================================
template <typename A, typename T>
inline std::string Cli::OptShim<A, T>::defaultValueDesc() const {
    return this->toValueDesc<T>();
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::doParseAction(
    Cli & cli,
    std::string const & val
) {
    auto self = static_cast<A *>(this);
    return m_parse(cli, *self, val);
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::doCheckAction(
    Cli & cli,
    std::string const & val
) {
    return exec(cli, val, m_checks);
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::doAfterActions(Cli & cli) {
    return exec(cli, {}, m_afters);
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::inverted() const {
    return this->m_bool && this->m_flagValue && this->m_flagDefault;
}

//===========================================================================
template <>
inline bool Cli::OptShim<Cli::Opt<bool>, bool>::inverted() const {
    // bool options are always marked as bool
    assert(this->m_bool && "internal dimcli error");
    if (this->m_flagValue)
        return this->m_flagDefault;
    return this->defaultValue();
}

//===========================================================================
template <typename A, typename T>
inline bool Cli::OptShim<A, T>::exec(
    Cli & cli,
    std::string const & val,
    std::vector<std::function<ActionFn>> const & actions
) {
    auto self = static_cast<A *>(this);
    for (auto && fn : actions) {
        if (!fn(cli, *self, val))
            return false;
    }
    return true;
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::command(std::string const & val) {
    m_command = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::group(std::string const & val) {
    m_group = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::show(bool visible) {
    m_visible = visible;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::desc(std::string const & val) {
    m_desc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::valueDesc(std::string const & val) {
    m_valueDesc = val;
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::defaultDesc(std::string const & val) {
    m_defaultDesc = val;
    if (val.empty())
        m_defaultDesc.push_back('\0');
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::defaultValue(T const & val) {
    m_defValue = val;
    for (auto && cd : m_choiceDescs)
        cd.second.def = !this->m_vector && val == m_choices[cd.second.pos];
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::implicitValue(T const & val) {
    if (m_bool) {
        // bools don't have separate values, just their presence/absence
        assert(!"bool argument values can't be implicit");
    } else {
        m_implicitValue = val;
    }
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::flagValue(bool isDefault) {
    auto self = static_cast<A *>(this);
    m_flagValue = true;
    if (!self->m_proxy->m_defFlagOpt) {
        self->m_proxy->m_defFlagOpt = self;
        m_flagDefault = true;
    } else if (isDefault) {
        self->m_proxy->m_defFlagOpt->m_flagDefault = false;
        self->m_proxy->m_defFlagOpt = self;
        m_flagDefault = true;
    } else {
        m_flagDefault = false;
    }
    m_bool = true;
    return *self;
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::choice(
    T const & val,
    std::string const & key,
    std::string const & desc,
    std::string const & sortKey
) {
    // The empty string isn't a valid choice because it can't be specified on
    // the command line, where unspecified picks the default instead.
    assert(!key.empty() && "an empty string can't be a choice");
    auto & cd = m_choiceDescs[key];
    cd.pos = m_choices.size();
    cd.desc = desc;
    cd.sortKey = sortKey;
    cd.def = (!this->m_vector && val == this->defaultValue());
    m_choices.push_back(val);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::parse(std::function<ActionFn> fn) {
    this->m_parse = move(fn);
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::check(std::function<ActionFn> fn) {
    this->m_checks.push_back(move(fn));
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::after(std::function<ActionFn> fn) {
    this->m_afters.push_back(move(fn));
    return static_cast<A &>(*this);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::require() {
    return after(&Cli::requireAction);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::siUnits(std::string const & symbol, int flags) {
    auto units = siUnitMapping(symbol, flags);
    return anyUnits(units.begin(), units.end(), flags);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::timeUnits(int flags) {
    return anyUnits({
            {"y", 365 * 24 * 60 * 60},
            {"w", 7 * 24 * 60 * 60},
            {"d", 24 * 60 * 60},
            {"h", 60 * 60},
            {"m", 60},
            {"min", 60},
            {"s", 1},
            {"ms", 1e-3},
            {"us", 1e-6},
            {"ns", 1e-9},
        },
        flags
    );
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::anyUnits(
    std::initializer_list<std::pair<char const *, double>> units,
    int flags
) {
    return anyUnits(units.begin(), units.end(), flags);
}

//===========================================================================
template <typename A, typename T>
template <typename U>
auto Cli::OptShim<A, T>::checkLimits(
    Cli & cli,
    std::string const & val,
    U const & x,
    int
) -> decltype(std::declval<T&>() > x)
{
    constexpr auto low = std::numeric_limits<T>::min();
    constexpr auto high = std::numeric_limits<T>::max();
    if (!std::is_arithmetic<T>::value)
        return true;
    return (x >= low && x <= high)
        || cli.badRange(*this, val, low, high);
}

//===========================================================================
template <typename A, typename T>
template <typename U>
bool Cli::OptShim<A, T>::checkLimits(
    Cli &,
    std::string const &,
    U const &,
    long
) {
    return true;
}

//===========================================================================
template <typename A, typename T>
template <typename InputIt>
A & Cli::OptShim<A, T>::anyUnits(InputIt first, InputIt last, int flags) {
    if (m_valueDesc.empty()) {
        m_valueDesc = defaultValueDesc()
            + ((flags & fUnitRequire) ? "<units>" : "[<units>]");
    }
    std::unordered_map<std::string, long double> units;
    if (flags & fUnitInsensitive) {
        auto & f = std::use_facet<std::ctype<char>>(std::locale());
        std::string name;
        for (; first != last; ++first) {
            name = first->first;
            f.tolower((char *) name.data(), name.data() + name.size());
            units[name] = first->second;
        }
    } else {
        units.insert(first, last);
    }
    return parse([units, flags](auto & cli, auto & opt, auto & val) {
        long double dval;
        bool success = true;
        if (!opt.withUnits(dval, val, units, flags))
            return cli.badUsage(opt, val);
        if (!opt.checkLimits(cli, val, dval, 0))
            return false;
        std::string sval;
        if (std::is_integral<T>::value)
            dval = std::round(dval);
        auto ival = (int64_t) dval;
        if (ival == dval) {
            sval = std::to_string(ival);
        } else {
            success = opt.toString(sval, dval);
            assert(success
                && "internal dimcli error, convert double to string failed"
            );
        }
        if (!opt.parseValue(sval))
            return cli.badUsage(opt, val);
        return success;
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::clamp(T const & low, T const & high) {
    assert(!(high < low) && "bad clamp, low greater than high");
    return check([low, high](auto & /* cli */, auto & opt, auto & /* val */) {
        if (*opt < low) {
            *opt = low;
        } else if (*opt > high) {
            *opt = high;
        }
        return true;
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::range(T const & low, T const & high) {
    assert(!(high < low) && "bad range, low greater than high");
    return check([low, high](auto & cli, auto & opt, auto & val) {
        return (*opt >= low && *opt <= high)
            || cli.badRange(opt, val, low, high);
    });
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::prompt(int flags) {
    return prompt({}, flags);
}

//===========================================================================
template <typename A, typename T>
A & Cli::OptShim<A, T>::prompt(std::string const & msg, int flags) {
    return after([=](auto & cli, auto & opt, auto & /* val */) {
        return cli.prompt(opt, msg, flags);
    });
}


/****************************************************************************
*
*   Cli::ArgMatch
*
*   Reference to the command line argument that was used to populate a value
*
***/

struct Cli::ArgMatch {
    // Name of the argument that populated the value, or an empty
    // string if it wasn't populated.
    std::string name;

    // Member of argv[] that populated the value or 0 if it wasn't.
    int pos{};
};


/****************************************************************************
*
*   Cli::Value
*
***/

template <typename T>
struct Cli::Value {
    // Where the value came from.
    ArgMatch m_match;

    // Whether the value was explicitly set.
    bool m_explicit{};

    // Points to the opt with the default flag value.
    Opt<T> * m_defFlagOpt{};

    T * m_value{};
    T m_internal{};

    Value(T * value) : m_value(value ? value : &m_internal) {}
};


/****************************************************************************
*
*   Cli::Opt
*
***/

template <typename T>
class Cli::Opt : public OptShim<Opt<T>, T> {
public:
    Opt(std::shared_ptr<Value<T>> value, std::string const & keys);

    T & operator*() { return *m_proxy->m_value; }
    T * operator->() { return m_proxy->m_value; }

    // Inherited via OptBase
    std::string const & from() const final { return m_proxy->m_match.name; }
    int pos() const final { return m_proxy->m_match.pos; }
    size_t size() const final { return 1; }
    void reset() final;
    bool parseValue(std::string const & value) final;
    void unspecifiedValue() final;

private:
    friend class Cli;
    bool defaultValueToString(std::string & out) const final;
    bool assign(std::string const & name, size_t pos) final;
    bool assigned() const final { return m_proxy->m_explicit; }
    bool sameValue(void const * value) const final {
        return value == m_proxy->m_value;
    }

    std::shared_ptr<Value<T>> m_proxy;
};

//===========================================================================
template <typename T>
Cli::Opt<T>::Opt(
    std::shared_ptr<Value<T>> value,
    std::string const & keys
)
    : OptShim<Opt, T>{keys, std::is_same<T, bool>::value}
    , m_proxy{value}
{}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::parseValue(std::string const & value) {
    auto & tmp = *m_proxy->m_value;
    if (this->m_flagValue) {
        // Value passed for flagValue (just like bools) is generated
        // internally and will be 0 or 1.
        if (value == "1") {
            tmp = this->defaultValue();
        } else {
            assert(value == "0" && "internal dimcli error");
        }
        return true;
    }
    if (!this->m_choices.empty()) {
        auto i = this->m_choiceDescs.find(value);
        if (i == this->m_choiceDescs.end())
            return false;
        tmp = this->m_choices[i->second.pos];
        return true;
    }
    return this->fromString(tmp, value);
}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::defaultValueToString(std::string & out) const {
    return this->toString(out, this->defaultValue());
}

//===========================================================================
template <typename T>
inline bool Cli::Opt<T>::assign(std::string const & name, size_t pos) {
    m_proxy->m_match.name = name;
    m_proxy->m_match.pos = (int)pos;
    m_proxy->m_explicit = true;
    return true;
}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::reset() {
    if (!this->m_flagValue || this->m_flagDefault)
        *m_proxy->m_value = this->defaultValue();
    m_proxy->m_match.name.clear();
    m_proxy->m_match.pos = 0;
    m_proxy->m_explicit = false;
}

//===========================================================================
template <typename T>
inline void Cli::Opt<T>::unspecifiedValue() {
    *m_proxy->m_value = this->implicitValue();
}


/****************************************************************************
*
*   Cli::ValueVec
*
***/

template <typename T>
struct Cli::ValueVec {
    // Where the values came from.
    std::vector<ArgMatch> m_matches;

    // Points to the opt with the default flag value.
    OptVec<T> * m_defFlagOpt{};

    std::vector<T> * m_values{};
    std::vector<T> m_internal;

    ValueVec(std::vector<T> * value)
        : m_values(value ? value : &m_internal)
    {}
};


/****************************************************************************
*
*   Cli::OptVec
*
***/

template <typename T>
class Cli::OptVec : public OptShim<OptVec<T>, T> {
public:
    OptVec(
        std::shared_ptr<ValueVec<T>> values,
        std::string const & keys,
        int nargs
    );

    std::vector<T> & operator*() { return *m_proxy->m_values; }
    std::vector<T> * operator->() { return m_proxy->m_values; }

    T & operator[](size_t index) { return (*m_proxy->m_values)[index]; }

    // Information about a specific member of the vector of values at the
    // time it was parsed. If the value vector has been changed (sort, erase,
    // insert, etc) by the application these will no longer correspond.
    std::string const & from(size_t index) const;
    int pos(size_t index) const;

    // Inherited via OptBase
    std::string const & from() const final { return from(size() - 1); }
    int pos() const final { return pos(size() - 1); }
    size_t size() const final { return m_proxy->m_values->size(); }
    void reset() final;
    bool parseValue(std::string const & value) final;
    void unspecifiedValue() final;

private:
    friend class Cli;
    bool defaultValueToString(std::string & out) const final;
    bool assign(std::string const & name, size_t pos) final;
    bool assigned() const final { return !m_proxy->m_values->empty(); }
    bool sameValue(void const * value) const final {
        return value == m_proxy->m_values;
    }

    std::shared_ptr<ValueVec<T>> m_proxy;
    std::string m_empty;
};

//===========================================================================
template <typename T>
Cli::OptVec<T>::OptVec(
    std::shared_ptr<ValueVec<T>> values,
    std::string const & keys,
    int nargs
)
    : OptShim<OptVec, T>{keys, std::is_same<T, bool>::value}
    , m_proxy(values)
{
    assert(nargs >= -1
        && "max values in a vector option must be >= 0, or -1 (unlimited)");
    this->m_vector = true;
    this->m_nargs = nargs;
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::parseValue(std::string const & value) {
    auto & tmp = m_proxy->m_values->back();
    if (this->m_flagValue) {
        // Value passed for flagValue (just like bools) is generated
        // internally and will be 0 or 1.
        if (value == "1") {
            tmp = this->defaultValue();
        } else {
            assert(value == "0" && "internal dimcli error");
            m_proxy->m_values->pop_back();
            m_proxy->m_matches.pop_back();
        }
        return true;
    }
    if (!this->m_choices.empty()) {
        auto i = this->m_choiceDescs.find(value);
        if (i == this->m_choiceDescs.end())
            return false;
        tmp = this->m_choices[i->second.pos];
        return true;
    }

    return this->fromString(tmp, value);
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::defaultValueToString(std::string & out) const {
    out.clear();
    return false;
}

//===========================================================================
template <typename T>
inline bool Cli::OptVec<T>::assign(std::string const & name, size_t pos) {
    if (this->m_nargs != -1
        && (size_t) this->m_nargs == m_proxy->m_matches.size()
    ) {
        return false;
    }

    ArgMatch match;
    match.name = name;
    match.pos = (int)pos;
    m_proxy->m_matches.push_back(match);
    m_proxy->m_values->resize(m_proxy->m_matches.size());
    return true;
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::reset() {
    m_proxy->m_values->clear();
    m_proxy->m_matches.clear();
}

//===========================================================================
template <typename T>
inline void Cli::OptVec<T>::unspecifiedValue() {
    m_proxy->m_values->back() = this->implicitValue();
}

//===========================================================================
template <typename T>
inline std::string const & Cli::OptVec<T>::from(size_t index) const {
    return index >= size() ? m_empty : m_proxy->m_matches[index].name;
}

//===========================================================================
template <typename T>
inline int Cli::OptVec<T>::pos(size_t index) const {
    return index >= size() ? 0 : m_proxy->m_matches[index].pos;
}

} // namespace


/****************************************************************************
*
*   Restore settings
*
***/

// Restore as many compiler settings as possible so they don't leak into the
// application.
#ifndef DIMCLI_LIB_KEEP_MACROS

// clear all dim header macros so they don't leak into the application
#undef DIMCLI_LIB_DYN_LINK
#undef DIMCLI_LIB_KEEP_MACROS
#undef DIMCLI_LIB_STANDALONE
#undef DIMCLI_LIB_WINAPI_FAMILY_APP

#undef DIMCLI_LIB_DECL
#undef DIMCLI_LIB_NO_ENV
#undef DIMCLI_LIB_NO_CONSOLE
#undef DIMCLI_LIB_SOURCE

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#undef DIMCLI_LIB_FILESYSTEM
#undef DIMCLI_LIB_FILESYSTEM_PATH

#endif

#pragma warning(pop)