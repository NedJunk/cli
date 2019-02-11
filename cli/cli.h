/*******************************************************************************
 * CLI - A simple command line interface.
 * Copyright (C) 2016 Daniele Pallastrelli
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#ifndef CLI_H_
#define CLI_H_

#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "colorprofile.h"

namespace cli
{

    // ********************************************************************

    template < typename T > struct TypeDesc {};
    template <> struct TypeDesc< char > { static const char* Name() { return "<char>"; } };
    template <> struct TypeDesc< unsigned char > { static const char* Name() { return "<unsigned char>"; } };
    template <> struct TypeDesc< short > { static const char* Name() { return "<short>"; } };
    template <> struct TypeDesc< unsigned short > { static const char* Name() { return "<unsigned short>"; } };
    template <> struct TypeDesc< int > { static const char* Name() { return "<int>"; } };
    template <> struct TypeDesc< unsigned int > { static const char* Name() { return "<unsigned int>"; } };
    template <> struct TypeDesc< long > { static const char* Name() { return "<long>"; } };
    template <> struct TypeDesc< unsigned long > { static const char* Name() { return "<unsigned long>"; } };
    template <> struct TypeDesc< float > { static const char* Name() { return "<float>"; } };
    template <> struct TypeDesc< double > { static const char* Name() { return "<double>"; } };
    template <> struct TypeDesc< long double > { static const char* Name() { return "<long double>"; } };
    template <> struct TypeDesc< bool > { static const char* Name() { return "<bool>"; } };
    template <> struct TypeDesc< std::string > { static const char* Name() { return "<string>"; } };

    // ********************************************************************

    class History
    {
    public:

        using Item = std::vector<std::string>;

        explicit History(std::size_t size) : maxSize(size) {}

        template <typename T>
        void Add(T&& item)
        {
            buffer.push_front(std::forward<T>(item));
            if (buffer.size() > maxSize) buffer.pop_back();
        }

        void Show(std::ostream& out) const
        {
            out << '\n';
            for ( auto& item: buffer )
                out << ToString(item) << '\n';
            out << '\n' << std::flush;
        }

        void ResetCurrent()
        {
            currentIndex = 0;
        }

        void ToPreviousEntry()
        {
            if ( currentIndex == buffer.size()-1 )
                currentIndex = 0;
            else
                ++currentIndex;
        }

        void ToNextEntry()
        {
            if ( currentIndex == 0 )
                currentIndex = buffer.size()-1;
            else
                --currentIndex;
        }

        std::string GetCurrent() const
        {
            if (buffer.empty()) return std::string();
            return ToString( buffer[ currentIndex ] );
        }

    private:

        static std::string ToString(const Item& item)
        {
            std::string result;
            std::for_each(item.begin(), item.end(), [&](const std::string &piece){ result += piece + ' '; });
            return result;
        }

        using Buffer = std::deque<Item>;
        Buffer buffer;
        const std::size_t maxSize;
        std::size_t currentIndex = 0; // 0 = last entry, buffer.size()-1 = oldest entry
    };

    // ********************************************************************

    // forward declarations
    class Menu;
    class CliSession;


    class Cli
    {

        // inner class to provide a global output stream
        class OutStream
        {
        public:
            template <typename T>
            OutStream& operator << (const T& msg)
            {
                for (auto out: ostreams)
                    *out << msg;
                return *this;
            }

            // this is the type of std::cout
            typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
            // this is the function signature of std::endl
            typedef CoutType& (*StandardEndLine)(CoutType&);

            // takes << std::endl
            OutStream& operator << (StandardEndLine manip)
            {
                for (auto out: ostreams)
                    manip(*out);
                return *this;
            }

        private:
            friend class Cli;

            void Register(std::ostream& o)
            {
                ostreams.push_back(&o);
            }
            void UnRegister(std::ostream& o)
            {
                ostreams.erase(std::remove(ostreams.begin(), ostreams.end(), &o), ostreams.end());
            }

            std::vector<std::ostream*> ostreams;
        };
        // end inner class

    public:
        Cli(
            std::unique_ptr< Menu >&& rootMenu,
            std::function< void(std::ostream&) > exitAction = std::function< void(std::ostream&) >()
        );

        // disable value semantics
        Cli( const Cli& ) = delete;
        Cli& operator = ( const Cli& ) = delete;

        void ExitAction( std::function< void(std::ostream&)> action );
        Menu* RootMenu() { return rootMenu.get(); }
        void ExitAction( std::ostream& out ) { if ( exitAction ) exitAction( out ); }

        static void Register(std::ostream& o) { cout().Register(o); }
        static void UnRegister(std::ostream& o) { cout().UnRegister(o); }

        static OutStream& cout() { static OutStream s; return s; }

    private:
        std::unique_ptr< Menu > rootMenu; // just to keep it alive
        std::function< void(std::ostream&) > exitAction;
    };

    // ********************************************************************

    class Command
    {
    public:
        explicit Command(const std::string& _name) : name(_name) {}
        virtual ~Command() = default;
        virtual bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) = 0;
        virtual void Help( std::ostream& out ) const = 0;
        // Returns the collection of completions relatives to this command.
        // For simple commands, provides a base implementation that use the name of the command
        // for aggregate commands (i.e., Menu), the function is redefined to give the menu command
        // and the subcommand recursively
        virtual std::vector<std::string> GetCompletionRecursive(const std::string& line) const
        {
            if ( boost::algorithm::starts_with(name, line) ) return {name};
            else return {};
        }
    protected:
        const std::string& Name() const { return name; }
    private:
        const std::string name;
    };

    // ********************************************************************

    // free utility function to get completions from a list of commands and the current line
    inline std::vector<std::string> GetCompletions(const std::vector< std::unique_ptr< Command > >& cmds, const std::string& currentLine)
    {
        std::vector<std::string> result;
        std::for_each( cmds.begin(), cmds.end(),
            [&currentLine,&result](auto& cmd)
            {
                auto c = cmd->GetCompletionRecursive(currentLine);
                result.insert(result.end(), std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()));
            }
        );
        return result;
    }

    // ********************************************************************

    class CliSession
    {
    public:
        CliSession( Cli& _cli, std::ostream& _out, std::size_t historySize = 100 );
        ~CliSession() { cli.UnRegister(out); }

        // disable value semantics
        CliSession( const CliSession& ) = delete;
        CliSession& operator = ( const CliSession& ) = delete;

        void Feed( const std::string& cmd );

        void Prompt();

        void Current( Menu* menu )
        {
            current = menu;
        }

        std::ostream& OutStream() { return out; }

        void Help() const;

        void Exit()
        {
            if (exitAction) exitAction(out);
            cli.ExitAction(out);
        }

        void ExitAction( std::function< void(std::ostream&)> action )
        {
            exitAction = action;
        }

        void ShowHistory() const { history.Show(out); }

        std::string PreviousCmd()
        {
            auto result = history.GetCurrent();
            history.ToPreviousEntry();
            return result;
        }

        std::string NextCmd() {
            auto result = history.GetCurrent();
            history.ToNextEntry();
            return result;
        }

        std::vector<std::string> GetCompletions(const std::string& currentLine) const;

    private:

        Cli& cli;
        Menu* current;
        std::unique_ptr< Menu > globalScopeMenu;
        std::ostream& out;
        std::function< void(std::ostream&)> exitAction;
        History history;
    };

    // ********************************************************************

    class Menu : public Command
    {
    public:
        // disable value semantics
        Menu( const Menu& ) = delete;
        Menu& operator = ( const Menu& ) = delete;

        Menu() : Command( {} ), parent( nullptr ), description() {}

        Menu( const std::string& _name, const std::string& desc = "(menu)" ) :
            Command( _name ), parent( nullptr ), description( desc )
        {}

        template < typename F >
        void Add( const std::string& name, F f, const std::string& help = "" )
        {
            // dispatch to private Add methods
            Add( name, help, f, &F::operator() );
        }

        void Add( std::unique_ptr< Command >&& cmd )
        {
            cmds.push_back( std::move(cmd) );
        }

        void Add( std::unique_ptr< Menu >&& menu )
        {
            menu -> parent = this;
            cmds.push_back( std::move(menu) );
        }

        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine[ 0 ] == Name() )
            {
                if ( cmdLine.size() == 1 )
                {
                    session.Current( this );
                    return true;
                }
                else
                {
                    // check also for subcommands
                    std::vector<std::string > subCmdLine( cmdLine.begin()+1, cmdLine.end() );
                    for ( auto& cmd: cmds )
                        if ( cmd -> Exec( subCmdLine, session ) ) return true;
                }
            }
            return false;
        }

        bool ScanCmds( const std::vector< std::string >& cmdLine, CliSession& session )
        {
            for ( auto& cmd: cmds )
                if ( cmd -> Exec( cmdLine, session ) ) return true;
            if ( parent && parent -> Exec( cmdLine, session ) ) return true;
            return false;
        }

        std::string Prompt() const
        {
            return Name();
        }

        void MainHelp( std::ostream& out )
        {
            for ( auto& cmd: cmds )
                cmd -> Help( out );
            if ( parent ) parent -> Help( out );
        }

        void Help( std::ostream& out ) const override
        {
            out << " - " << Name() << "\n\t" << description << "\n";
        }

        std::vector<std::string> GetCompletions(const std::string& currentLine) const
        {
            auto result = cli::GetCompletions(cmds, currentLine);
			if (parent)
			{
				auto c = parent->GetCompletionRecursive(currentLine);
				result.insert( result.end(), std::make_move_iterator(c.begin()), std::make_move_iterator(c.end()));
			}
			return result;
        }

        virtual std::vector<std::string> GetCompletionRecursive(const std::string& line) const override
        {
            if ( boost::algorithm::starts_with( line, Name() ) )
            {
                auto rest = line;
                rest.erase( 0, Name().size() );
                boost::algorithm::trim_left(rest);
                std::vector<std::string> result;
                for ( auto& cmd: cmds )
                {
                    auto cs = cmd->GetCompletionRecursive( rest );
                    for ( auto& c: cs )
                        result.push_back( Name() + ' ' + c );
                }
                return result;
            }
            return Command::GetCompletionRecursive(line);
        }

    private:

        template < typename F, typename R >
        void Add( const std::string& name, const std::string& help, F& f,R (F::*mf)(std::ostream& out) const );

        template < typename F, typename R, typename A1 >
        void Add( const std::string& name, const std::string& help, F& f,R (F::*mf)(A1, std::ostream& out) const );

        template < typename F, typename R, typename A1, typename A2 >
        void Add( const std::string& name, const std::string& help, F& f,R (F::*mf)(A1, A2, std::ostream& out) const );

        template < typename F, typename R, typename A1, typename A2, typename A3 >
        void Add( const std::string& name, const std::string& help, F& f,R (F::*mf)(A1, A2, A3, std::ostream& out) const );

        template < typename F, typename R, typename A1, typename A2, typename A3, typename A4 >
        void Add( const std::string& name, const std::string& help, F& f,R (F::*mf)(A1, A2, A3, A4, std::ostream& out) const );

        Menu* parent;
        const std::string description;
        using Cmds = std::vector< std::unique_ptr< Command > >;
        Cmds cmds;
    };

    // ********************************************************************

    class FuncCmd : public Command
    {
    public:
        // disable value semantics
        FuncCmd( const FuncCmd& ) = delete;
        FuncCmd& operator = ( const FuncCmd& ) = delete;

        FuncCmd(
            const std::string& _name,
            std::function< void( std::ostream& )> _function,
            const std::string& desc = ""
        ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 1 ) return false;
            if ( cmdLine[ 0 ] == Name() )
            {
                function( session.OutStream() );
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name() << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( std::ostream& )> function;
        const std::string description;
    };

    template < typename T >
    class FuncCmd1 : public Command
    {
    public:
        // disable value semantics
        FuncCmd1( const FuncCmd1& ) = delete;
        FuncCmd1& operator = ( const FuncCmd1& ) = delete;

        FuncCmd1(
            const std::string& _name,
            std::function< void( T, std::ostream& ) > _function,
            const std::string& desc = ""
            ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 2 ) return false;
            if ( Name() == cmdLine[ 0 ] )
            {
                try
                {
                    T arg = boost::lexical_cast<T>( cmdLine[ 1 ] );
                    function( arg, session.OutStream() );
                }
                catch ( boost::bad_lexical_cast & )
                {
                    return false;
                }
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name()
                << " " << TypeDesc< T >::Name()
                << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( T, std::ostream& )> function;
        const std::string description;
    };

    template < typename T1, typename T2 >
    class FuncCmd2 : public Command
    {
    public:
        // disable value semantics
        FuncCmd2( const FuncCmd2& ) = delete;
        FuncCmd2& operator = ( const FuncCmd2& ) = delete;

        FuncCmd2(
            const std::string& _name,
            std::function< void( T1, T2, std::ostream& ) > _function,
            const std::string& desc = "2 parameter command"
            ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 3 ) return false;
            if ( Name() == cmdLine[ 0 ] )
            {
                try
                {
                    T1 arg1 = boost::lexical_cast<T1>( cmdLine[ 1 ] );
                    T2 arg2 = boost::lexical_cast<T2>( cmdLine[ 2 ] );
                    function( arg1, arg2, session.OutStream() );
                }
                catch ( boost::bad_lexical_cast & )
                {
                    return false;
                }
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name()
                << " " << TypeDesc< T1 >::Name()
                << " " << TypeDesc< T2 >::Name()
                << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( T1, T2, std::ostream& )> function;
        const std::string description;
    };

    template < typename T1, typename T2, typename T3 >
    class FuncCmd3 : public Command
    {
    public:
        // disable value semantics
        FuncCmd3( const FuncCmd3& ) = delete;
        FuncCmd3& operator = ( const FuncCmd3& ) = delete;

        FuncCmd3(
            const std::string& _name,
            std::function< void( T1, T2, T3, std::ostream& ) > _function,
            const std::string& desc = "3 parameters command"
            ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 4 ) return false;
            if ( Name() == cmdLine[ 0 ] )
            {
                try
                {
                    T1 arg1 = boost::lexical_cast<T1>( cmdLine[ 1 ] );
                    T2 arg2 = boost::lexical_cast<T2>( cmdLine[ 2 ] );
                    T3 arg3 = boost::lexical_cast<T3>( cmdLine[ 3 ] );
                    function( arg1, arg2, arg3, session.OutStream() );
                }
                catch ( boost::bad_lexical_cast & )
                {
                    return false;
                }
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name()
                << " " << TypeDesc< T1 >::Name()
                << " " << TypeDesc< T2 >::Name()
                << " " << TypeDesc< T3 >::Name()
                << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( T1, T2, T3, std::ostream& )> function;
        const std::string description;
    };

    template < typename T1, typename T2, typename T3, typename T4 >
    class FuncCmd4 : public Command
    {
    public:
        // disable value semantics
        FuncCmd4( const FuncCmd4& ) = delete;
        FuncCmd4& operator = ( const FuncCmd4& ) = delete;

        FuncCmd4(
            const std::string& _name,
            std::function< void( T1, T2, T3, T4, std::ostream& ) > _function,
            const std::string& desc = "4 parameters command"
            ) : Command( _name ), function( _function ), description( desc )
        {
        }
        bool Exec( const std::vector< std::string >& cmdLine, CliSession& session ) override
        {
            if ( cmdLine.size() != 5 ) return false;
            if ( Name() == cmdLine[ 0 ] )
            {
                try
                {
                    T1 arg1 = boost::lexical_cast<T1>( cmdLine[ 1 ] );
                    T2 arg2 = boost::lexical_cast<T2>( cmdLine[ 2 ] );
                    T3 arg3 = boost::lexical_cast<T3>( cmdLine[ 3 ] );
                    T4 arg4 = boost::lexical_cast<T4>( cmdLine[ 4 ] );
                    function( arg1, arg2, arg3, arg4, session.OutStream() );
                }
                catch ( boost::bad_lexical_cast & )
                {
                    return false;
                }
                return true;
            }

            return false;
        }
        void Help( std::ostream& out ) const override
        {
            out << " - " << Name()
                << " " << TypeDesc< T1 >::Name()
                << " " << TypeDesc< T2 >::Name()
                << " " << TypeDesc< T3 >::Name()
                << " " << TypeDesc< T4 >::Name()
                << "\n\t" << description << "\n";
        }
    private:
        const std::function< void( T1, T2, T3, T4, std::ostream& )> function;
        const std::string description;
    };

    // ********************************************************************

    // Cli implementation

    inline Cli::Cli( std::unique_ptr< Menu >&& _rootMenu, std::function< void( std::ostream& )> _exitAction ) :
        rootMenu( std::move(_rootMenu) ),
        exitAction( _exitAction )
    {
    }

    inline void Cli::ExitAction( std::function< void(std::ostream&)> action )
    {
        exitAction = action;
    }

    // CliSession implementation

    inline CliSession::CliSession(Cli& _cli, std::ostream& _out, std::size_t historySize) :
            cli(_cli),
            current(cli.RootMenu()),
            globalScopeMenu(std::make_unique< Menu >()),
            out(_out),
            history(historySize)
        {
            cli.Register(out);
            globalScopeMenu->Add(
                "help",
                [this](std::ostream&){ Help(); },
                "This help message"
            );
            globalScopeMenu->Add(
                "exit",
                [this](std::ostream&){ Exit(); },
                "Quit the session"
            );
#ifdef CLI_HISTORY_CMD
            globalScopeMenu->Add(
                "history",
                [this](std::ostream&){ ShowHistory(); },
                "Show the history"
            );
#endif
        }

    inline void CliSession::Feed( const std::string& cmd )
    {
        history.ResetCurrent(); // TODO here or in the caller?

        std::vector< std::string > strs;
        boost::split( strs, cmd, boost::is_any_of( " \t\n" ), boost::token_compress_on );
        // remove null entries from the vector:
        strs.erase(
            std::remove_if(
                strs.begin(),
                strs.end(),
                [](const std::string& s){ return s.empty(); }
            ),
            strs.end()
        );
        if ( strs.empty() ) return; // just hit enter

        // global cmds check
        bool found = globalScopeMenu->ScanCmds(strs, *this);

        // root menu recursive cmds check
        if ( !found ) found = current -> ScanCmds( strs, *this );

        if ( found ) // insert into history
            history.Add( std::move(strs) ); // last use of strs
        else // error msg if not found
            out << "Command unknown: " << cmd << "\n";

        return;
    }

    inline void CliSession::Prompt()
    {
        out << beforePrompt
            << current -> Prompt()
            << afterPrompt
            << "> "
            << std::flush;
    }

    inline void CliSession::Help() const
    {
        out << "Commands available:\n";
        globalScopeMenu->MainHelp(out);
        current -> MainHelp( out );
    }

    inline std::vector<std::string> CliSession::GetCompletions( const std::string& currentLine ) const
    {
        auto v1 = globalScopeMenu->GetCompletions(currentLine);
        auto v3 = current -> GetCompletions(currentLine);
        v1.insert( v1.end(), std::make_move_iterator(v3.begin()), std::make_move_iterator(v3.end()) );
        return v1;
    }

    // Menu implementation

    template < typename F, typename R >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(std::ostream& out) const )
    {
        cmds.push_back( std::make_unique< FuncCmd >( name, f, help ) );
    }

    template < typename F, typename R, typename A1 >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(A1, std::ostream& out) const )
    {
        cmds.push_back( std::make_unique< FuncCmd1< A1 > >( name, f, help ) );
    }

    template < typename F, typename R, typename A1, typename A2 >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(A1, A2, std::ostream& out) const )
    {
        cmds.push_back( std::make_unique< FuncCmd2< A1, A2 > >( name, f, help ) );
    }

    template < typename F, typename R, typename A1, typename A2, typename A3 >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(A1, A2, A3, std::ostream& out) const )
    {
        cmds.push_back( std::make_unique< FuncCmd3< A1, A2, A3 > >( name, f, help ) );
    }

    template < typename F, typename R, typename A1, typename A2, typename A3, typename A4 >
    void Menu::Add( const std::string& name, const std::string& help, F& f,R (F::*)(A1, A2, A3, A4, std::ostream& out) const )
    {
        cmds.push_back( std::make_unique< FuncCmd4< A1, A2, A3, A4> >( name, f, help ) );
    }

} // namespace

#endif
