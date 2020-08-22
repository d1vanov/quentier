Quentier project uses clang-format tool for automatic formatting of source code. It is mandatory to run clang-format before contributing code fixes to Quentier. The version of clang-format tool must be at least 10. It is recommended to use prebuilt clang-format binaries from [here](https://github.com/muttleyxd/clang-format-static-binaries/releases).

The simplest way to run clang-format over the entire Quentier's codebase is to trigger `clang-format` build target. For this ensure that you have `clang-format` binary in your `PATH` (e.g. `export PATH=<path-to-folder-with-clang-format-tool>:$PATH` for Linux/macOS or `set PATH="<path-to-folder-with-clang-format-tool>";%PATH%` for Windows) and then run `cmake --build . --target clang-format` in your build directory. See [building/installation guide](INSTALL.md) for more details about the build process.

Alternatively you can run `clang-format` tool manually over the files you want to format:
```
clang-format -style=file -i <path-to-source-file-to-format>
```
The downside of this approach is that you need to run `clang-format` over every changed source file separately.

Clang-format can be integrated with various editors and IDEs. See [clang-format docs](https://clang.llvm.org/docs/ClangFormat.html) for reference. For vim it is recommended to use [this plugin](https://github.com/rhysd/vim-clang-format). Here are the options for it in `~/.vimrc` which are compatible with `Quentier` project:
```
let g:clang_format#command='<path-to-clang-format-binary>'
let g:clang_format#enable_fallback_style=0
let g:clang_format#auto_format=1
let g:clang_format#auto_format_on_insert_leave=0
let g:clang_format#detect_style_file=1
```

Style used by Quentier project is formalized in [.clang-format](.clang-format) file within the repo. You can examine the formatting options in it and see their description in [this doc](https://clang.llvm.org/docs/ClangFormatStyleOptions.html). Some of these rules are outlined below for reference.

## Indentation

Quentier uses spaces for indentation, not tabs. Tabs width and shift width should be exactly 4 spaces.

## Curly braces usage

**Always** use curly braces in conditional and loop operators, even if their bodies occupy just the single line of code. The rationale is to avoid the possibility of mistakes when extending the loop or conditional operator body in future: suppose you had `if` operator like this:

    if (a > b)
        a = b;

Then suppose in future some additional action becomes needed. It’s unfortunately quite common to make a mistake like this:

    if (a > b)
        a = b;
        b = c;

See the issue? Even though the code is formatted as if `b = c` is executed inside the conditional operator’s body, in reality it is executed *after* the conditional operator’s body:

    if (a > b)
        a = b;

    b = c;

In order to avoid that, please **always** use curly braces to clearly define the scope of the operator’s body:

    if (a > b) {
        a = b;
    }

    if (a > b) {
        a = b;
        b = c;
    }

In Quentier project opening curly brace for operators is put at the end of the first line of the conditional operator or loop operator unless the operator body consists of multiple lines. In the latter case the opening curly brace is put at the beginning of the next line. In clang-format configuration it corresponds to option `BraceWrappingAfterControlStatementStyle` having the value of `MultiLine`. See [this ticket](https://reviews.llvm.org/D68296) for reference.

In class declarations the opening curly brace should always be placed on the next line after the declaration:

    class MyClass
    {
        // < class contents >
    };

In function/method implementations the opening curly brace should also always be placed on the next line after the function/method definition:

    void MyClass::myMethod()
    {
        // < method implementation contents >
    }

    void myFunction()
    {
        // < function implementation contents >
    }

Finally, please let the closing curly brace fully occupy its line of code, don’t append anything to it. For example, do this:

    if (a > b) {
        a = b;
    }
    else {
        b = a;
    }

but not this:

    if (a > b) {
        a = b;
    } else {
        b = a;
    }

## Line length

Lay out the source code in such a way that any single line of code is no longer than 80 columns. It is a rather hard boundary and it's not always possible to satisfy this requirement. Clang-format tool would try its best to satisfy this requirement though. The point is to have the majority of code written in a compact enough way.

## Separate multi-line statements with blank lines

Multi-line statements generally read better when they are separated from other statements with blank lines. Example:

    bool res = myFunctionCall(
        myFirstParam, mySecondParam, myThirdParam, myFourthParam,
        myFifthParam);

    if (res) {
        // do something
    }

Note the blank line between the function call and the `if` operator. It's easier to grasp the boundary between the two when there is a blank line between them.

Exceptions from this rule can be made in certain cases, for example, for logging macros:

    QNDEBUG("Some long logging message which continues on the next line: "
        << someValueToLog);
    return true;

Clang-format does not intervene with blank lines placement (other than ensuring no consequent blank lines) so it is up to the developer where to put blank lines. Use them to separate conceptually different code fragments as well as to improve the readability of code fragments.

## Grouping and sorting of includes

`#include` statements should be split into groups separated by blank lines. Includes within each group should be sorted alphabetically. The order of include groups should be roughly the following:

 * For `.cpp` files: the primary header inclusion (i.e for `MyClass.cpp` `#include "MyClass.h"` should go first)
 * Local includes (i.e those using `""` instead of `<>`)
 * Global includes from Quentier's libraries (i.e. those like `<lib/widget/NoteEditorWidget.h>` and such)
 * Global includes from libquentier's public headers (i.e. those like `<quentier/utility/Printable.h>` or `<quentier/types/Note.h>` and such)
 * Global includes from Qt's headers (i.e. those like `<QApplication>`, `<QWidget>` and such)
 * Global includes from other 3rd party libraries i.e. boost
 * Global includes from the standard library (i.e. `<iostream>`, `<algorithm>` and such)

Example:

    #include "MyClass.h"

    #include "AnotherClass.h"
    #include "SomeOtherClass.h"
    #include "YetAnotherClass.h"

    #include <lib/widget/AboutQuentierWidget.h>
    #include <lib/widget/NoteEditorWidget.h>

    #include <quentier/types/Account.h>
    #include <quentier/types/Notebook.h>
    #include <quentier/types/SavedSearch.h>
    #include <quentier/types/User.h>

    #include <QApplication>
    #include <QTextEdit>
    #include <QWidget>

    #include <boost/bimap.hpp>
    #include <boost/multi_index.hpp>

    #include <algorithm>
    #include <iostream>
    #include <utility>

## Function/method/constructor/operator parameters

When *declaring* functions, methods, class constructors or operators their parameters should be laid out in either of two ways:

 * If the whole line with declaration and parameters (and keywords like `const`, `override`, `noexcept` and others) fits into 80 columns, let it be the single line. Example:
```
    void myFunction(const QString & paramOne, const QString & param2);
```
 * If the whole line with declaration and parameters (and keywords like `const`, `override`, `noexcept` and others) does not fit into 80 columns, the parameters list should start on the next line after the declaration and be indented with extra 4 spaces; the parameters should be laid out in such a way that each line fits into 80 columns. Example:
```
    void myOtherFunction(
        const QString & paramOne, const QString & paramTwo,
        const int paramThree, const bool paramFour);
```
Same rules apply for functions, methods, class constructors and operators *definitions* i.e. implementations:
```
    void myFunction(const QString & paramOne, const QString & param2)
    {
        // < function implementation contents >
    }

    void myOtherFunction(
        const QString & paramOne, const QString & paramTwo,
        const int paramThree, const bool paramFour)
    {
        // < function implementation contents >
    }
```
Similar rules apply when calling functions, methods or class constructors, only in those cases the rule applies to variable names, literals etc.

## Line endings

The «native» line endings for headers and sources of Quentier are considered to be Unix-style single LFs. If you are using Windows or OS X / macOS and want to contribute some code changes to Quentier, please [configure git](https://help.github.com/articles/dealing-with-line-endings/) to convert the line endings to LF on commit or on push.

## Source file names

The name of a header/source file should reflect the name of the class declared/implemented in the file. In case when the header/source file contains declarations/implementations for more than one class, the file name should be either the most important class’ name or some name encompassing the nature of those multiple classes. If the header/source file doesn’t contain C++ classes at all, the file name should just provide some reasonable hint about the contents of the file.

C++ header files have `.h` extension and source files have `.cpp` extension.

## Namespace

Although Quentier is not a library and hence does not have to be very picky about the used namespaces, most of Quentier's code resides in `quentier` namespace.

## Class names

Class names should be capitalized. If the class name consists of several words, each word should be capitalized and there should be no underscore between the words. Like this:

    class MyClass
    {
        // < class contents >
    };

## Class/struct member variable naming

Private class/struct member variable names should start with `m_` followed by a word in the lower case. If the name of the member variable consists of multiple words, all the words but the first one should be capitalized, without underscores in between. For example:

    class MyClass
    {
    private:
        double   m_averageCounter;
        bool     m_countingEnabled;
    };

Public class/struct member variables may skip this convention.

Also, not a strict rule but a rule of thumb: use tabular indentation to keep the names of several member variables starting at the same column. That makes them slightly more readable.

## Class methods / functions naming

The first word in the name of a class method or a function should start from the lower case. If the name of method/function consists of just one word, the entire word should appear in the lower case. If the name of the method/function consists of several words, all the words but the first one should be capitalized, without underscores in between. For example:

    class MyClass
    {
    public:
        void do();
        void doThis();
        void doThisAndThat();
    };

The obvious exceptions from this rule are constructors and destructors of the class, since their names must match the class names in which the first word is capitalized.

## Comments inside the source code

A good rule of thumb is to try making the code so explicit that it doesn’t require commenting. However, it is not always possible/feasible due to various reasons: optimization, workarounds for some framework bugs etc. In such cases the tricky code should have a comment nearby explaining what it does and why does it have to be so incomprehensible.

If you add new classes or class methods or functions, you are encouraged do document their declarations in Doxygen format so that the code is easier to read and comprehend.

Also, please don’t comment out portions of code «just in case, to have it around should it ever be needed again». Having the unused code around is the task of the version control system.

## Classes layout

Try to stick with the following layout of class contents:

- `Q_OBJECT` macro, if required for the class
- inner classes, if any
- typedefs, if any
- constructors - the default one, if present, comes first, then non-default constructors, if any, then copy-constructor, if present.
- assignment operators, if any
- destructor, if present
- Qt signals, if any
- public methods, including Qt slots
- protected methods, including Qt slots
- private methods, including Qt slots
- member variables

## Preferred method/function implementation layout

- In the beginning of the method/function implementation it is wise to check the validity of input parameters values and possibly any other conditions and return with error if something is wrong and the method cannot do its job. Try to avoid multiple return points from the method/function unless these are the returns with error in the beginning of the method/function body.
- Try to make the bodies of methods/functions as small as possible. Ideally, the body of a method/function should fit a single screen or at least 2-3 screens. If it’s larger, consider refactoring the single method into a series of methods.

## Signal/slot signatures

- Use `Q_SIGNALS` macro instead of `signals`, `Q_SLOTS` instead of `slots` and `Q_EMIT` instead of `emit`. The reason is that there are 3rdparty libraries which use `signals`, `slots` and `emit` keywords in their own code and interfaces and the name clashing can create problems should Quentier start to use such libraries some day.
- Pass signal/slot parameters by value, not by const reference, unless you are certain that the objects interacting via the particular signals/slots would live in the same thread. The power and convenience of Qt’s signals/slots comes from their flexibility to the thread affinity of the connected objects but unless you pass parameters by value in signals/slots, it would be your job to provide the proper thread safety guarantees. There’s no reason to do this yourself when Qt can do it for you.

## C++ standard used

Quentier uses C++14 standard so usage of any features from that standard version is allowed. However, from practical perspective Quentier's code should be written in such a way that major supported compilers would have no problems building this code. At the time of this writing the oldest supported compilers are g++ 5.4.1 on Linux (the default compiler of Ubuntu Xenial, oldest still supported LTS release of Ubuntu) and Visual Studio 2017 on Windows.

## Qt versions supported

Libquentier supports building with Qt no older than 5.5.1. Be careful with features relevant only for the most recent versions of Qt which can break backward compatibility with older versions of the framework. Either don't use such bleeding edge features or use ifdefs to isolate the code using them. Example:

    #if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    // use feature first appeared in Qt 5.7
    #else
    // use some replacement for older versions of Qt
    #endif
