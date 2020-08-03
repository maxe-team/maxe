// Copyright Glen Knowles 2016 - 2018.
// Distributed under the Boost Software License, Version 1.0.
//
// cli.cpp - dimcli
//
// For documentation and examples:
// https://github.com/gknowles/dimcli

#ifndef DIMCLI_LIB_SOURCE
#define DIMCLI_LIB_SOURCE
#endif
#include "cli.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>

using namespace std;
using namespace Dim;
#ifdef DIMCLI_LIB_FILESYSTEM
namespace fs = DIMCLI_LIB_FILESYSTEM;
#endif

// getenv triggers the visual c++ security warning
#if (_MSC_VER >= 1400)
#pragma warning(disable : 4996) // this function or variable may be unsafe.
#endif


/****************************************************************************
*
*   Tuning parameters
*
***/

// column where description text starts
size_t const kMinDescCol = 11;
size_t const kMaxDescCol = 28;

// maximum help text line length
size_t const kMaxLineWidth = 79;


/****************************************************************************
*
*   Declarations
*
***/

// Name of group containing --help, --version, etc
char const kInternalOptionGroup[] = "~";

namespace {

struct GroupConfig {
    string name;
    string title;
    string sortKey;
};

struct CommandConfig {
    string name;
    string header;
    string desc;
    string footer;
    function<Cli::ActionFn> action;
    string cmdGroup;
    Cli::Opt<bool> * helpOpt{};
    unordered_map<string, GroupConfig> groups;
};

struct OptName {
    Cli::OptBase * opt;
    bool invert;        // set to false instead of true (only for bools)
    bool optional;      // value need not be present? (non-bools only)
    string name;        // name of argument (only for positionals)
    int pos;
};

struct ArgKey {
    string sort; // sort key
    string list;
    Cli::OptBase * opt{};
};

// Option name filters for opts that are externally bool
enum NameListType {
    kNameEnable,     // include names that enable the opt
    kNameDisable,    // include names that disable
    kNameAll,        // include all names
    kNameNonDefault, // include names that change from the default
};

struct CodecvtWchar : codecvt<wchar_t, char, mbstate_t> {
    // public destructor required for use with wstring_convert
    ~CodecvtWchar() {}
};

} // namespace

struct Cli::OptIndex {
    unordered_map<char, OptName> m_shortNames;
    unordered_map<string, OptName> m_longNames;
    vector<OptName> m_argNames;
    bool m_allowCommands{};

    void index(
        Cli const & cli,
        string const & cmd,
        bool requireVisible
    );
    bool findNamedArgs(
        vector<ArgKey> & namedArgs,
        size_t & colWidth,
        Cli const & cli,
        CommandConfig & cmd,
        NameListType type,
        bool flatten
    ) const;
    string nameList(
        Cli const & cli,
        OptBase const & opt,
        NameListType type
    ) const;

    void index(OptBase & opt);
    void indexName(OptBase & opt, string const & name, int pos);
    void indexShortName(
        OptBase & opt,
        char name,
        bool invert,
        bool optional,
        int pos
    );
    void indexLongName(
        OptBase & opt,
        string const & name,
        bool invert,
        bool optional,
        int pos
    );
};

struct Cli::Config {
    vector<function<BeforeFn>> befores;
    unordered_map<string, CommandConfig> cmds;
    unordered_map<string, GroupConfig> cmdGroups;
    list<unique_ptr<OptBase>> opts;
    bool responseFiles{true};
    string envOpts;
    istream * conin{&cin};
    ostream * conout{&cout};
    shared_ptr<locale> defLoc = make_shared<locale>();
    shared_ptr<locale> numLoc = make_shared<locale>("");

    int exitCode{kExitOk};
    string errMsg;
    string errDetail;
    string progName;
    string command;

    static void touchAllCmds(Cli & cli);
    static CommandConfig & findCmdAlways(Cli & cli);
    static CommandConfig & findCmdAlways(Cli & cli, string const & name);
    static CommandConfig const & findCmdOrDie(Cli const & cli);

    static GroupConfig & findCmdGrpAlways(Cli & cli);
    static GroupConfig & findCmdGrpAlways(Cli & cli, string const & name);
    static GroupConfig & findCmdGrpOrDie(Cli const & cli);

    static GroupConfig & findGrpAlways(Cli & cli);
    static GroupConfig & findGrpAlways(
        CommandConfig & cmd,
        string const & name
    );
    static GroupConfig const & findGrpOrDie(Cli const & cli);
};


/****************************************************************************
*
*   Helpers
*
***/

// forward declarations
static bool helpOptAction(Cli & cli, Cli::Opt<bool> & opt, string const & val);
static bool defCmdAction(Cli & cli);
static void printChoices(
    ostream & os,
    unordered_map<string, Cli::OptBase::ChoiceDesc> const & choices
);

//===========================================================================
#if defined(_WIN32)
static string displayName(string const & file) {
    char fname[_MAX_FNAME];
    _splitpath(file.c_str(), nullptr, nullptr, fname, nullptr);
    return fname;
}
#else
#include <libgen.h>
static string displayName(string file) {
    return basename((char *) file.data());
}
#endif

//===========================================================================
// Replaces a set of contiguous values in one vector with the entire contents
// of another, growing or shrinking it as needed.
template <typename T>
static void replace(
    vector<T> & out,
    size_t pos,
    size_t count,
    vector<T> && src
) {
    auto srcLen = src.size();
    if (count > srcLen) {
        out.erase(out.begin() + pos + srcLen, out.begin() + pos + count);
    } else if (count < srcLen) {
        out.insert(out.begin() + pos + count, srcLen - count, {});
    }
    auto i = out.begin() + pos;
    for (auto && val : src)
        *i++ = move(val);
}

//===========================================================================
static string trim(string const & val) {
    auto first = val.c_str();
    auto last = first + val.size();
    while (isspace(*first))
        ++first;
    if (!*first)
        return {};
    while (isspace(*--last))
        ;
    return string(first, last - first + 1);
}


/****************************************************************************
*
*   CliLocal
*
***/

//===========================================================================
CliLocal::CliLocal()
    : Cli(make_shared<Config>())
{}


/****************************************************************************
*
*   Cli::Config
*
***/

//===========================================================================
// static
void Cli::Config::touchAllCmds(Cli & cli) {
    // Make sure all opts have a backing command config
    for (auto && opt : cli.m_cfg->opts)
        Config::findCmdAlways(cli, opt->m_command);
    // Make sure all commands have a backing command group
    for (auto && cmd : cli.m_cfg->cmds)
        Config::findCmdGrpAlways(cli, cmd.second.cmdGroup);
}

//===========================================================================
// static
CommandConfig & Cli::Config::findCmdAlways(Cli & cli) {
    return findCmdAlways(cli, cli.command());
}

//===========================================================================
// static
CommandConfig & Cli::Config::findCmdAlways(
    Cli & cli,
    string const & name
) {
    auto & cmds = cli.m_cfg->cmds;
    auto i = cmds.find(name);
    if (i != cmds.end())
        return i->second;

    auto & cmd = cmds[name];
    cmd.name = name;
    cmd.action = defCmdAction;
    cmd.cmdGroup = cli.cmdGroup();
    auto & defGrp = findGrpAlways(cmd, "");
    defGrp.title = "Options";
    auto & intGrp = findGrpAlways(cmd, kInternalOptionGroup);
    intGrp.title.clear();
    auto & hlp = cli.opt<bool>("help.")
        .desc("Show this message and exit.")
        .check(helpOptAction)
        .command(name)
        .group(kInternalOptionGroup);
    cmd.helpOpt = &hlp;
    return cmd;
}

//===========================================================================
// static
CommandConfig const & Cli::Config::findCmdOrDie(Cli const & cli) {
    auto & cmds = cli.m_cfg->cmds;
    auto i = cmds.find(cli.command());
    assert(i != cmds.end()
        && "internal dimcli error: uninitialized command context");
    return i->second;
}

//===========================================================================
// static
GroupConfig & Cli::Config::findCmdGrpAlways(Cli & cli) {
    return findCmdGrpAlways(cli, cli.cmdGroup());
}

//===========================================================================
// static
GroupConfig & Cli::Config::findCmdGrpAlways(Cli & cli, string const & name) {
    auto & grps = cli.m_cfg->cmdGroups;
    auto i = grps.find(name);
    if (i != grps.end())
        return i->second;

    auto & grp = grps[name];
    grp.name = grp.sortKey = name;
    if (name.empty()) {
        grp.title = "Commands";
    } else if (name == kInternalOptionGroup) {
        grp.title = "";
    } else {
        grp.title = name;
    }
    return grp;
}

