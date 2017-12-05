# How to contribute

Contributions to Quentier are warmly welcome. However, in order to ensure the successful adoption of your contributions, please refer to this guide.

## Issues

This section explains how to create and fill GitHub issues in a way most helpful to the developers and community at large.

### How to report a bug

If you think you have found a bug in Quentier, you are kindly encouraged to create an issue and describe the bug you have encountered. However, before doing that please look through the existing [open](https://github.com/d1vanov/Quentier/issues) and [closed](https://github.com/d1vanov/Quentier/issues?q=is%3Aissue+is%3Aclosed) issues to see whether the bug you've encountered has already been reported or maybe even fixed in some version of Quentier. If you don't see anything like what you experience, please proceed to creating a new issue.

If Quentier crashes, here is what you should do:
 * If the version of Quentier you are using was built with crash handler support from Google breakpad library (currently only available for Linux and Windows versions of Quentier), the crashing app should display the window with excuses and some information which can help to diagnose the cause of the crash. Please include that information into your bug report. In the most ideal case, please also upload somewhere the generated minidump file (its location is also displayed in the post-crash window) as it can be very useful for troubleshooting.
 * If the version of Quentier you are using was built without the crash handler, consider trying to reproduce the issue with the version including the crash handler.
 * If you are using Quentier on Mac, the default crash handler on that platform also provides useful information for troubleshooting:
   * On Quentier crash you should notice the appearance of a window with exclamation mark and title like "The application Quentier quit unexpectedly"
   * That window would contain three buttons: "Close", "Report..." and "Reopen".
   * Press the "Report..." button: another window with title like "Problem Report for Quentier" should appear
   * Select "Problem details" tab (the middle one)
   * Copy the entire contents of the window and attach them to the issue
 * Regardless of the information from the crash handler, please describe what you were doing before the app crashed, in as much detail as possible.

If Quentier behaves in a way you would not expect, please do the following:
 * Describe in fine details what you were doing, which result you expected to get and what you actually got
 * Provide the trace-level application log collected while reproducing the issue


### How to suggest a new feature or improvement

If you have a new feature or improvement suggestion, please make sure you describe it with the appropriate level of detail and also describe at least one more or less realistic use case dictating the necessity of the change.


## How to contribute a bugfix/new feature/improvement

This section explains the preferred process for contributing some change of Quentier's code.

### Contributing a bugfix

If you have not only found a clear bug but have also identified its reason and would like to contribute a pull request with a bugfix, please go ahead but note the following:

 * `master` branch is meant to contain the current stable version of Quentier. Small and safe bugfixes are ok to land in `master`. More complicated bugfixes changing a lot of code are not indended for `master`. Instead, please contribute them to `development` branch.
 * Make sure the changes you propose agree with the [coding style](CodingStyle.md) of the project. If they don't but the bugfix you are contributing is good enough, there's a chance that your contribution would be accepted but the coding style would have to be cleaned up after that. You are encouraged to be a good citizen and not force others to clean up after you.
 * Try to ensure the bugfix you propose doesn't break something while fixing a different thing. For this try to make the fix as small and local as possible.

### Contributing a new feature or improvement

If you'd like to implement some new feature or improvement suggested by either yourself or someone else, please don't rush to write the code but first let the community members know you are going to implement the feature/improvement - it is required to prevent the duplication of effort. In the simplest case just write in the project issue tracker that you'd like to take some particular thing. 

The preferred approach would also include describing your implementation plan or ideas before the actual implementation. You can simply write something like

> I think it could be solved by adding method B to class A

or

> A good way to get it done seems to introduce class C which would handle D

It is required to ensure your vision is in line with that of other developers. Once the implementation plan is agreed, it is safe to start coding it with the knowledge that the code won't contradict the project's vision and would be accepted when ready. Please don't start the actual work on the new features before the vision agreement is achieved - it can lead to problems with code acceptance to Quentier.

When working on a feature or improvement implementation, please comply with the [coding style](CodingStyle.md) of the project.

All the new features and major improvements should be contributed to `development` branch, not `master`.


### What changes are generally NOT approved

- Breaking backward compatibility without a really good reason for that.
- The introduction of dependencies on strictly the latest versions of anything, like, on some feature existing only in the latest version of Qt.
- The direct unconditional usage of C++11/14/17 features breaking the build with older compilers not supporting them (see the [coding style](CodingStyle.md) doc for more info on that)
- The breakage of building/working with Qt4
