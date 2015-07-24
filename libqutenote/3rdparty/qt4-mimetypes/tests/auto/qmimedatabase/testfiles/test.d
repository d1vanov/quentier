#!/usr/bin/dmd -run
/* sh style script syntax is supported */

/* Hello World in D
   To compile:
     dmd hello.d
   or to optimize:
     dmd -O -inline -release hello.d
*/

import std.stdio;

void main(string[] args)
{
    writefln("Hello World, Reloaded");

    // auto type inference and built-in foreach
    foreach (argc, argv; args)
    {
        // Object Oriented Programming
        auto cl = new CmdLin(argc, argv);
        // Improved typesafe printf
        writeln(cl.argnum, cl.suffix, " arg: ", cl.argv);
        // Automatic or explicit memory management
        delete cl;
    }

    // Nested structs and classes
    struct specs
    {
        // all members automatically initialized
        int count, allocated;
    }

    // Nested functions can refer to outer
    // variables like args
    specs argspecs()
    {
        specs* s = new specs;
        // no need for '->'
        s.count = args.length;		   // get length of array with .length
        s.allocated = typeof(args).sizeof; // built-in native type properties
        foreach (argv; args)
            s.allocated += argv.length * typeof(argv[0]).sizeof;
        return *s;
    }

    // built-in string and common string operations
    writefln("argc = %d, " ~ "allocated = %d",
	argspecs().count, argspecs().allocated);
}

class CmdLin
{
    private int _argc;
    private string _argv;

public:
    this(int argc, string argv)	// constructor
    {
        _argc = argc;
        _argv = argv;
    }

    int argnum()
    {
        return _argc + 1;
    }

    string argv()
    {
        return _argv;
    }

    string suffix()
    {
        string suffix = "th";
        switch (_argc)
        {
          case 0:
            suffix = "st";
            break;
          case 1:
            suffix = "nd";
            break;
          case 2:
            suffix = "rd";
            break;
          default:
	    break;
        }
        return suffix;
    }
}