//===========================================================================
// static
GroupConfig & Cli::Config::findCmdGrpOrDie(Cli const & cli) {
    auto & name = cli.cmdGroup();
    auto & grps = cli.m_cfg->cmdGroups;
    auto i = grps.find(name);
    assert(i != grps.end()
        && "internal dimcli error: uninitialized command group context");
    return i->second;
}

//===========================================================================
// static
GroupConfig & Cli::Config::findGrpAlways(Cli & cli) {
    return findGrpAlways(findCmdAlways(cli), cli.group());
}

//===========================================================================
// static
GroupConfig & Cli::Config::findGrpAlways(
    CommandConfig & cmd,
    string const & name
) {
    auto i = cmd.groups.find(name);
    if (i != cmd.groups.end())
        return i->second;
    auto & grp = cmd.groups[name];
    grp.name = grp.sortKey = grp.title = name;
    return grp;
}

//===========================================================================
// static
GroupConfig const & Cli::Config::findGrpOrDie(Cli const & cli) {
    auto & grps = Config::findCmdOrDie(cli).groups;
    auto i = grps.find(cli.group());
    assert(i != grps.end()
        && "internal dimcli error: uninitialized group context");
    return i->second;
}


/****************************************************************************
*
*   Cli::OptBase
*
***/

//===========================================================================
Cli::OptBase::OptBase(string const & names, bool boolean)
    : m_bool{boolean}
    , m_names{names}
{
    // set m_fromName and assert if names is malformed
    OptIndex ndx;
    ndx.index(*this);
}

//===========================================================================
locale Cli::OptBase::imbue(locale const & loc) {
    return m_interpreter.imbue(loc);
}

//===========================================================================
string Cli::OptBase::defaultPrompt() const {
    auto name = (string) m_fromName;
    while (name.size() && name[0] == '-')
        name.erase(0, 1);
    if (name.size())
        name[0] = (char)toupper(name[0]);
    return name;
}

//===========================================================================
void Cli::OptBase::setNameIfEmpty(string const & name) {
    if (m_fromName.empty())
        m_fromName = name;
}

//===========================================================================
bool Cli::OptBase::withUnits(
    long double & out,
    string const & val,
    unordered_map<string, long double> const & units,
    int flags
) const {
    auto & f = use_facet<ctype<char>>(m_interpreter.getloc());

    auto pos = val.size();
    for (;;) {
        if (!pos--)
            return false;
        if (f.is(f.digit, val[pos]) || val[pos] == '.') {
            pos += 1;
            break;
        }
    }
    auto num = val.substr(0, pos);
    auto unit = val.substr(pos);

    if (!fromString(out, num))
        return false;
    if (unit.empty())
        return (~flags & fUnitRequire);

    if (flags & fUnitInsensitive)
        f.tolower((char *) unit.data(), unit.data() + unit.size());
    auto i = units.find(unit);
    if (i != units.end()) {
        out *= i->second;
        return true;
    } else {
        return false;
    }
}


/****************************************************************************
*
*   Cli::OptIndex
*
***/

//===========================================================================
void Cli::OptIndex::index(
    Cli const & cli,
    string const & cmd,
    bool requireVisible
) {
    m_argNames.clear();
    m_longNames.clear();
    m_shortNames.clear();
    m_allowCommands = cmd.empty();
    for (auto && opt : cli.m_cfg->opts) {
        if (opt->m_command == cmd && (opt->m_visible || !requireVisible))
            index(*opt);
    }
    for (unsigned i = 0; i < m_argNames.size(); ++i) {
        auto & key = m_argNames[i];
        if (key.name.empty())
            key.name = "arg" + to_string(i + 1);
    }
}

//===========================================================================
bool Cli::OptIndex::findNamedArgs(
    vector<ArgKey> & namedArgs,
    size_t & colWidth,
    Cli const & cli,
    CommandConfig & cmd,
    NameListType type,
    bool flatten
) const {
    namedArgs.clear();
    for (auto && opt : cli.m_cfg->opts) {
        auto list = nameList(cli, *opt, type);
        if (auto width = list.size()) {
            colWidth = max(colWidth, width);
            ArgKey key;
            key.opt = opt.get();
            key.list = list;

            // sort by group sort key followed by name list with leading
            // dashes removed
            key.sort = Config::findGrpAlways(cmd, opt->m_group).sortKey;
            if (flatten && key.sort != kInternalOptionGroup)
                key.sort.clear();
            key.sort += '\0';
            key.sort += list.substr(list.find_first_not_of('-'));
            namedArgs.push_back(key);
        }
    }
    sort(namedArgs.begin(), namedArgs.end(), [](auto & a, auto & b) {
        return a.sort < b.sort;
    });
    return !namedArgs.empty();
}

//===========================================================================
static bool includeName(
    OptName const & name,
    NameListType type,
    Cli::OptBase const & opt,
    bool boolean,
    bool inverted
) {
    if (name.opt != &opt)
        return false;
    if (boolean) {
        if (type == kNameEnable)
            return !name.invert;
        if (type == kNameDisable)
            return name.invert;

        // includeName is always called with a filter (i.e. not kNameAll)
        assert(type == kNameNonDefault
            && "internal dimcli error: unknown NameListType");
        return inverted == name.invert;
    }
    return true;
}

//===========================================================================
string Cli::OptIndex::nameList(
    Cli const & cli,
    Cli::OptBase const & opt,
    NameListType type
) const {
    string list;

    if (type == kNameAll) {
        list = nameList(cli, opt, kNameEnable);
        if (opt.m_bool) {
            auto invert = nameList(cli, opt, kNameDisable);
            if (!invert.empty()) {
                list += list.empty() ? "/ " : " / ";
                list += invert;
            }
        }
        return list;
    }

    bool foundLong = false;
    bool optional = false;

    // names
    vector<const decltype(m_shortNames)::value_type *> snames;
    for (auto & sn : m_shortNames)
        snames.push_back(&sn);
    sort(snames.begin(), snames.end(), [](auto & a, auto & b) {
        return a->second.pos < b->second.pos;
    });
    for (auto && sn : snames) {
        if (!includeName(sn->second, type, opt, opt.m_bool, opt.inverted()))
            continue;
        optional = sn->second.optional;
        if (!list.empty())
            list += ", ";
        list += '-';
        list += sn->first;
    }
    vector<const decltype(m_longNames)::value_type *> lnames;
    for (auto & ln : m_longNames)
        lnames.push_back(&ln);
    sort(lnames.begin(), lnames.end(), [](auto & a, auto & b) {
        return a->second.pos < b->second.pos;
    });
    for (auto && ln : lnames) {
        if (!includeName(ln->second, type, opt, opt.m_bool, opt.inverted()))
            continue;
        optional = ln->second.optional;
        if (!list.empty())
            list += ", ";
        foundLong = true;
        list += "--";
        list += ln->first;
    }
    if (opt.m_bool || list.empty())
        return list;

    // value
    auto valDesc = opt.m_valueDesc.empty()
        ? opt.defaultValueDesc()
        : opt.m_valueDesc;
    if (optional) {
        list += foundLong ? "[=" : " [";
        list += valDesc + "]";
    } else {
        list += foundLong ? '=' : ' ';
        list += valDesc;
    }
    return list;
}

//===========================================================================
void Cli::OptIndex::index(OptBase & opt) {
    auto ptr = opt.m_names.c_str();
    string name;
    char close;
    bool hasPos = false;
    int pos = 0;
    for (;; ++ptr) {
        switch (*ptr) {
        case 0: return;
        case ' ': continue;
        case '[': close = ']'; break;
        case '<': close = '>'; break;
        default: close = ' '; break;
        }
        auto b = ptr;
        bool hasEqual = false;
        while (*ptr && *ptr != close) {
            if (*ptr == '=')
                hasEqual = true;
            ptr += 1;
        }
        if (hasEqual && close == ' ') {
            assert(!"bad argument name, contains '='");
        } else if (hasPos && close != ' ') {
            assert(!"argument with multiple positional names");
        } else {
            if (close == ' ') {
                name = string(b, ptr - b);
            } else {
                hasPos = true;
                name = string(b, 1);
                name += trim(string(b + 1, ptr - b - 1));
            }
            indexName(opt, name, pos);
            pos += 2;
        }
        if (!*ptr)
            return;
    }
}

//===========================================================================
void Cli::OptIndex::indexName(OptBase & opt, string const & name, int pos) {
    bool const kInvert = true;
    bool const kOptional = true;

    auto where = m_argNames.end();
    switch (name[0]) {
    case '-':
        assert(!"bad argument name, contains '-'");
        return;
    case '[':
        m_argNames.push_back({&opt, !kInvert, kOptional, name.data() + 1});
        where = m_argNames.end() - 1;
        goto INDEX_POS_NAME;
    case '<':
        where = find_if(
            m_argNames.begin(),
            m_argNames.end(),
            [](auto && key) { return key.optional; }
        );
        where = m_argNames.insert(
            where,
            {&opt, !kInvert, !kOptional, name.data() + 1}
        );
    INDEX_POS_NAME:
        opt.setNameIfEmpty(where->name);
        if (opt.m_command.empty())
            m_allowCommands = false;
        return;
    }
    bool invert = false;
    bool optional = false;
    auto prefix = 0;
    if (name.size() > 1) {
        switch (name[0]) {
        case '!':
            if (!opt.m_bool) {
                // Inversion doesn't make any sense for non-bool options and
                // will be ignored for them. But it is allowed to be specified
                // because they might be turned into flagValues, and flagValues
                // act like bools syntacticly.
                //
                // And the case of an inverted bool converted to a flagValue
                // has to be handled anyway.
            }
            prefix = 1;
            invert = true;
            break;
        case '?':
            if (opt.m_bool) {
                // Bool options don't have values, only their presences or
                // absence, therefore they can't have optional values.
                assert(!"bad modifier '?' for bool argument");
                return;
            }
            prefix = 1;
            optional = true;
            break;
        }
    }
    if (name.size() - prefix == 1) {
        indexShortName(opt, name[prefix], invert, optional, pos);
    } else {
        indexLongName(opt, name.substr(prefix), invert, optional, pos);
    }
}

//===========================================================================
void Cli::OptIndex::indexShortName(
    OptBase & opt,
    char name,
    bool invert,
    bool optional,
    int pos
) {
    m_shortNames[name] = {&opt, invert, optional, {}, pos};
    opt.setNameIfEmpty("-"s + name);
}

//===========================================================================
void Cli::OptIndex::indexLongName(
    OptBase & opt,
    string const & name,
    bool invert,
    bool optional,
    int pos
) {
    bool allowNo = true;
    auto key = string{name};
    if (key.back() == '.') {
        allowNo = false;
        if (key.size() == 2) {
            assert(!"bad modifier '.' for short name");
            return;
        }
        key.pop_back();
    }
    opt.setNameIfEmpty("--" + key);
    m_longNames[key] = {&opt, invert, optional, {}, pos};
    if (opt.m_bool && allowNo)
        m_longNames["no-" + key] = {&opt, !invert, optional, {}, pos + 1};
}


/****************************************************************************
*
*   Action callbacks
*
***/

//===========================================================================
// static
bool Cli::defParseAction(Cli & cli, OptBase & opt, string const & val) {
    if (opt.parseValue(val))
        return true;

    ostringstream os;
    printChoices(os, opt.m_choiceDescs);
    return cli.badUsage(opt, val, os.str());
}

//===========================================================================
// static
bool Cli::requireAction(Cli & cli, OptBase & opt, string const &) {
    if (opt)
        return true;
    string const & name = !opt.defaultFrom().empty()
        ? opt.defaultFrom()
        : "UNKNOWN";
    return cli.badUsage("No value given for " + name);
}

//===========================================================================
static bool helpBeforeAction(Cli &, vector<string> & args) {
    if (args.size() == 1)
        args.push_back("--help");
    return true;
}

//===========================================================================
static bool helpOptAction(
    Cli & cli,
    Cli::Opt<bool> & opt,
    string const & // val
) {
    if (*opt) {
        cli.printHelp(cli.conout(), {}, cli.commandMatched());
        return false;
    }
    return true;
}

//===========================================================================
static bool defCmdAction(Cli & cli) {
    if (cli.commandMatched().empty()) {
        return cli.fail(kExitUsage, "No command given.");
    } else {
        ostringstream os;
        os << "Command '" << cli.commandMatched() << "' has not been implemented.";
        return cli.fail(kExitSoftware, os.str());
    }
}

//===========================================================================
static bool helpCmdAction(Cli & cli) {
    Cli::OptIndex ndx;
    ndx.index(cli, cli.commandMatched(), false);
    auto cmd = *static_cast<Cli::Opt<string> &>(*ndx.m_argNames[0].opt);
    auto usage = *static_cast<Cli::Opt<bool> &>(*ndx.m_shortNames['u'].opt);
    if (!cli.commandExists(cmd)) {
        return cli.fail(
            kExitUsage,
            "Command 'help': Help requested for unknown command: " + cmd
        );
    }
    if (usage) {
        cli.printUsageEx(cli.conout(), {}, cmd);
    } else {
        cli.printHelp(cli.conout(), {}, cmd);
    }
    return true;
}


/****************************************************************************
*
*   Cli
*
***/

//===========================================================================
static shared_ptr<Cli::Config> globalConfig() {
    static auto s_cfg = make_shared<Cli::Config>();
    return s_cfg;
}

//===========================================================================
// Creates a handle to the shared configuration
Cli::Cli()
    : m_cfg(globalConfig())
{
    helpOpt();
}

//===========================================================================
Cli::Cli(Cli const & from)
    : m_cfg(from.m_cfg)
    , m_group(from.m_group)
    , m_command(from.m_command)
{}

//===========================================================================
// protected
Cli::Cli(shared_ptr<Config> cfg)
    : m_cfg(cfg)
{
    helpOpt();
}

//===========================================================================
Cli & Cli::operator=(Cli const & from) {
    m_cfg = from.m_cfg;
    m_group = from.m_group;
    m_command = from.m_command;
    return *this;
}

//===========================================================================
Cli & Cli::operator=(Cli && from) noexcept {
    m_cfg = move(from.m_cfg);
    m_group = move(from.m_group);
    m_command = move(from.m_command);
    return *this;
}


/****************************************************************************
*
*   Configuration
*
***/

//===========================================================================
Cli::Opt<bool> & Cli::confirmOpt(string const & prompt) {
    auto & ask = opt<bool>("y yes")
        .desc("Suppress prompting to allow execution.")
        .check([](auto &, auto & opt, auto &) { return *opt; })
        .prompt(prompt.empty() ? "Are you sure?" : prompt);
    return ask;
}

//===========================================================================
Cli::Opt<bool> & Cli::helpOpt() {
    return *Config::findCmdAlways(*this).helpOpt;
}

//===========================================================================
Cli::Opt<string> & Cli::passwordOpt(bool confirm) {
    return opt<string>("password.")
        .desc("Password required for access.")
        .prompt(fPromptHide | fPromptNoDefault | (confirm * fPromptConfirm));
}

//===========================================================================
Cli::Opt<bool> & Cli::versionOpt(
    string const & version,
    string const & progName
) {
    auto act = [version, progName](auto & cli, auto &/*opt*/, auto &/*val*/) {
        auto prog = string{progName};
        if (prog.empty())
            prog = displayName(cli.progName());
        cli.conout() << prog << " version " << version << endl;
        return false;
    };
    return opt<bool>("version.")
        .desc("Show version and exit.")
        .check(act)
        .group(kInternalOptionGroup);
}

//===========================================================================
Cli & Cli::group(string const & name) {
    m_group = name;
    Config::findGrpAlways(*this);
    return *this;
}

//===========================================================================
Cli & Cli::title(string const & val) {
    Config::findGrpAlways(*this).title = val;
    return *this;
}

//===========================================================================
Cli & Cli::sortKey(string const & val) {
    Config::findGrpAlways(*this).sortKey = val;
    return *this;
}

//===========================================================================
string const & Cli::title() const {
    return Config::findGrpOrDie(*this).title;
}

//===========================================================================
string const & Cli::sortKey() const {
    return Config::findGrpOrDie(*this).sortKey;
}

//===========================================================================
Cli & Cli::command(string const & name, string const & grpName) {
    Config::findCmdAlways(*this, name);
    m_command = name;
    m_group = grpName;
    return *this;
}

//===========================================================================
Cli & Cli::action(function<ActionFn> fn) {
    Config::findCmdAlways(*this).action = move(fn);
    return *this;
}

//===========================================================================
Cli & Cli::header(string const & val) {
    auto & hdr = Config::findCmdAlways(*this).header;
    hdr = val;
    if (hdr.empty())
        hdr.push_back('\0');
    return *this;
}

//===========================================================================
Cli & Cli::desc(string const & val) {
    Config::findCmdAlways(*this).desc = val;
    return *this;
}

//===========================================================================
Cli & Cli::footer(string const & val) {
    auto & ftr = Config::findCmdAlways(*this).footer;
    ftr = val;
    if (ftr.empty())
        ftr.push_back('\0');
    return *this;
}

//===========================================================================
string const & Cli::header() const {
    return Config::findCmdOrDie(*this).header;
}

//===========================================================================
string const & Cli::desc() const {
    return Config::findCmdOrDie(*this).desc;
}

//===========================================================================
string const & Cli::footer() const {
    return Config::findCmdOrDie(*this).footer;
}

//===========================================================================
Cli & Cli::helpCmd() {
    // Use new instance so the current context command is preserved.
    Cli cli{*this};
    cli.command("help")
        .cmdGroup(kInternalOptionGroup)
        .desc("Show help for individual commands and exit. If no command is "
            "given the list of commands and general options are shown.")
        .action(helpCmdAction);
    cli.opt<string>("[command]")
        .desc("Command to show help information about.");
    cli.opt<bool>("u usage")
        .desc("Only show condensed usage.");
    return *this;
}

//===========================================================================
Cli & Cli::helpNoArgs() {
    return before(helpBeforeAction);
}

//===========================================================================
Cli & Cli::cmdGroup(string const & name) {
    Config::findCmdAlways(*this).cmdGroup = name;
    Config::findCmdGrpAlways(*this);
    return *this;
}

//===========================================================================
Cli & Cli::cmdTitle(string const & val) {
    Config::findCmdGrpAlways(*this).title = val;
    return *this;
}

//===========================================================================
Cli & Cli::cmdSortKey(string const & key) {
    Config::findCmdGrpAlways(*this).sortKey = key;
    return *this;
}

//===========================================================================
string const & Cli::cmdGroup() const {
    return Config::findCmdOrDie(*this).cmdGroup;
}

//===========================================================================
string const & Cli::cmdTitle() const {
    return Config::findCmdGrpOrDie(*this).title;
}

//===========================================================================
string const & Cli::cmdSortKey() const {
    return Config::findCmdGrpOrDie(*this).sortKey;
}

//===========================================================================
void Cli::responseFiles(bool enable) {
    m_cfg->responseFiles = enable;
}

#if !defined(DIMCLI_LIB_NO_ENV)
//===========================================================================
void Cli::envOpts(string const & var) {
    m_cfg->envOpts = var;
}
#endif

//===========================================================================
Cli & Cli::before(function<BeforeFn> fn) {
    m_cfg->befores.push_back(move(fn));
    return *this;
}

//===========================================================================
Cli & Cli::iostreams(istream * in, ostream * out) {
    m_cfg->conin = in ? in : &cin;
    m_cfg->conout = out ? out : &cout;
    return *this;
}

//===========================================================================
istream & Cli::conin() {
    return *m_cfg->conin;
}

//===========================================================================
ostream & Cli::conout() {
    return *m_cfg->conout;
}

//===========================================================================
void Cli::addOpt(unique_ptr<OptBase> src) {
    m_cfg->opts.push_back(move(src));
}

//===========================================================================
Cli::OptBase * Cli::findOpt(void const * value) {
    if (value) {
        for (auto && opt : m_cfg->opts) {
            if (opt->sameValue(value))
                return opt.get();
        }
    }
    return nullptr;
}


/****************************************************************************
*
*   Parse argv
*
***/

//===========================================================================
// static
vector<string> Cli::toArgv(string const & cmdline) {
#if defined(_WIN32)
    return toWindowsArgv(cmdline);
#else
    return toGnuArgv(cmdline);
#endif
}

//===========================================================================
// static
vector<string> Cli::toArgv(size_t argc, char * argv[]) {
    vector<string> out;
    out.reserve(argc);
    for (unsigned i = 0; i < argc && argv[i]; ++i)
        out.push_back(argv[i]);
    assert(argc == out.size() && !argv[argc]
        && "bad arguments, argc and null terminator don't agree");
    return out;
}

//===========================================================================
// static
vector<string> Cli::toArgv(size_t argc, wchar_t * argv[]) {
    vector<string> out;
    out.reserve(argc);
    wstring_convert<CodecvtWchar> wcvt("BAD_ENCODING");
    for (unsigned i = 0; i < argc && argv[i]; ++i) {
        auto tmp = (string) wcvt.to_bytes(argv[i]);
        out.push_back(move(tmp));
    }
    assert(argc == out.size() && !argv[argc]
        && "bad arguments, argc and null terminator don't agree");
    return out;
}

//===========================================================================
// static
vector<char const *> Cli::toPtrArgv(vector<string> const & args) {
    vector<char const *> argv;
    argv.reserve(args.size() + 1);
    for (auto && arg : args)
        argv.push_back(arg.data());
    argv.push_back(nullptr);
    argv.pop_back();
    return argv;
}

//===========================================================================
// These rules where gleaned by inspecting glib's g_shell_parse_argv which
// takes its rules from the "Shell Command Language" section of the UNIX98
// spec -- ignoring parameter expansion ("$()" and "${}"), command
// substitution (back quote `), operators as separators, etc.
//
// Arguments are split on whitespace (" \t\r\n\f\v") unless the whitespace
// is escaped, quoted, or in a comment.
// - unquoted: any char following a backslash is replaced by that char,
//   except newline, which is removed. An unquoted '#' starts a comment.
// - comment: everything up to, but not including, the next newline is ignored
// - single quotes: preserve the string exactly, no escape sequences, not
//   even \'
// - double quotes: some chars ($ ' " \ and newline) are escaped when
//   following a backslash, a backslash not followed one of those five chars
//   is preserved. All other chars are preserved.
//
// When escaping it's simplest to not quote and just escape the following:
//   Must: | & ; < > ( ) $ ` \ " ' SP TAB CR LF FF VTAB
//   Should: * ? [ # ~ = %
//===========================================================================
// static
vector<string> Cli::toGlibArgv(string const & cmdline) {
    vector<string> out;
    char const * cur = cmdline.c_str();
    char const * last = cur + cmdline.size();

    string arg;

IN_GAP:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\':
            if (cur < last) {
                ch = *cur++;
                if (ch == '\n')
                    break;
            }
            arg += ch;
            goto IN_UNQUOTED;
        default: arg += ch; goto IN_UNQUOTED;
        case '"': goto IN_DQUOTE;
        case '\'': goto IN_SQUOTE;
        case '#': goto IN_COMMENT;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v': break;
        }
    }
    return out;

IN_COMMENT:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\r':
        case '\n': goto IN_GAP;
        }
    }
    return out;

IN_UNQUOTED:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\':
            if (cur < last) {
                ch = *cur++;
                if (ch == '\n')
                    break;
            }
            arg += ch;
            break;
        default: arg += ch; break;
        case '"': goto IN_DQUOTE;
        case '\'': goto IN_SQUOTE;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v':
            out.push_back(move(arg));
            arg.clear();
            goto IN_GAP;
        }
    }
    out.push_back(move(arg));
    return out;

IN_SQUOTE:
    while (cur < last) {
        char ch = *cur++;
        if (ch == '\'')
            goto IN_UNQUOTED;
        arg += ch;
    }
    out.push_back(move(arg));
    return out;

IN_DQUOTE:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '"': goto IN_UNQUOTED;
        case '\\':
            if (cur < last) {
                ch = *cur++;
                switch (ch) {
                case '$':
                case '\'':
                case '"':
                case '\\': break;
                case '\n': continue;
                default: arg += '\\';
                }
            }
            arg += ch;
            break;
        default: arg += ch; break;
        }
    }
    out.push_back(move(arg));
    return out;
}

//===========================================================================
// Rules from libiberty's buildargv().
//
// Arguments are split on whitespace (" \t\r\n\f\v") unless quoted or escaped
//  - backslashes: always escapes the following character.
//  - single quotes and double quotes: escape each other and whitespace.
//===========================================================================
// static
vector<string> Cli::toGnuArgv(string const & cmdline) {
    vector<string> out;
    char const * cur = cmdline.c_str();
    char const * last = cur + cmdline.size();

    string arg;
    char quote;

IN_GAP:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\':
            if (cur < last)
                ch = *cur++;
            arg += ch;
            goto IN_UNQUOTED;
        default: arg += ch; goto IN_UNQUOTED;
        case '\'':
        case '"': quote = ch; goto IN_QUOTED;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v': break;
        }
    }
    return out;

IN_UNQUOTED:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\':
            if (cur < last)
                ch = *cur++;
            arg += ch;
            break;
        default: arg += ch; break;
        case '"':
        case '\'': quote = ch; goto IN_QUOTED;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v':
            out.push_back(move(arg));
            arg.clear();
            goto IN_GAP;
        }
    }
    out.push_back(move(arg));
    return out;


IN_QUOTED:
    while (cur < last) {
        char ch = *cur++;
        if (ch == quote)
            goto IN_UNQUOTED;
        if (ch == '\\' && cur < last)
            ch = *cur++;
        arg += ch;
    }
    out.push_back(move(arg));
    return out;
}

//===========================================================================
// Rules defined in the "Parsing C++ Command-Line Arguments" article on MSDN.
//
// Arguments are split on whitespace (" \t") unless the whitespace is quoted.
// - double quotes: preserves whitespace that would otherwise end the
//   argument, can occur in the midst of an argument.
// - backslashes:
//   - an even number followed by a double quote adds one backslash for each
//     pair and the quote is a delimiter.
//   - an odd number followed by a double quote adds one backslash for each
//     pair, the last one is tossed, and the quote is added to the argument.
//   - any number not followed by a double quote are literals.
//===========================================================================
static void appendBackslashes(string & arg, int & backslashes) {
    if (backslashes) {
        arg.append(backslashes, '\\');
        backslashes = 0;
    }
}

//===========================================================================
// static
vector<string> Cli::toWindowsArgv(string const & cmdline) {
    vector<string> out;
    char const * cur = cmdline.c_str();
    char const * last = cur + cmdline.size();

    string arg;
    int backslashes = 0;

IN_GAP:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\': backslashes += 1; goto IN_UNQUOTED;
        case '"': goto IN_QUOTED;
        case ' ':
        case '\t':
        case '\r':
        case '\n': break;
        default: arg += ch; goto IN_UNQUOTED;
        }
    }
    return out;

IN_UNQUOTED:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\': backslashes += 1; break;
        case '"':
            if (int num = backslashes) {
                backslashes = 0;
                arg.append(num / 2, '\\');
                if (num % 2 == 1) {
                    arg += ch;
                    break;
                }
            }
            goto IN_QUOTED;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            appendBackslashes(arg, backslashes);
            out.push_back(move(arg));
            arg.clear();
            goto IN_GAP;
        default:
            appendBackslashes(arg, backslashes);
            arg += ch;
            break;
        }
    }
    appendBackslashes(arg, backslashes);
    out.push_back(move(arg));
    return out;

IN_QUOTED:
    while (cur < last) {
        char ch = *cur++;
        switch (ch) {
        case '\\': backslashes += 1; break;
        case '"':
            if (int num = backslashes) {
                backslashes = 0;
                arg.append(num / 2, '\\');
                if (num % 2 == 1) {
                    arg += ch;
                    break;
                }
            }
            goto IN_UNQUOTED;
        default:
            appendBackslashes(arg, backslashes);
            arg += ch;
            break;
        }
    }
    appendBackslashes(arg, backslashes);
    out.push_back(move(arg));
    return out;
}


/****************************************************************************
*
*   Join argv to form a command line
*
***/

//===========================================================================
// static
string Cli::toCmdline(vector<string> const & args) {
    auto ptrs = toPtrArgv(args);
    return toCmdline(ptrs.size(), ptrs.data());
}

//===========================================================================
// static
string Cli::toCmdline(size_t argc, char * argv[]) {
#if defined(_WIN32)
    return toWindowsCmdline(argc, argv);
#else
    return toGnuCmdline(argc, argv);
#endif
}

//===========================================================================
// static
string Cli::toCmdline(size_t argc, char const * argv[]) {
    return toCmdline(argc, (char **) argv);
}

//===========================================================================
// static
string Cli::toGlibCmdline(size_t, char * argv[]) {
    string out;
    if (!*argv)
        return out;
    for (;;) {
        for (auto ptr = *argv; *ptr; ++ptr) {
            switch (*ptr) {
            // Must escape
            case '|': case '&': case ';': case '<': case '>': case '(':
            case ')': case '$': case '`': case '\\': case '"': case '\'':
            case ' ': case '\t': case '\r': case '\n': case '\f': case '\v':
            // Should escape
            case '*': case '?': case '[': case '#': case '~': case '=':
            case '%':
                out += '\\';
            }
            out += *ptr;
        }
        if (!*++argv)
            return out;
        out += ' ';
    }
}

//===========================================================================
// static
string Cli::toGnuCmdline(size_t, char * argv[]) {
    string out;
    if (!*argv)
        return out;
    for (;;) {
        for (auto ptr = *argv; *ptr; ++ptr) {
            switch (*ptr) {
            case ' ': case '\t': case '\r': case '\n': case '\f': case '\v':
            case '\\': case '\'': case '"':
                out += '\\';
            }
            out += *ptr;
        }
        if (!*++argv)
            return out;
        out += ' ';
    }
}

//===========================================================================
// static
string Cli::toWindowsCmdline(size_t, char * argv[]) {
    string out;
    if (!*argv)
        return out;

    for (;;) {
        auto base = out.size();
        size_t backslashes = 0;
        auto ptr = *argv;
        for (; *ptr; ++ptr) {
            switch (*ptr) {
            case '\\': backslashes += 1; break;
            case ' ':
            case '\t': goto QUOTE;
            case '"':
                out.append(backslashes + 1, '\\');
                backslashes = 0;
                break;
            default: backslashes = 0; break;
            }
            out += *ptr;
        }
        goto NEXT;

    QUOTE:
        backslashes = 0;
        out.insert(base, 1, '"');
        out += *ptr++;
        for (; *ptr; ++ptr) {
            switch (*ptr) {
            case '\\': backslashes += 1; break;
            case '"':
                out.append(backslashes + 1, '\\');
                backslashes = 0;
                break;
            default: backslashes = 0; break;
            }
            out += *ptr;
        }
        out += '"';

    NEXT:
        if (!*++argv)
            return out;
        out += ' ';
    }
}


/****************************************************************************
*
*   Response files
*
***/

#ifdef DIMCLI_LIB_FILESYSTEM

// forward declarations
static bool expandResponseFiles(
    Cli & cli,
    vector<string> & args,
    vector<string> & ancestors
);

//===========================================================================
// Returns false on error, if there was an error content will either be empty
// or - if there was a transcoding error - contain the original content.
static bool loadFileUtf8(string & content, fs::path const & fn) {
    content.clear();

    error_code ec;
    auto bytes = (size_t) fs::file_size(fn, ec);
    if (ec) {
        // A file system race is required for this to fail (fs::exist success
        // immediately followed by fs::file_size failure) and there's no
        // practical way to cause it in a test. So give up and exclude this
        // line from the test coverage report.
        return false; // LCOV_EXCL_LINE
    }

    content.resize(bytes);
    ifstream f(fn, ios::binary);
    f.read(const_cast<char *>(content.data()), content.size());
    if (!f) {
        content.clear();
        return false;
    }
    f.close();

    if (content.size() < 2)
        return true;
    if (content[0] == '\xff' && content[1] == '\xfe') {
        wstring_convert<CodecvtWchar> wcvt("");
        auto base = reinterpret_cast<wchar_t const *>(content.data());
        auto tmp = (string) wcvt.to_bytes(
            base + 1,
            base + content.size() / sizeof(wchar_t)
        );
        if (tmp.empty())
            return false;
        content = tmp;
    } else if (content.size() >= 3
        && content[0] == '\xef'
        && content[1] == '\xbb'
        && content[2] == '\xbf'
    ) {
        content.erase(0, 3);
    }
    return true;
}

//===========================================================================
static bool expandResponseFile(
    Cli & cli,
    vector<string> & args,
    size_t & pos,
    vector<string> & ancestors
) {
    string content;
    error_code ec;
    auto fn = args[pos].substr(1);
    auto cfn = ancestors.empty()
        ? (fs::path) fn
        : fs::path(ancestors.back()).parent_path() / fn;
    cfn = fs::canonical(cfn, ec);
    if (ec || !fs::exists(cfn))
        return cli.badUsage("Invalid response file", fn);
    for (auto && a : ancestors) {
        if (a == cfn.string())
            return cli.badUsage("Recursive response file", fn);
    }
    ancestors.push_back(cfn.string());
    if (!loadFileUtf8(content, cfn)) {
        string desc = content.empty() ? "Read error" : "Invalid encoding";
        return cli.badUsage(desc, fn);
    }
    auto rargs = cli.toArgv(content);
    if (!expandResponseFiles(cli, rargs, ancestors))
        return false;
    auto rargLen = rargs.size();
    replace(args, pos, 1, move(rargs));
    pos += rargLen - 1;
    ancestors.pop_back();
    return true;
}

//===========================================================================
// "ancestors" contains the set of response files these args came from,
// directly or indirectly, and is used to detect recursive response files.
static bool expandResponseFiles(
    Cli & cli,
    vector<string> & args,
    vector<string> & ancestors
) {
    for (size_t pos = 0; pos < args.size(); ++pos) {
        if (!args[pos].empty() && args[pos][0] == '@') {
            if (!expandResponseFile(cli, args, pos, ancestors))
                return false;
        }
    }
    return true;
}

#endif


/****************************************************************************
*
*   Parse command line
*
***/

//===========================================================================
// static
vector<pair<string, double>> Cli::siUnitMapping(
    string const & symbol,
    int flags
) {
    vector<pair<string, double>> units({
        {"ki", double(1ull << 10)},
        {"Mi", double(1ull << 20)},
        {"Gi", double(1ull << 30)},
        {"Ti", double(1ull << 40)},
        {"Pi", double(1ull << 50)},
    });
    if (flags & fUnitBinaryPrefix) {
        units.insert(units.end(), {
            {"k", double(1ull << 10)},
            {"M", double(1ull << 20)},
            {"G", double(1ull << 30)},
            {"T", double(1ull << 40)},
            {"P", double(1ull << 50)},
        });
    } else {
        units.insert(units.end(), {
            {"k", 1e+3},
            {"M", 1e+6},
            {"G", 1e+9},
            {"T", 1e+12},
            {"P", 1e+15},
        });
        if (~flags & fUnitInsensitive) {
            units.insert(units.end(), {
                {"m", 1e-3},
                {"u", 1e-6},
                {"n", 1e-9},
                {"p", 1e-12},
                {"f", 1e-15},
            });
        }
    }
    if (!symbol.empty()) {
        if (flags & fUnitRequire) {
            for (auto && kv : units)
                kv.first += symbol;
        } else {
            units.reserve(2 * units.size());
            for (auto i = units.size(); i-- > 0;) {
                auto & kv = units[i];
                units.push_back({kv.first + symbol, kv.second});
            }
        }
        units.push_back({symbol, 1});
    }
    return units;
}

//===========================================================================
Cli & Cli::resetValues() {
    for (auto && opt : m_cfg->opts)
        opt->reset();
    m_cfg->exitCode = kExitOk;
    m_cfg->errMsg.clear();
    m_cfg->errDetail.clear();
    m_cfg->progName.clear();
    m_cfg->command.clear();
    return *this;
}

//===========================================================================
bool Cli::prompt(OptBase & opt, string const & msg, int flags) {
    if (!opt.from().empty())
        return true;
    auto & is = conin();
    auto & os = conout();
    if (msg.empty()) {
        os << opt.defaultPrompt();
    } else {
        os << msg;
    }
    bool defAdded = false;
    if (~flags & fPromptNoDefault) {
        if (opt.m_bool) {
            defAdded = true;
            bool def = false;
            if (!opt.m_flagValue) {
                auto & bopt = static_cast<Opt<bool>&>(opt);
                def = bopt.defaultValue();
            }
            os << (def ? " [Y/n]:" : " [y/N]:");
        } else {
            string tmp;
            if (opt.defaultValueToString(tmp) && !tmp.empty()) {
                defAdded = true;
                os << " [" << tmp << "]:";
            }
        }
    }
    if (!defAdded && msg.empty())
        os << ':';
    os << ' ';
    if (flags & fPromptHide)
        consoleEnableEcho(false); // disable if hide, must be re-enabled
    string val;
    os.flush();
    getline(is, val);
    if (flags & fPromptHide) {
        os << endl;
        if (~flags & fPromptConfirm)
            consoleEnableEcho(true); // re-enable when hide and !confirm
    }
    if (flags & fPromptConfirm) {
        string again;
        os << "Enter again to confirm: " << flush;
        getline(is, again);
        if (flags & fPromptHide) {
            os << endl;
            consoleEnableEcho(true); // re-enable when hide and confirm
        }
        if (val != again)
            return badUsage("Confirm failed, entries not the same.");
    }
    if (opt.m_bool) {
        // Honor the contract that bool parse functions are only presented
        // with either "0" or "1".
        val = val.size() && (val[0] == 'y' || val[0] == 'Y') ? "1" : "0";
    }
    return parseValue(opt, opt.defaultFrom(), 0, val.c_str());
}

//===========================================================================
bool Cli::parseValue(
    OptBase & opt,
    string const & name,
    size_t pos,
    char const ptr[]
) {
    if (!opt.assign(name, pos)) {
        string prefix = "Too many '" + name + "' values";
        string detail = "The maximum number of values is "
            + to_string(opt.m_nargs) + ".";
        return badUsage(prefix, ptr, detail);
    }
    string val;
    if (ptr) {
        val = ptr;
        if (!opt.doParseAction(*this, val))
            return false;
    } else {
        opt.unspecifiedValue();
    }
    return opt.doCheckAction(*this, val);
}

//===========================================================================
bool Cli::badUsage(
    string const & prefix,
    string const & value,
    string const & detail
) {
    string out;
    auto & cmd = commandMatched();
    if (cmd.size())
        out = "Command '" + cmd + "': ";
    out += prefix;
    if (!value.empty()) {
        out += ": ";
        out += value;
    }
    return fail(kExitUsage, out, detail);
}

//===========================================================================
bool Cli::badUsage(
    OptBase const & opt,
    string const & value,
    string const & detail
) {
    string prefix = "Invalid '" + opt.from() + "' value";
    return badUsage(prefix, value, detail);
}

//===========================================================================
bool Cli::fail(int code, string const & msg, string const & detail) {
    m_cfg->exitCode = code;
    m_cfg->errMsg = msg;
    m_cfg->errDetail = detail;
    return false;
}

//===========================================================================
bool Cli::parse(vector<string> & args) {
    // the 0th (name of this program) opt must always be present
    assert(!args.empty() && "at least one (program name) argument required");

    Config::touchAllCmds(*this);
    OptIndex ndx;
    ndx.index(*this, "", false);
    bool needCmd = m_cfg->cmds.size() > 1;

    // Commands can't be added when the top level command has a positional
    // argument, command processing requires that the first positional is
    // available to identify the command.
    assert((ndx.m_allowCommands || !needCmd)
        && "mixing top level positionals with commands");

    resetValues();

#if !defined(DIMCLI_LIB_NO_ENV)
    // insert environment options
    if (m_cfg->envOpts.size()) {
        if (char const * val = getenv(m_cfg->envOpts.c_str()))
            replace(args, 1, 0, toArgv(val));
    }
#endif

    // expand response files
#ifdef DIMCLI_LIB_FILESYSTEM
    vector<string> ancestors;
    if (m_cfg->responseFiles && !expandResponseFiles(*this, args, ancestors))
        return false;
#endif

    // before actions
    for (auto && fn : m_cfg->befores) {
        if (!fn(*this, args))
            return false;
    }

    // populate options
    auto arg = args.data();
    auto argc = args.size();

    string name;
    unsigned pos = 0;
    bool moreOpts = true;
    m_cfg->progName = *arg;
    size_t argPos = 1;
    arg += 1;

    for (; argPos < argc; ++argPos, ++arg) {
        OptName argName;
        char const * equal = nullptr;
        auto ptr = arg->c_str();
        if (*ptr == '-' && ptr[1] && moreOpts) {
            ptr += 1;
            for (; *ptr && *ptr != '-'; ++ptr) {
                auto it = ndx.m_shortNames.find(*ptr);
                name = '-';
                name += *ptr;
                if (it == ndx.m_shortNames.end())
                    return badUsage("Unknown option", name);
                argName = it->second;
                if (argName.opt->m_bool) {
                    if (!parseValue(
                        *argName.opt,
                        name,
                        argPos,
                        argName.invert ? "0" : "1"
                    )) {
                        return false;
                    }
                    continue;
                }
                ptr += 1;
                goto OPTION_VALUE;
            }
            if (!*ptr)
                continue;

            ptr += 1;
            if (!*ptr) {
                // bare "--" found, all remaining args are positional
                moreOpts = false;
                continue;
            }
            string key;
            equal = strchr(ptr, '=');
            if (equal) {
                key.assign(ptr, equal);
                ptr = equal + 1;
            } else {
                key = ptr;
                ptr = "";
            }
            auto it = ndx.m_longNames.find(key);
            name = "--";
            name += key;
            if (it == ndx.m_longNames.end())
                return badUsage("Unknown option", name);
            argName = it->second;
            if (argName.opt->m_bool) {
                if (equal)
                    return badUsage("Unknown option", name + "=");
                if (!parseValue(
                    *argName.opt,
                    name,
                    argPos,
                    argName.invert ? "0" : "1"
                )) {
                    return false;
                }
                continue;
            }
            goto OPTION_VALUE;
        }

        // positional value
        if (needCmd) {
            auto cmd = (string) ptr;
            if (!commandExists(cmd))
                return badUsage("Unknown command", cmd);
            needCmd = false;
            m_cfg->command = cmd;
            ndx.index(*this, cmd, false);
            continue;
        }

        if (pos >= ndx.m_argNames.size())
            return badUsage("Unexpected argument", ptr);
        argName = ndx.m_argNames[pos];
        name = argName.name;
        if (!parseValue(*argName.opt, name, argPos, ptr))
            return false;
        if (!argName.opt->m_vector)
            pos += 1;
        continue;

    OPTION_VALUE:
        if (*ptr || equal) {
            if (!parseValue(*argName.opt, name, argPos, ptr))
                return false;
            continue;
        }
        if (argName.optional) {
            if (!parseValue(*argName.opt, name, argPos, nullptr))
                return false;
            continue;
        }
        argPos += 1;
        arg += 1;
        if (argPos == argc)
            return badUsage("Option requires value", name);
        if (!parseValue(*argName.opt, name, argPos, arg->c_str()))
            return false;
    }

    if (!needCmd
        && pos < ndx.m_argNames.size()
        && !ndx.m_argNames[pos].optional
    ) {
        return badUsage("Missing argument", ndx.m_argNames[pos].name);
    }

    // after actions
    for (auto && opt : m_cfg->opts) {
        if (!opt->m_command.empty() && opt->m_command != commandMatched())
            continue;
        if (!opt->doAfterActions(*this))
            return false;
    }

    return true;
}

//===========================================================================
bool Cli::parse(ostream & os, vector<string> & args) {
    if (parse(args))
        return true;
    printError(os);
    return false;
}

//===========================================================================
bool Cli::parse(size_t argc, char * argv[]) {
    auto args = toArgv(argc, argv);
    return parse(args);
}

//===========================================================================
bool Cli::parse(ostream & os, size_t argc, char * argv[]) {
    auto args = toArgv(argc, argv);
    return parse(os, args);
}


/****************************************************************************
*
*   Parse results
*
***/

//===========================================================================
int Cli::exitCode() const {
    return m_cfg->exitCode;
};

//===========================================================================
string const & Cli::errMsg() const {
    return m_cfg->errMsg;
}

//===========================================================================
string const & Cli::errDetail() const {
    return m_cfg->errDetail;
}

//===========================================================================
string const & Cli::progName() const {
    return m_cfg->progName;
}

//===========================================================================
string const & Cli::commandMatched() const {
    return m_cfg->command;
}

//===========================================================================
bool Cli::exec() {
    auto & name = commandMatched();
    auto & cmd = m_cfg->cmds[name];
    if (!cmd.action) {
        // Most likely parse failed, was never run, or "this" was reset.
        assert(!"command found by parse no longer exists");
        return fail(
            Dim::kExitSoftware,
            "Subcommand found by parse no longer exists."
        );
    }
    if (!cmd.action(*this)) {
        if (exitCode())
            return false;

        // If the process should exit but there is still asynchronous work
        // going on, consider a custom "exit pending" exit code with special
        // handling in your main function to wait for it to complete.
        assert(!"command failed without setting exit code");
        return fail(
            Dim::kExitSoftware,
            "Subcommand failed without setting exit code."
        );
    }
    return true;
}

//===========================================================================
bool Cli::exec(ostream & os) {
    if (exec())
        return true;
    printError(os);
    return false;
}

//===========================================================================
bool Cli::exec(size_t argc, char * argv[]) {
    return parse(argc, argv) && exec();
}

//===========================================================================
bool Cli::exec(ostream & os, size_t argc, char * argv[]) {
    return parse(os, argc, argv) && exec(os);
}

//===========================================================================
bool Cli::exec(vector<string> & args) {
    return parse(args) && exec();
}

//===========================================================================
bool Cli::exec(ostream & os, vector<string> & args) {
    return parse(os, args) && exec(os);
}

//===========================================================================
bool Cli::commandExists(string const & name) const {
    auto & cmds = m_cfg->cmds;
    return cmds.find(name) != cmds.end();
}


/****************************************************************************
*
*   Help
*
***/

namespace {

struct WrapPos {
    size_t pos{0};
    size_t maxWidth{kMaxLineWidth};
    string prefix;
};

struct ChoiceKey {
    size_t pos;
    char const * key;
    char const * desc;
    char const * sortKey;
    bool def;
};

} // namespace

//===========================================================================
static void writeNewline(ostream & os, WrapPos & wp) {
    os << '\n' << wp.prefix;
    wp.pos = wp.prefix.size();
}

//===========================================================================
// Write token, potentially adding a line break first.
static void writeToken(ostream & os, WrapPos & wp, string const & token) {
    if (wp.pos + token.size() + 1 > wp.maxWidth) {
        if (wp.pos > wp.prefix.size()) {
            writeNewline(os, wp);
        }
    }
    if (wp.pos) {
        os << ' ';
        wp.pos += 1;
    }
    os << token;
    wp.pos += token.size();
}

//===========================================================================
// Write series of tokens, collapsing (and potentially breaking on) spaces.
static void writeText(ostream & os, WrapPos & wp, string const & text) {
    char const * base = text.c_str();
    for (;;) {
        while (*base == ' ')
            base += 1;
        if (!*base)
            return;
        auto nl = strchr(base, '\n');
        auto ptr = strchr(base, ' ');
        if (!ptr)
            ptr = text.c_str() + text.size();
        if (nl && nl < ptr) {
            writeToken(os, wp, string(base, nl));
            writeNewline(os, wp);
            base = nl + 1;
        } else {
            writeToken(os, wp, string(base, ptr));
            base = ptr;
        }
    }
}

//===========================================================================
// Like text, except advance to descCol first, and indent any additional
// required lines to descCol.
static void writeDescCol(
    ostream & os,
    WrapPos & wp,
    string const & text,
    size_t descCol
) {
    if (text.empty())
        return;
    if (wp.pos < descCol) {
        writeToken(os, wp, string(descCol - wp.pos - 1, ' '));
    } else if (wp.pos < descCol + 4) {
        os << ' ';
        wp.pos += 1;
    } else {
        wp.pos = wp.maxWidth;
    }
    wp.prefix.assign(descCol, ' ');
    writeText(os, wp, text);
}

//===========================================================================
string Cli::descStr(Cli::OptBase const & opt) const {
    string desc = opt.m_desc;
    string tmp;
    if (!opt.m_choiceDescs.empty()) {
        // "default" tag is added to individual choices later
    } else if (opt.m_flagValue && opt.m_flagDefault) {
        desc += " (default)";
    } else if (opt.m_vector) {
        if (opt.m_nargs != -1) {
            (void) opt.toString(tmp, opt.m_nargs);
            desc += " (limit: " + tmp + ")";
        }
    } else if (!opt.m_bool) {
        if (opt.m_defaultDesc.empty()) {
            if (!opt.defaultValueToString(tmp))
                tmp.clear();
        } else {
            tmp = opt.m_defaultDesc;
            if (!tmp[0])
                tmp.clear();
        }
        if (!tmp.empty())
            desc += " (default: " + tmp + ")";
    }
    return desc;
}

//===========================================================================
static void getChoiceKeys(
    vector<ChoiceKey> & keys,
    size_t & maxWidth,
    unordered_map<string, Cli::OptBase::ChoiceDesc> const & choices
) {
    keys.clear();
    maxWidth = 0;
    for (auto && cd : choices) {
        maxWidth = max(maxWidth, cd.first.size());
        ChoiceKey key;
        key.pos = cd.second.pos;
        key.key = cd.first.c_str();
        key.desc = cd.second.desc.c_str();
        key.sortKey = cd.second.sortKey.c_str();
        key.def = cd.second.def;
        keys.push_back(key);
    }
    sort(keys.begin(), keys.end(), [](auto & a, auto & b) {
        if (int rc = strcmp(a.sortKey, b.sortKey))
            return rc < 0;
        return a.pos < b.pos;
    });
}

//===========================================================================
static void writeChoices(
    ostream & os,
    WrapPos & wp,
    unordered_map<string, Cli::OptBase::ChoiceDesc> const & choices
) {
    if (choices.empty())
        return;
    size_t colWidth = 0;
    vector<ChoiceKey> keys;
    getChoiceKeys(keys, colWidth, choices);
    size_t const indent = 6;
    colWidth = max(min(colWidth + indent + 1, kMaxDescCol), kMinDescCol);

    string desc;
    for (auto && k : keys) {
        wp.prefix.assign(indent + 2, ' ');
        writeToken(os, wp, string(indent, ' ') + k.key);
        desc = k.desc;
        if (k.def)
            desc += " (default)";
        writeDescCol(os, wp, desc, colWidth);
        os << '\n';
        wp.pos = 0;
    }
}

//===========================================================================
static void printChoices(
    ostream & os,
    unordered_map<string, Cli::OptBase::ChoiceDesc> const & choices
) {
    if (choices.empty())
        return;
    WrapPos wp;
    writeText(os, wp, "Must be ");
    size_t colWidth = 0;
    vector<ChoiceKey> keys;
    getChoiceKeys(keys, colWidth, choices);

    string val;
    size_t pos = 0;
    auto num = keys.size();
    for (auto i = keys.begin(); pos < num; ++pos, ++i) {
        val = '"';
        val += i->key;
        val += '"';
        if (pos == 0 && num == 2) {
            writeToken(os, wp, val);
            writeToken(os, wp, "or");
        } else if (pos + 1 == num) {
            val += '.';
            writeToken(os, wp, val);
        } else {
            val += ',';
            writeToken(os, wp, val);
            if (pos + 2 == num)
                writeToken(os, wp, "or");
        }
    }
}

//===========================================================================
int Cli::printHelp(
    ostream & os,
    string const & progName,
    string const & cmdName
) {
    auto & cmd = Config::findCmdAlways(*this, cmdName);
    auto & top = Config::findCmdAlways(*this, "");
    auto & hdr = cmd.header.empty() ? top.header : cmd.header;
    if (*hdr.data()) {
        WrapPos wp;
        writeText(os, wp, hdr);
        writeNewline(os, wp);
    }
    printUsage(os, progName, cmdName);
    if (!cmd.desc.empty()) {
        WrapPos wp;
        writeNewline(os, wp);
        writeText(os, wp, cmd.desc);
        writeNewline(os, wp);
    }
    if (cmdName.empty())
        printCommands(os);
    printPositionals(os, cmdName);
    printOptions(os, cmdName);
    auto & ftr = cmd.footer.empty() ? top.footer : cmd.footer;
    if (*ftr.data()) {
        WrapPos wp;
        writeNewline(os, wp);
        writeText(os, wp, ftr);
    }
    return exitCode();
}

//===========================================================================
int Cli::writeUsageImpl(
    ostream & os,
    string const & arg0,
    string const & cmdName,
    bool expandedOptions
) {
    OptIndex ndx;
    ndx.index(*this, cmdName, true);
    auto & cmd = Config::findCmdAlways(*this, cmdName);
    auto prog = displayName(arg0.empty() ? progName() : arg0);
    string const usageStr{"Usage: "};
    os << usageStr << prog;
    WrapPos wp;
    wp.pos = prog.size() + usageStr.size();
    wp.prefix = string(wp.pos, ' ');
    if (cmdName.size())
        writeToken(os, wp, cmdName);
    if (!ndx.m_shortNames.empty() || !ndx.m_longNames.empty()) {
        if (!expandedOptions) {
            writeToken(os, wp, "[OPTIONS]");
        } else {
            size_t colWidth = 0;
            vector<ArgKey> namedArgs;
            ndx.findNamedArgs(
                namedArgs,
                colWidth,
                *this,
                cmd,
                kNameNonDefault,
                true
            );
            for (auto && key : namedArgs)
                writeToken(os, wp, "[" + key.list + "]");
        }
    }
    if (cmdName.empty() && m_cfg->cmds.size() > 1) {
        writeToken(os, wp, "command");
        writeToken(os, wp, "[args...]");
    } else {
        for (auto && pa : ndx.m_argNames) {
            string token = pa.name.find(' ') == string::npos
                ? pa.name
                : "<" + pa.name + ">";
            if (pa.opt->m_vector)
                token += "...";
            if (pa.optional) {
                writeToken(os, wp, "[" + token + "]");
            } else {
                writeToken(os, wp, token);
            }
        }
    }
    os << '\n';
    return exitCode();
}

//===========================================================================
int Cli::printUsage(
    ostream & os,
    string const & arg0,
    string const & cmd
) {
    return writeUsageImpl(os, arg0, cmd, false);
}

//===========================================================================
int Cli::printUsageEx(
    ostream & os,
    string const & arg0,
    string const & cmd
) {
    return writeUsageImpl(os, arg0, cmd, true);
}

//===========================================================================
static size_t clampDescWidth(size_t colWidth) {
    if (colWidth < kMinDescCol)
        return kMinDescCol;
    if (colWidth > kMaxDescCol)
        return kMaxDescCol;
    return colWidth;
}

//===========================================================================
void Cli::printPositionals(ostream & os, string const & cmd) {
    OptIndex ndx;
    ndx.index(*this, cmd, true);
    size_t colWidth = 0;
    bool hasDesc = false;
    for (auto && pa : ndx.m_argNames) {
        // find widest positional argument name
        colWidth = max(colWidth, pa.name.size());
        hasDesc = hasDesc || pa.opt->m_desc.size();
    }
    colWidth = clampDescWidth(colWidth + 3);
    if (!hasDesc)
        return;

    WrapPos wp;
    for (auto && pa : ndx.m_argNames) {
        wp.prefix.assign(4, ' ');
        writeToken(os, wp, "  " + pa.name);
        writeDescCol(os, wp, descStr(*pa.opt), colWidth);
        os << '\n';
        wp.pos = 0;
        writeChoices(os, wp, pa.opt->m_choiceDescs);
    }
}

//===========================================================================
void Cli::printOptions(ostream & os, string const & cmdName) {
    OptIndex ndx;
    ndx.index(*this, cmdName, true);
    auto & cmd = Config::findCmdAlways(*this, cmdName);

    // find named args and the longest name list
    size_t colWidth = 0;
    vector<ArgKey> namedArgs;
    if (!ndx.findNamedArgs(namedArgs, colWidth, *this, cmd, kNameAll, false))
        return;
    colWidth = clampDescWidth(colWidth + 3);

    WrapPos wp;
    char const * gname = nullptr;
    for (auto && key : namedArgs) {
        if (!gname || key.opt->m_group != gname) {
            gname = key.opt->m_group.c_str();
            writeNewline(os, wp);
            auto & grp = Config::findGrpAlways(cmd, key.opt->m_group);
            auto title = grp.title;
            if (title.empty()
                && strcmp(gname, kInternalOptionGroup) == 0
                && &key == namedArgs.data()
            ) {
                // First group and it's the internal group, give it a title
                // so it's not just left hanging.
                title = "Options";
            }
            if (!title.empty()) {
                writeText(os, wp, title + ":");
                writeNewline(os, wp);
            }
        }
        wp.prefix.assign(4, ' ');
        os << ' ';
        wp.pos = 1;
        writeText(os, wp, key.list);
        writeDescCol(os, wp, descStr(*key.opt), colWidth);
        wp.prefix.clear();
        writeNewline(os, wp);
        writeChoices(os, wp, key.opt->m_choiceDescs);
        wp.prefix.clear();
    }
}

//===========================================================================
void Cli::printCommands(ostream & os) {
    Config::touchAllCmds(*this);

    size_t colWidth = 0;
    struct CmdKey {
        char const * name;
        CommandConfig const * cmd;
        GroupConfig const * grp;
    };
    vector<CmdKey> keys;
    for (auto && cmd : m_cfg->cmds) {
        if (auto width = cmd.first.size()) {
            colWidth = max(colWidth, width);
            CmdKey key = {
                cmd.first.c_str(),
                &cmd.second,
                &m_cfg->cmdGroups[cmd.second.cmdGroup]
            };
            keys.push_back(key);
        }
    }
    if (keys.empty())
        return;
    colWidth = max(min(colWidth + 3, kMaxDescCol), kMinDescCol);
    sort(keys.begin(), keys.end(), [](auto & a, auto & b) {
        if (int rc = a.grp->sortKey.compare(b.grp->sortKey))
            return rc < 0;
        return strcmp(a.name, b.name) < 0;
    });

    WrapPos wp;
    char const * gname = nullptr;
    for (auto && key : keys) {
        if (!gname || key.grp->name != gname) {
            gname = key.grp->name.c_str();
            writeNewline(os, wp);
            auto title = key.grp->title;
            if (title.empty()
                && strcmp(gname, kInternalOptionGroup) == 0
                && &key == keys.data()
            ) {
                // First group and it's the internal group, give it a title.
                title = "Commands";
            }
            if (!title.empty()) {
                writeText(os, wp, title + ":");
                writeNewline(os, wp);
            }
        }

        wp.prefix.assign(4, ' ');
        writeToken(os, wp, "  "s + key.name);
        auto desc = key.cmd->desc;
        size_t pos = 0;
        for (;;) {
            pos = desc.find_first_of(".!?", pos);
            if (pos == string::npos)
                break;
            pos += 1;
            if (desc[pos] == ' ') {
                desc.resize(pos);
                break;
            }
        }
        desc = trim(desc);
        writeDescCol(os, wp, desc, colWidth);
        wp.prefix.clear();
        writeNewline(os, wp);
    }
}

//===========================================================================
int Cli::printError(ostream & os) {
    auto code = exitCode();
    if (code) {
        os << "Error: " << errMsg() << endl;
        auto & detail = errDetail();
        if (detail.size())
            os << detail << endl;
    }
    return code;
}


/****************************************************************************
*
*   Native console API
*
***/

#if defined(DIMCLI_LIB_NO_CONSOLE)

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    assert(enable && "disabling echo requires console support enabled");
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    auto hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hInput, &mode);
    if (enable) {
        mode |= ENABLE_ECHO_INPUT;
    } else {
        mode &= ~ENABLE_ECHO_INPUT;
    }
    SetConsoleMode(hInput, mode);
}

#else

#include <termios.h>
#include <unistd.h>

//===========================================================================
void Cli::consoleEnableEcho(bool enable) {
    termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (enable) {
        tty.c_lflag |= ECHO;
    } else {
        tty.c_lflag &= ~ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

#endif
